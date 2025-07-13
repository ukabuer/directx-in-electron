const {
  app,
  BrowserWindow,
  sharedTexture,
  ipcMain,
  nativeImage,
} = require("electron");
const path = require("path");
const { endianness } = require("os");
const { spawn } = require("child_process");

const target = "build/my-renderer.exe";

const startNewProcess = (hwnd) => {
  const data = endianness() == "LE" ? hwnd.readInt32LE() : hwnd.readInt32BE();
  const p = spawn(target, [data], {
    cwd: process.cwd(),
  });
  p.stdout.on("data", (data) => {
    console.log(`[native side] ${data}`);
  });
  p.stderr.on("data", (data) => {
    console.log(`[native side] ${data}`);
  });
};

const createWindow = () => {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
    },
  });

  win.loadFile(path.join(__dirname, "index.html"));
  win.webContents.setFrameRate(60);
  win.on("ready-to-show", () => {
    const hwnd = win.getNativeWindowHandle();
    startNewProcess(hwnd);
  });

  const osr = new BrowserWindow({
    width: 128,
    height: 128,
    show: false,
    webPreferences: {
      offscreen: {
        useSharedTexture: true,
      },
    },
  });

  osr.webContents.setFrameRate(240);
  osr.webContents.on("paint", (event) => {
    // Step 1: Input source of shared texture handle.
    const texture = event.texture;

    if (!texture) {
      console.error("No texture, GPU may be unavailable, skipping.");
      return;
    }

    // Step 2: Import as SharedTextureImported
    console.log(texture.textureInfo);
    const imported = sharedTexture.importSharedTexture(texture.textureInfo);

    // Step 3: Prepare for transfer to another process (win's renderer)
    const transfer = imported.startTransferSharedTexture();

    // Step 4: Send the shared texture to the renderer process (goto preload.js)
    win.webContents.send("shared-texture", transfer);
  });

  ipcMain.on("shared-texture-done", (event, id) => {
    // Step 12: Release the shared texture resources at main process
    const data = capturedTextures.get(id);
    if (data) {
      capturedTextures.delete(id);
      const { imported, texture } = data;

      // Step 13: Release the imported shared texture
      imported.release(() => {
        // Step 14: Release the shared texture once GPU is done
        texture.release();
      });

      // Step 15: Slightly timeout and capture the node screenshot
      setTimeout(async () => {
        // Step 16: Compare the captured image with the target image
        const captured = await win.webContents.capturePage({
          x: 16,
          y: 16,
          width: 128,
          height: 128,
        });

        // Step 17: Resize the target image to match the captured image size, in case dpr != 1
        const target = targetImage.resize({ ...captured.getSize() });

        // Step 18: nativeImage have error comparing pixel data when color space is different,
        // send to browser for comparison using canvas.
        win.webContents.send("verify-captured-image", {
          captured: captured.toDataURL(),
          target: target.toDataURL(),
        });
      }, 300);
    }
  });

  win.loadFile(htmlPath);
  osr.loadFile(osrPath);
};

app.whenReady().then(() => {
  createWindow();

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") app.quit();
});

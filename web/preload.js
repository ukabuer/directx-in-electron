const { sharedTexture } = require("electron");
const { ipcRenderer, contextBridge } = require("electron/renderer");

contextBridge.exposeInMainWorld("textures", {
  onSharedTexture: (cb) => {
    ipcRenderer.on("shared-texture", async (e, transfer) => {
      // Step 5: Get the shared texture from the transfer
      const imported = sharedTexture.finishTransferSharedTexture(transfer);
      // Step 6: Let the renderer render using WebGPU
      await cb(imported);

      // Step 10: Release the shared texture with a callback
      imported.release(() => {
        // Step 11: When GPU command buffer is done, we can notify the main process to release
        ipcRenderer.send("shared-texture-done", id);
      });
    });
  },
  verifyCapturedImage: (verify) => {
    ipcRenderer.on("verify-captured-image", (e, images) => {
      verify(images, (result) => {
        ipcRenderer.send("verify-captured-image-done", result);
      });
    });
  },
});

window.addEventListener("DOMContentLoaded", () => {
  const replaceText = (selector, text) => {
    const element = document.getElementById(selector);
    if (element) element.innerText = text;
  };

  for (const dependency of ["chrome", "node", "electron"]) {
    replaceText(`${dependency}-version`, process.versions[dependency]);
  }
});

window.textures.onSharedTexture(async (id, imported) => {
  try {
    // Step 7: Get VideoFrame from the imported texture
    const frame = imported.getVideoFrame();

    // Step 8: Render using WebGPU
    await window.renderFrame(frame);

    // Step 9: Release the VideoFrame as we no longer need it
    frame.close();
  } catch (error) {
    console.error("Error getting VideoFrame:", error);
  }
});

window.textures.verifyCapturedImage((images, result) => {
  const { captured, target } = images;
  // Step 19: Compare the captured image with the target image
  const capturedImage = new Image();
  capturedImage.src = captured;
});

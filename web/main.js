const { app, BrowserWindow, sharedTexture, ipcMain } = require("electron");
const path = require("node:path");
const { spawn } = require("node:child_process");

const NATIVE_RENDERER = "build/my-renderer.exe";
const TEX_WIDTH = 300;
const TEX_HEIGHT = 300;

const processId = process.pid;
console.log(
	`Electron app's main process id: ${processId}(0x${processId.toString(16)})`,
);

const startNativeRendererProcess = (onSharedHandle) => {
	const p = spawn(NATIVE_RENDERER, [processId], {
		cwd: process.cwd(),
	});
	p.stdout.on("data", (data) => {
		console.log(`[native side log] ${data}`);
		const dataStr = data.toString();
		const regex = /Texture shared handle: (.+)/;
		const matches = dataStr.match(regex);
		if (matches) {
			const hex = `0x${matches[1]}`;
			const handleInt = parseInt(hex, 16);
			console.log(
				`Received Shared Handle: ${handleInt}(0x${handleInt.toString(16)})`,
			);
			onSharedHandle(handleInt);
		}
	});
	p.stderr.on("data", (data) => {
		console.log(`[native side error] ${data}`);
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

	win.on("ready-to-show", async () => {
		const onSharedHandle = async (handleInt) => {
			const bytes = [];
			for (let i = 0; i < 8; i++) {
				const byte = handleInt & 0xff;
				bytes.push(byte);
				handleInt = handleInt >> 8;
			}
			const handle = Buffer.from(bytes);
			const imported = sharedTexture.importSharedTexture({
				textureInfo: {
					codedSize: {
						width: TEX_WIDTH,
						height: TEX_HEIGHT,
					},
					pixelFormat: "bgra",
					timestamp: count++,
					handle: {
						ntHandle: handle,
					},
				},
			});

			await sharedTexture.sendSharedTexture({
				frame: win.webContents.mainFrame,
				importedSharedTexture: imported,
			});

			imported.release();
		};
		startNativeRendererProcess(onSharedHandle);

		let count = 0;
		ipcMain.on("renderer", async (_, message) => {
			console.log(message);
			const handleInt = parseInt(message, 10);
			console.log(
				`Received Shared Handle: ${handleInt}(0x${handleInt.toString(16)})`,
			);
			onSharedHandle(handleInt);
		});
	});
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

const { app, BrowserWindow, sharedTexture, ipcMain } = require("electron");
const path = require("path");
const { endianness } = require("os");
const { spawn } = require("child_process");

const target = "build/my-renderer.exe";

console.log(process.pid, process.pid.toString(16));
console.log("0x" + process.pid.toString(16));

const startNativeRendererProcess = (hwnd) => {
	const data = endianness() === "LE" ? hwnd.readInt32LE() : hwnd.readInt32BE();
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

	win.on("ready-to-show", async () => {
		let count = 0;
		ipcMain.on("renderer", async (event, message) => {
			console.log(message);
			const value = parseInt(message);
			console.log(value);
			const value1 = message >> 8;
			const value2 = message & 0xff;
			console.log(value1, value2);
			const handle = Buffer.from([
				value1,
				value2,
				0x0,
				0x0,
				0x0,
				0x0,
				0x0,
				0x0,
			]);
			const handleData = handle.readInt32LE();
			console.log(handleData.toString(16));
			// setInterval(() => {
			const imported = sharedTexture.importSharedTexture({
				textureInfo: {
					codedSize: {
						width: 300,
						height: 300,
					},
					pixelFormat: "bgra",
					visibleRect: {
						x: 0,
						y: 0,
						width: 300,
						height: 300,
					},
					timestamp: count++,
					handle: {
						ntHandle: handle,
					},
				},
			});

			console.log(imported);
			console.log("import: " + Date.now());

			await sharedTexture.sendSharedTexture({
				frame: win.webContents.mainFrame,
				importedSharedTexture: imported,
			});

			console.log("sent: " + Date.now());

			// imported.release();
			// }, 1000);
		});
	});
};

app.whenReady().then(() => {
	// startNativeRendererProcess(hwnd);

	createWindow();

	app.on("activate", () => {
		if (BrowserWindow.getAllWindows().length === 0) createWindow();
	});
});

app.on("window-all-closed", () => {
	if (process.platform !== "darwin") app.quit();
});

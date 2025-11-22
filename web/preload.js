const { sharedTexture } = require("electron");
const { contextBridge, ipcRenderer } = require("electron/renderer");

contextBridge.exposeInMainWorld("onSharedTexture", async (cb) => {
	sharedTexture.setSharedTextureReceiver(async ({ importedSharedTexture }) => {
		// const frame = importedSharedTexture.getVideoFrame();
		// const buffer = new Uint8Array(frame.allocationSize());
		// console.log(frame.allocationSize());

		// console.log("begin copy");

		// const layout = await frame.copyTo(buffer);
		// console.log(buffer);
		// console.log(layout);

		console.log("setSharedTextureReceiver");
		console.log(importedSharedTexture);
		return cb(importedSharedTexture);
	});
});

contextBridge.exposeInMainWorld("sendMessageToMain", (message) => {
	console.log(message);
	ipcRenderer.send("renderer", message);
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

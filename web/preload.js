const { sharedTexture } = require("electron");
const { contextBridge, ipcRenderer } = require("electron/renderer");

contextBridge.exposeInMainWorld("onSharedTexture", async (cb) => {
	sharedTexture.setSharedTextureReceiver(async ({ importedSharedTexture }) => {
		return cb(importedSharedTexture);
	});
});

contextBridge.exposeInMainWorld("sendMessageToMain", (message) => {
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

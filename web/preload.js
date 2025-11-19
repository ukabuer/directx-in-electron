const { sharedTexture } = require("electron");
const { contextBridge } = require("electron/renderer");

contextBridge.exposeInMainWorld("onSharedTexture", (cb) => {
	sharedTexture.setSharedTextureReceiver(({ importedSharedTexture }) => {
		cb(importedSharedTexture);
	});
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

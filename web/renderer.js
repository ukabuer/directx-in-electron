// Import WebGPU utilities
const canvas = document.createElement("canvas");
canvas.width = 128;
canvas.height = 128;
canvas.style.width = "128px";
canvas.style.height = "128px";
canvas.style.position = "absolute";
canvas.style.top = "16px";
canvas.style.left = "16px";

document.body.appendChild(canvas);
const context = canvas.getContext("2d");

window.onSharedTexture(async (imported) => {
	try {
		/**
		 * @type {VideoFrame}
		 */
		const frame = imported.getVideoFrame();

		context.drawImage(frame);

		frame.close();

		imported.release();
	} catch (error) {
		console.error("Error getting VideoFrame:", error);
	}
});

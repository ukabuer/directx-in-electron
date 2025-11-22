const TEX_WIDTH = 300;
const TEX_HEIGHT = 300;

const canvas = document.createElement("canvas");
canvas.width = TEX_WIDTH;
canvas.height = TEX_HEIGHT;
canvas.style.width = `${canvas.width}px`;
canvas.style.height = `${canvas.height}px`;
canvas.style.padding = "16px";
canvas.style.display = "block";

document.body.appendChild(canvas);
const context = canvas.getContext("2d");

window.onSharedTexture(async (imported) => {
	try {
		/** @type {VideoFrame} */
		const frame = imported.getVideoFrame();
		const draw = () => {
			context.drawImage(frame, 0, 0, canvas.width, canvas.height);
			requestAnimationFrame(draw);
		};
		requestAnimationFrame(draw);
		// frame.close();
		// imported.release();
	} catch (error) {
		canvas.style.backgroundColor = "red";
		console.error("Error getting VideoFrame:", error);
	}
});

/*
const input = document.createElement("input");
input.width = 200;
input.type = "text";
document.body.appendChild(input);
const button = document.createElement("button");
button.textContent = "send";
document.body.appendChild(button);
button.onmouseup = (e) => {
	window.sendMessageToMain(input.value);
};
*/

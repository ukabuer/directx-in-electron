const input = document.createElement("input");
input.width = 300;
input.type = "text";
document.body.appendChild(input);
const button = document.createElement("button");
button.textContent = "send";
document.body.appendChild(button);
button.onmouseup = (e) => {
	console.log(input.value);
	window.sendMessageToMain(input.value);
};

const canvas = document.createElement("canvas");
canvas.width = 300;
canvas.height = 300;
canvas.style.width = "300px";
canvas.style.height = "300px";
canvas.style.position = "absolute";
canvas.style.top = "160px";
canvas.style.left = "16px";
canvas.style.backgroundColor = "red";

document.body.appendChild(canvas);
const context = canvas.getContext("2d");

window.onSharedTexture(async (imported) => {
	try {
		/**
		 * @type {VideoFrame}
		 */
		const frame = imported.getVideoFrame();
		console.log(frame.codedWidth, frame.codedHeight);
		console.log(frame);

		console.log(frame.allocationSize());
		// const buffer = new Uint8Array(frame.allocationSize());
		// const layout = await frame.copyTo(buffer);
		// console.log(buffer);
		// console.log(layout);
		console.log("received: " + Date.now());
		// setInterval(() => {
		canvas.style.backgroundColor = "green";
		context.drawImage(frame, 0, 0, 300, 300);
		// }, 100);

		// frame.close();

		// imported.release();
	} catch (error) {
		canvas.style.backgroundColor = "blue";
		console.error("Error getting VideoFrame:", error);
	}
});

// globals begin
wasm = null; buffer = null; frameFunc = undefined;
// globals end

ctx = document.getElementById("screen").getContext("2d");
ctx.canvas.width = 800;
ctx.canvas.height = 450;

function raylib_js_set_frame(frame) {
    frameFunc = wasm.__indirect_function_table.get(frame);
}

function ColorPtrToColor(colorPtr) {
    if (wasm === null) throw new Error("wasm not loaded");
    var [r, g, b, a] = new Uint8Array(buffer, colorPtr, 4);
    r = r.toString(16).padStart(2, "0");
    g = g.toString(16).padStart(2, "0");
    b = b.toString(16).padStart(2, "0");
    a = a.toString(16).padStart(2, "0");
    return "#"+r+g+b+a;
}

function FillBackground(colorPtr) {
    ctx.fillStyle = ColorPtrToColor(colorPtr);
    ctx.fillRect(0, 0, 800, 450);
}

function DrawSegment(startPtr, endPtr, colorPtr, thick) {
    [sx, sy] = new Float32Array(buffer, startPtr, 2);
    [ex, ey] = new Float32Array(buffer, endPtr, 2);
    ctx.beginPath();
    ctx.strokeStyle = ColorPtrToColor(colorPtr);
    ctx.lineWidth = thick;
    ctx.moveTo(sx, sy);
    ctx.lineTo(ex, ey);
    ctx.stroke();
    console.log("Shit");
}

async function init() {
    const { instance } = await WebAssembly.instantiateStreaming(
        fetch("./index.wasm"), {env: {
            raylib_js_set_frame, FillBackground, DrawSegment
        }}
    );
    wasm = instance.exports;
    buffer = wasm.memory.buffer;
    wasm.main();
    const next = (timestamp) => {
        frameFunc();
        window.requestAnimationFrame(next);
    };
    window.requestAnimationFrame((timestamp) => {
        this.previous = timestamp;
        window.requestAnimationFrame(next);
    });
}
console.log("???");
init();
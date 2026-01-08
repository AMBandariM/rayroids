// globals begin
wasm = null; buffer = null; frameFunc = undefined;
prevDownKeys = new Set(); currDownKeys = new Set();
dt = undefined; prevTimestamp = undefined;
fpsTimeAcc = 0; fpsFrameCount = 0; fps = 60;
// globals end

ctx = document.getElementById("screen").getContext("2d");

// special function for raylib.js
function raylib_js_set_frame(frame) {
    frameFunc = wasm.__indirect_function_table.get(frame);
}

// utils begin
function ColorPtrToColor(colorPtr) {
    if (wasm === null) throw new Error("wasm not loaded");
    var [r, g, b, a] = new Uint8Array(buffer, colorPtr, 4);
    r = r.toString(16).padStart(2, "0");
    g = g.toString(16).padStart(2, "0");
    b = b.toString(16).padStart(2, "0");
    a = a.toString(16).padStart(2, "0");
    return "#"+r+g+b+a;
}

function CStrLen(buffer, cstrPtr) {
    let len = 0;
    while (buffer[cstrPtr++] != 0) len++;
    return len;
}

function CStrPtrToString(cstrPtr) {
    const mem = new Uint8Array(buffer);
    const len = CStrLen(mem, cstrPtr);
    return new TextDecoder().decode(new Uint8Array(buffer, cstrPtr, len));
}

const glfwKeyMapping = {
    "Space":          32,
    "Quote":          39,
    "Comma":          44,
    "Minus":          45,
    "Period":         46,
    "Slash":          47,
    "Digit0":         48,
    "Digit1":         49,
    "Digit2":         50,
    "Digit3":         51,
    "Digit4":         52,
    "Digit5":         53,
    "Digit6":         54,
    "Digit7":         55,
    "Digit8":         56,
    "Digit9":         57,
    "Semicolon":      59,
    "Equal":          61,
    "KeyA":           65,
    "KeyB":           66,
    "KeyC":           67,
    "KeyD":           68,
    "KeyE":           69,
    "KeyF":           70,
    "KeyG":           71,
    "KeyH":           72,
    "KeyI":           73,
    "KeyJ":           74,
    "KeyK":           75,
    "KeyL":           76,
    "KeyM":           77,
    "KeyN":           78,
    "KeyO":           79,
    "KeyP":           80,
    "KeyQ":           81,
    "KeyR":           82,
    "KeyS":           83,
    "KeyT":           84,
    "KeyU":           85,
    "KeyV":           86,
    "KeyW":           87,
    "KeyX":           88,
    "KeyY":           89,
    "KeyZ":           90,
    "BracketLeft":    91,
    "Backslash":      92,
    "BracketRight":   93,
    "Backquote":      96,
    //  GLFW_KEY_WORLD_1   161 /* non-US #1 */
    //  GLFW_KEY_WORLD_2   162 /* non-US #2 */
    "Escape":         256,
    "Enter":          257,
    "Tab":            258,
    "Backspace":      259,
    "Insert":         260,
    "Delete":         261,
    "ArrowRight":     262,
    "ArrowLeft":      263,
    "ArrowDown":      264,
    "ArrowUp":        265,
    "PageUp":         266,
    "PageDown":       267,
    "Home":           268,
    "End":            269,
    "CapsLock":       280,
    "ScrollLock":     281,
    "NumLock":        282,
    "PrintScreen":    283,
    "Pause":          284,
    "F1":             290,
    "F2":             291,
    "F3":             292,
    "F4":             293,
    "F5":             294,
    "F6":             295,
    "F7":             296,
    "F8":             297,
    "F9":             298,
    "F10":            299,
    "F11":            300,
    "F12":            301,
    "F13":            302,
    "F14":            303,
    "F15":            304,
    "F16":            305,
    "F17":            306,
    "F18":            307,
    "F19":            308,
    "F20":            309,
    "F21":            310,
    "F22":            311,
    "F23":            312,
    "F24":            313,
    "F25":            314,
    "NumPad0":        320,
    "NumPad1":        321,
    "NumPad2":        322,
    "NumPad3":        323,
    "NumPad4":        324,
    "NumPad5":        325,
    "NumPad6":        326,
    "NumPad7":        327,
    "NumPad8":        328,
    "NumPad9":        329,
    "NumpadDecimal":  330,
    "NumpadDivide":   331,
    "NumpadMultiply": 332,
    "NumpadSubtract": 333,
    "NumpadAdd":      334,
    "NumpadEnter":    335,
    "NumpadEqual":    336,
    "ShiftLeft":      340,
    "ControlLeft" :   341,
    "AltLeft":        342,
    "MetaLeft":       343,
    "ShiftRight":     344,
    "ControlRight":   345,
    "AltRight":       346,
    "MetaRight":      347,
    "ContextMenu":    348,
    //  GLFW_KEY_LAST   GLFW_KEY_MENU
}
//utils end

function InitWindow(width, height, titlePtr) {
    ctx.canvas.width = width;
    ctx.canvas.height = height;
    const tit = CStrPtrToString(titlePtr);
    document.title = tit;
}

function ClearBackground(colorPtr) {
    ctx.fillStyle = ColorPtrToColor(colorPtr);
    ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);
}

function DrawLineEx(startPtr, endPtr, thick, colorPtr) {
    [sx, sy] = new Float32Array(buffer, startPtr, 2);
    [ex, ey] = new Float32Array(buffer, endPtr, 2);
    ctx.beginPath();
    ctx.strokeStyle = ColorPtrToColor(colorPtr);
    ctx.lineWidth = thick;
    ctx.moveTo(sx, sy);
    ctx.lineTo(ex, ey);
    ctx.stroke();
}

function EndDrawing() {
    prevDownKeys.clear()
    prevDownKeys = new Set(currDownKeys);

    fpsFrameCount++;
    fpsTimeAcc += dt;
    if (fpsTimeAcc >= 1000.0) {
        fpsTimeAcc = 0;
        fps = fpsFrameCount;
        fpsFrameCount = 0;
    }
}

function IsKeyPressed(key) {
    return !prevDownKeys.has(key) && currDownKeys.has(key);
}
function IsKeyDown(key) {
    return currDownKeys.has(key);
}
function IsKeyReleased(key) {
    return prevDownKeys.has(key) && !currDownKeys.has(key);
}

function GetFrameTime() {
    if (dt === undefined) return 1.0; 
    return dt / 1000.0;
}

function GetFPS() {
    return fps;
}

let textFormatBufferPtr = null;
let textFormatBufferPivot = null;
const fixedBufferSize = 65536;
function allocateTextFormatBuffer() {
    if (textFormatBufferPtr !== null) return;
    textFormatBufferPtr = buffer.byteLength;
    textFormatBufferPivot = textFormatBufferPtr;
    wasm.memory.grow(1);
    buffer = wasm.memory.buffer;
}
function TextFormat(formatPtr, varargPtr) {
    const format = CStrPtrToString(formatPtr);
    const placeholderRegex = /%(0)?(\d*)d/g;

    let argIndex = 0;
    const formatted = format.replace(placeholderRegex, (match, zeroPad, widthStr) => {
        const [arg] = new Int32Array(buffer, varargPtr + argIndex * 4, 1);
        argIndex++;
        let numStr = arg.toString();
        const padChar = zeroPad ? '0' : ' ';
        const width = parseInt(widthStr, 10) || 0;

        if (width > numStr.length) {
            if (padChar === '0' && numStr[0] === '-') {
                const absStr = numStr.slice(1);
                const padded = absStr.padStart(width - 1, '0');
                numStr = '-' + padded;
            } else {
                numStr = numStr.padStart(width, padChar);
            }
        }
        return numStr;
    });

    const encoder = new TextEncoder();
    const bytes = encoder.encode(formatted);
    let writeLen = Math.min(bytes.length, fixedBufferSize - 1);
    if (textFormatBufferPivot + writeLen + 10 > textFormatBufferPtr + fixedBufferSize) textFormatBufferPivot = textFormatBufferPtr;
    new Uint8Array(buffer, textFormatBufferPivot, writeLen).set(bytes.slice(0, writeLen));
    new Uint8Array(buffer, textFormatBufferPivot + writeLen, 1)[0] = 0;
    const ret = textFormatBufferPivot;
    textFormatBufferPivot += writeLen + 1;
    return ret;
    
}

function DrawText(textPtr, posX, posY, fontSize, colorPtr) {
    const text = CStrPtrToString(textPtr);
    const color = ColorPtrToColor(colorPtr);
    ctx.fillStyle = color;
    fontSize *= 0.70;
    ctx.font = `${fontSize}px grixel`;
    ctx.fillText(text, posX, posY + fontSize / 2);
}

function MeasureText(textPtr, fontSize) {
    fontSize *= 0.70;
    ctx.font = `${fontSize}px grixel`;
    return ctx.measureText(CStrPtrToString(textPtr)).width;
}

function GetRandomValue(min, max) {
    return min + Math.floor(Math.random()*(max - min + 1));
}

async function init() {
    const { instance } = await WebAssembly.instantiateStreaming(
        fetch("./index.wasm"), {env: {
            raylib_js_set_frame, InitWindow, ClearBackground, DrawLineEx, EndDrawing, IsKeyPressed, IsKeyDown, IsKeyReleased,
            GetFrameTime, GetFPS, TextFormat, MeasureText, DrawText, GetRandomValue,
            cosf: Math.cos, sinf: Math.sin
        }}
    );
    wasm = instance.exports;
    buffer = wasm.memory.buffer;
    allocateTextFormatBuffer()
    wasm.main();
    const next = (timestamp) => {
        dt = timestamp - prevTimestamp;
        prevTimestamp = timestamp;
        frameFunc();
        window.requestAnimationFrame(next);
    };
    window.requestAnimationFrame((timestamp) => {
        prevTimestamp = timestamp;
        window.requestAnimationFrame(next);
    });
    window.addEventListener("keydown", (e) => {currDownKeys.add(glfwKeyMapping[e.code]);});
    window.addEventListener("keyup", (e) => {currDownKeys.delete(glfwKeyMapping[e.code]);});
}
init();
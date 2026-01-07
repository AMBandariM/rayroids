typedef unsigned char byte;
typedef struct {byte r; byte g; byte b; byte a;} Color;
typedef struct {float x; float y;} Vec2;

void IWait();
void FillBackground(Color color);
void DrawSegment(Vec2 start, Vec2 end, Color color, float thick);
void raylib_js_set_frame(void (*entry)(void));

const Color red = (Color){ .r = 255, .g = 0, .b = 0, .a = 255 };
const Color blue = (Color){ .r = 0, .g = 0, .b = 255, .a = 255 };
const Color black = (Color){ .r = 0, .g = 0, .b = 0, .a = 255};

float x = 0.0f;
void frame(void) {
    FillBackground(red);
    DrawSegment((Vec2){ .x = x, .y = 200.0f }, (Vec2){ .x = x + 100.0f, .y = 200.0f }, black, 3.0f);
    x += 1.0f; if (x > 200.0f) x = 0.0f;
}

int main(void) {
    FillBackground(blue);
    DrawSegment((Vec2){ .x = 100, .y = 100}, (Vec2){ .x = 200, .y = 200}, red, 3.0f);
    DrawSegment((Vec2){ .x = 200, .y = 200}, (Vec2){ .x = 300, .y = 100}, red, 1.0f);


    raylib_js_set_frame(frame);
}
#ifdef PLATFORM_WEB
#include <raylib.js.h>
#else
#include <raylib.h>
#include <raymath.h>
#endif

#ifndef __linux__
#define ToggleFullscreen ToggleBorderlessWindowed
#endif

// ---------------------------------------------------------------------------------------------
//     DEFINITIONS
// ---------------------------------------------------------------------------------------------

typedef enum { GS_MENU, GS_GAME, GS_OVER, GS_CREDITS } GameScene;

typedef enum { MS_START_NEW_GAME, MS_TOGGLE_FULLSCREEN, MS_CREDITS, MS_EXIT, MS_N } MenuState;
const char *menu_state_names[] = { "Start New Game", "Toggle Fullscreen", "Credits", "Exit" };

#define s_ship_body 4
const Vector2 ship_body[s_ship_body] = {
    (Vector2){ .x = 32.0f, .y = 0.0f},
    (Vector2){ .x = -24.0f, .y = -18.0f},
    (Vector2){ .x = -8.0f, .y = 0.0f},
    (Vector2){ .x = -24.0f, .y = 18.0f},    
};

typedef struct {
    Vector2 start;
    Vector2 diff;
    float   timer;
} Bullet;

typedef struct {
    unsigned char size;  // at most 8
    Vector2  center;
    Vector2  nodes[8];   // number of nodes is specified by size
    Vector2  speed;
    float    angular_speed;
} Meteor;

// ---------------------------------------------------------------------------------------------
//     GLOBAL VARIABLES
// ---------------------------------------------------------------------------------------------

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900
int screen_width = SCREEN_WIDTH, screen_height = SCREEN_HEIGHT;
// general state
GameScene game_scene = GS_MENU;
// menu state
MenuState menu_state = MS_START_NEW_GAME;
// game state
Vector2 ship_pos; float ship_curr_speed; const float ship_max_speed = 200.0f; const float ship_acceleration = 500.0f;
float ship_angle; const float angular_speed = 0.2f; const float angular_speed_inplace_add = 0.3f;
float shooter_timer; const float shoot_cooldown = 0.1f;
#define max_n_bullets 10
int n_bullets; Bullet bullets[max_n_bullets] = {0}; const float bullet_time = 0.3f;
#define max_n_meteors 64
int n_meteors; Meteor meteors[max_n_meteors] = {0}; int n_new_meteors; Meteor new_meteors[max_n_meteors];
float meteor_timer; const float meteor_cooldown = 10.0f;
int score, highest = 0, hp;
Shader space_shader;
bool running = true;
Sound shootSound;
Sound meteorSound;

// ---------------------------------------------------------------------------------------------
//     FUNCTIONS
// ---------------------------------------------------------------------------------------------

void init_global_variables() {
    int read_n; unsigned char *read_data = LoadFileData("data.bin", &read_n); if (read_n) highest = *(int *)read_data; UnloadFileData(read_data);
    space_shader = LoadShader(0, "shaders/space.fs");
    shootSound = LoadSound("shoot.wav");
    meteorSound = LoadSound("meteor.wav");
    SetSoundVolume(shootSound, 0.7);
    SetSoundVolume(meteorSound, 0.5);
}

void init_gameplay() {
    ship_pos = (Vector2){ .x = (float)screen_width / 2.0f, .y = (float)screen_height / 2.0f }; ship_curr_speed = 0.0f;
    shooter_timer = meteor_timer = 0.0f; ship_angle = -PI / 2.0f; score = 0; n_bullets = n_new_meteors = n_meteors = 0; hp = 1;
}

void tik_tok(float dt) {
    for (int i = 0; i < n_bullets; ++i) {
        bullets[i].timer -= dt;
        if (bullets[i].timer <= 0.0f) {
            bullets[i] = bullets[n_bullets - 1];
            n_bullets--;
        }
    }
    shooter_timer -= dt; if (shooter_timer <= 0.0f) shooter_timer = 0.0f;
    meteor_timer -= dt; if (meteor_timer <= 0.0f) meteor_timer = 0.0f;
}

void game_frame() {
    screen_width = GetRenderWidth(); screen_height = GetRenderHeight();
    BeginDrawing();
    switch (game_scene) {
    case GS_MENU: {
        if (IsKeyPressed(KEY_ESCAPE)) running = false;
        float t = (float)GetTime(); SetShaderValue(space_shader, GetShaderLocation(space_shader, "time"), &t, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(space_shader); DrawRectangle(-10, -10, 4000, 4000, BLACK); EndShaderMode();
    
        for (int i = 0; i < MS_N; ++i) DrawText(menu_state_names[i], 100, 100 + 50 * i, 48, menu_state == i ? IsKeyDown(KEY_SPACE)? GREEN : RED : DARKBROWN);
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menu_state = (menu_state + 1) % MS_N;
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) menu_state = (menu_state - 1 + MS_N) % MS_N;
        if (IsKeyReleased(KEY_SPACE)) switch (menu_state) {
            case MS_START_NEW_GAME: {
                init_gameplay();
    
                game_scene = GS_GAME;
            } break;
            case MS_TOGGLE_FULLSCREEN: ToggleFullscreen(); break;
            case MS_CREDITS: game_scene = GS_CREDITS; break;
            case MS_EXIT: running = false; break;
            default: TraceLog(LOG_FATAL, "Unreachable!");
        }
    } break;
    case GS_GAME: {
        if (IsKeyPressed(KEY_ESCAPE)) game_scene = GS_MENU;
        float t = (float)GetTime(); SetShaderValue(space_shader, GetShaderLocation(space_shader, "time"), &t, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(space_shader); DrawRectangle(-10, -10, 4000, 4000, BLACK); EndShaderMode();
        float dt = GetFrameTime();
        // player rotation
        float rotation_dir = 0.0f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) rotation_dir = 1.0f;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) rotation_dir = -1.0f;
        ship_angle += rotation_dir * dt * (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W) || shooter_timer > EPSILON ? angular_speed : angular_speed + angular_speed_inplace_add) * 2 * PI;
        // player move and shoot
        ship_curr_speed += (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W) ? 1.0f : -1.5f) * ship_acceleration * dt;
        ship_curr_speed = Clamp(ship_curr_speed, 0, ship_max_speed);
        ship_pos = Vector2Add(ship_pos, Vector2Rotate((Vector2){ .x = ship_curr_speed * dt, .y = 0.0f }, ship_angle));
        if (IsKeyDown(KEY_SPACE) && shooter_timer < EPSILON && n_bullets < max_n_bullets) {
            shooter_timer = shoot_cooldown;
            bullets[n_bullets] = (Bullet){
                .start = Vector2Add(ship_pos, Vector2Rotate(Vector2Add(ship_body[0], (Vector2){ .x = 20.0f, .y = 0.0f }), ship_angle)),
                .diff = Vector2Add(ship_body[0], Vector2Rotate((Vector2){ .x = 500.0f, .y = 0.0f }, ship_angle)),
                .timer = bullet_time
            };
            n_bullets++;
            SetSoundPitch(shootSound, 1.0 + GetRandomValue(-1, +1) / 30.0);
            PlaySound(shootSound);
        }
        // player screen limit
        if (ship_pos.x < 0.0f) ship_pos.x += screen_width;
        else if (ship_pos.x >= screen_width) ship_pos.x -= screen_width;
        if (ship_pos.y < 0.0f) ship_pos.y += screen_height;
        else if (ship_pos.y >= screen_height) ship_pos.y -= screen_height;
        // meteor creation
        if (meteor_timer < EPSILON) {
            if (n_meteors + n_new_meteors < max_n_meteors) {
                new_meteors[n_new_meteors].size = 8;
                float theta = 2.0 * PI * GetRandomValue(0, 314) / 314;
                new_meteors[n_new_meteors].center = Vector2Add((Vector2){ .x = (float)screen_width / 2.0f, (float)screen_height / 2.0f }, Vector2Rotate((Vector2){ .x = (float)screen_width / 2.0f + (float)screen_height / 2.0f, .y = 0.0f }, theta));
                for (int i = 0; i < 8; ++i) new_meteors[n_new_meteors].nodes[i] = Vector2Rotate((Vector2){ .x = 1.0f * GetRandomValue(25 + 3 * 8, 40 + 3 * 8), .y = 0.0f }, 2.0f * PI * i / 8);
                new_meteors[n_new_meteors].speed = Vector2Rotate((Vector2){ .x = 100.0, .y = 0}, theta + PI);
                new_meteors[n_new_meteors].angular_speed = GetRandomValue(-10, 10) / 10.0;
                n_new_meteors++;
            }
            int tmp = 0, tmpscr = score; while (tmpscr) { tmp++; tmpscr /= 10; }
            meteor_timer += meteor_cooldown / (tmp + 1);
        }
        // meteor move
        for (int i = 0; i < n_meteors; ++i) {
            meteors[i].center = Vector2Add(meteors[i].center, Vector2Scale(meteors[i].speed, dt));
            for (int j = 0; j < meteors[i].size; ++j) meteors[i].nodes[j] = Vector2Rotate(meteors[i].nodes[j], meteors[i].angular_speed * dt);
        }
        for (int i = 0; i < n_new_meteors; ++i) {
            new_meteors[i].center = Vector2Add(new_meteors[i].center, Vector2Scale(new_meteors[i].speed, dt));
            for (int j = 0; j < new_meteors[i].size; ++j) new_meteors[i].nodes[j] = Vector2Rotate(new_meteors[i].nodes[j], new_meteors[i].angular_speed * dt);
        }
        // new meteor to meteor
        for (int i = 0; i < n_new_meteors; ++i) {
            float mix = new_meteors[i].center.x, miy = new_meteors[i].center.y;
            if (mix > 60.0f && mix < screen_width - 60.0f && miy > 60.0f && miy < screen_height - 60.0f) {
                meteors[n_meteors] = new_meteors[i];
                n_new_meteors--;
                new_meteors[i] = new_meteors[n_new_meteors];
                n_meteors++;
            }
        }
        // meteor screen limit
        for (int i = 0; i < n_meteors; ++i) {
            float mix = meteors[i].center.x, miy = meteors[i].center.y;
            if (mix < 0.0f) meteors[i].center.x += screen_width;
            else if (mix >= screen_width) meteors[i].center.x -= screen_width;
            if (miy < 0.0f) meteors[i].center.y += screen_height;
            else if (miy >= screen_height) meteors[i].center.y -= screen_height;
        }
        tik_tok(dt);
        for (int dx = -1; dx <= 1; ++dx) for (int dy = -1; dy <= 1; ++dy) {
            Vector2 d = { .x = screen_width * dx, .y = screen_height * dy };
            // player graphics
            for (int i = 0; i < s_ship_body; ++i) {
                Vector2 point = Vector2Add(Vector2Add(ship_pos, d), Vector2Rotate(ship_body[i], ship_angle));
                DrawLineEx(point, Vector2Add(Vector2Add(ship_pos, d), Vector2Rotate(ship_body[(i + 1) % s_ship_body], ship_angle)), 2.0f, RAYWHITE);
                // collision
                if (dx == 0 && dy == 0) {
                    for (int j = 0; j < n_meteors; ++j) if (CheckCollisionPointPoly(Vector2Subtract(point, meteors[j].center), meteors[j].nodes, meteors[j].size)) hp--;
                    if (!hp) game_scene = GS_OVER;
                }
            }
            for (int bul = 0; bul < n_bullets; ++bul) {
                float t = (bullet_time - bullets[bul].timer) / bullet_time;
                Vector2 shift = (Vector2){ .x = 1.0f * dx * SCREEN_WIDTH, .y = 1.0f * dy * SCREEN_HEIGHT };
                Vector2 lead = Vector2Add(Vector2Add(bullets[bul].start, shift), Vector2Scale(bullets[bul].diff, t*t));
                DrawLineEx(Vector2Add(Vector2Add(bullets[bul].start, shift), Vector2Scale(bullets[bul].diff, t)), lead, 4.0f, RED);
                // meteor physics
                for (int i = 0; i < n_meteors; ++i) if (CheckCollisionPointPoly(Vector2Subtract(lead, meteors[i].center), meteors[i].nodes, meteors[i].size)) {
                    Meteor tmp = meteors[i];
                    meteors[i] = meteors[n_meteors - 1];
                    SetSoundPitch(meteorSound, 1.0 + GetRandomValue(-10, +10) / 50.0);
                    PlaySound(meteorSound);
                    if (tmp.size > 5) {
                        for (int j = n_meteors - 1; j <= n_meteors && j < max_n_meteors; ++j) {
                            meteors[j] = tmp; meteors[j].size--;
                            for (int k = 0; k < meteors[j].size; ++k) meteors[j].nodes[k] = Vector2Rotate((Vector2){ .x = 1.0f * GetRandomValue(25 + 3 * meteors[j].size, 40 + 3 * meteors[j].size), .y = 0.0f }, 2.0f * PI * k / meteors[j].size);
                            meteors[j].speed = Vector2Rotate(meteors[j].speed, 1.0 * (2 * (j - n_meteors) + 1));
                            meteors[j].angular_speed = GetRandomValue(-10, 10) / 10.0;
                        }
                        if (n_meteors < max_n_meteors - 1) n_meteors++;
                    }
                    else n_meteors--;
                    score++; bullets[bul] = bullets[n_bullets - 1]; n_bullets--; break;
                }
            }
            // meteor graphics
            for (int i = 0; i < n_meteors; ++i) for (int j = 0; j < meteors[i].size; ++j) DrawLineEx(Vector2Add(d, Vector2Add(meteors[i].center, meteors[i].nodes[j])), Vector2Add(d, Vector2Add(meteors[i].center, meteors[i].nodes[(j + 1) % meteors[i].size])), 2.0f, RAYWHITE);
        }
        // meteor graphics cont.
        for (int i = 0; i < n_new_meteors; ++i) for (int j = 0; j < new_meteors[i].size; ++j) DrawLineEx(Vector2Add(new_meteors[i].center, new_meteors[i].nodes[j]), Vector2Add(new_meteors[i].center, new_meteors[i].nodes[(j + 1) % new_meteors[i].size]), 2.0f, GRAY);
        // texts
        DrawText(TextFormat("SCORE :: %08d / %08d", score, highest), 20, 20, 20, GRAY);
        DrawText(TextFormat("FPS :: %d %d %d", GetFPS(), n_meteors, n_new_meteors), 20, screen_height - 40, 20, GRAY);
    } break;
    case GS_OVER: {
        if (IsKeyPressed(KEY_ESCAPE)) game_scene = GS_MENU;
        const bool new_high = score > highest; 
        Color text_color = new_high ? GREEN : RED;
        DrawText(new_high ? "NEW HIGH SCORE" : "GAME OVER", (screen_width - MeasureText(new_high ? "NEW HIGH SCORE" : "GAME OVER", 48)) / 2, screen_height / 2 - 50, 48, text_color);
        const char *score_text = TextFormat("SCORE :: %08d", score);
        DrawText(score_text, (screen_width - MeasureText(score_text, 36)) / 2, screen_height / 2, 36, text_color);
        DrawText("press R/M to restart/menu", (screen_width - MeasureText("press R/M to restart/menu", 18)) / 2, screen_height - 22, 18, text_color);
        if (IsKeyReleased(KEY_R)) {
            if (new_high) highest = score;
            init_gameplay();
            game_scene = GS_GAME;
        } else if (IsKeyReleased(KEY_M)) {
            if (new_high) highest = score;
            game_scene = GS_MENU;
        }
    } break;
    case GS_CREDITS: {
        if (IsKeyPressed(KEY_ESCAPE)) game_scene = GS_MENU;
        ClearBackground(DARKGREEN);
        DrawText("Created by",(screen_width - MeasureText("Created by", 48)) / 2, screen_height / 2 - 80, 48, BLACK);
        DrawText("AmirMohammad Bandari",(screen_width - MeasureText("AmirMohammad Bandari", 48)) / 2, screen_height / 2 - 30, 48, BLACK);
        DrawText("https://github.com/AMBandariM",(screen_width - MeasureText("https://github.com/AMBandariM", 36)) / 2, screen_height / 2 + 20, 36, BLACK);
        if (IsKeyReleased(KEY_SPACE)) game_scene = GS_MENU;
    } break;
    default: TraceLog(LOG_FATAL, "Unreachable!");
    }
    EndDrawing();
}

// ---------------------------------------------------------------------------------------------
//     MAIN FUNCTION
// ---------------------------------------------------------------------------------------------

int main(void) {
#ifndef DEBUG
    SetTraceLogLevel(LOG_NONE);
#endif
    InitWindow(screen_width, screen_height, "Rayroids");
    InitAudioDevice();
    init_global_variables();
    if (!IsWindowFullscreen()) ToggleFullscreen();
    SetExitKey(KEY_NULL);
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
    HideCursor();

#ifdef PLATFORM_WEB
    raylib_js_set_frame(game_frame);
#else
    while (!WindowShouldClose() && running) {
        game_frame();
    }
#endif

    SaveFileData("data.bin", &highest, sizeof highest);
    UnloadShader(space_shader);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
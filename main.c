#include <raylib.h>
#include <raymath.h>

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
    unsigned char size;  // at most 8
    Vector2  center;
    Vector2  nodes[8];   // number of nodes is specified by size
    Vector2  speed;
    float    angular_speed;
    bool     is_new;     // ignore screen limits and don't make shadows if it's new
} Meteor;

int main(void) {
    // general state
    int screen_width = 800, screen_height = 600;
    GameScene game_scene = GS_MENU;
    // menu state
    MenuState menu_state = MS_START_NEW_GAME;
    // game state
    Vector2 ship_pos = {0}; const float speed = 200.0f;
    float ship_angle = 0.0f; const float angular_speed = 0.2f; const float angular_speed_inplace_add = 0.3f;
    float shooter_timer = 0.0f; float shoot_timer = 0.0f; const float shoot_cooldown = 0.3f;
    #define max_n_meteors 64
    int n_meteors = 0; Meteor meteors[max_n_meteors] = {0};
    float meteor_timer = 0.0f; const float meteor_cooldown = 10.0f;
    int score = 0, highest = 0, hp = 1;
    int read_n; char *read_data = LoadFileData("data.bin", &read_n); if (read_n) highest = *(int *)read_data; UnloadFileData(read_data);

    SetTraceLogLevel(LOG_FATAL);
    InitWindow(screen_width, screen_height, "Rayroids");
    if (!IsWindowFullscreen()) ToggleFullscreen();
    SetExitKey(KEY_NULL);
    Shader space_shader = LoadShader(0, "shaders/space.fs");
    SetTargetFPS(120);
    HideCursor();
    bool running = true;
    while (!WindowShouldClose() && running) {
        screen_width = GetRenderWidth(); screen_height = GetRenderHeight();
        BeginDrawing();
        switch (game_scene) {
        case GS_MENU: {
            if (IsKeyPressed(KEY_ESCAPE)) running = false;
            float t = (float)GetTime(); SetShaderValue(space_shader, GetShaderLocation(space_shader, "time"), &t, SHADER_UNIFORM_FLOAT);
            BeginShaderMode(space_shader); DrawRectangle(-10, -10, 4000, 4000, BLACK); EndShaderMode(); // ClearBackground(DARKBLUE);

            for (int i = 0; i < MS_N; ++i) DrawText(menu_state_names[i], 100, 100 + 50 * i, 48, menu_state == i ? IsKeyDown(KEY_SPACE)? GREEN : RED : DARKBROWN);
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menu_state = (menu_state + 1) % MS_N;
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) menu_state = (menu_state - 1 + MS_N) % MS_N;
            if (IsKeyReleased(KEY_SPACE)) switch (menu_state) {
                case MS_START_NEW_GAME: {
                    ship_pos = (Vector2){ .x = (float)screen_width / 2.0f, .y = (float)screen_height / 2.0f };
                    shoot_timer = meteor_timer = ship_angle = 0.0f; score = 0; n_meteors = 0; hp = 1;
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
            BeginShaderMode(space_shader); DrawRectangle(-10, -10, 4000, 4000, BLACK); EndShaderMode(); // ClearBackground(DARKBLUE);
            float dt = GetFrameTime();
            // player rotation
            float rotation_dir = 0.0f;
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) rotation_dir = 1.0f;
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) rotation_dir = -1.0f;
            if (shooter_timer > EPSILON) rotation_dir = 0.0f;
            ship_angle += rotation_dir * dt * (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W) ? angular_speed : angular_speed + angular_speed_inplace_add) * 2 * PI;
            // player move and shoot
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) ship_pos = Vector2Add(ship_pos, Vector2Rotate((Vector2){ .x = speed * dt, .y = 0.0f }, ship_angle));
            if (IsKeyDown(KEY_SPACE) && shooter_timer < EPSILON) {
                shooter_timer = shoot_timer = shoot_cooldown;
            }
            // player screen limit
            if (ship_pos.x < 0.0f) ship_pos.x += screen_width;
            else if (ship_pos.x >= screen_width) ship_pos.x -= screen_width;
            if (ship_pos.y < 0.0f) ship_pos.y += screen_height;
            else if (ship_pos.y >= screen_height) ship_pos.y -= screen_height;
            // meteor creation
            if (meteor_timer < EPSILON) {
                if (n_meteors < max_n_meteors) {
                    meteors[n_meteors].size = 8;
                    float theta = 2.0 * PI * GetRandomValue(0, 314) / 314;
                    meteors[n_meteors].center = Vector2Add((Vector2){ .x = (float)screen_width / 2.0f, (float)screen_height / 2.0f }, Vector2Rotate((Vector2){ .x = (float)screen_width / 2.0f + (float)screen_height / 2.0f, .y = 0.0f }, theta));
                    for (int i = 0; i < 8; ++i) meteors[n_meteors].nodes[i] = Vector2Rotate((Vector2){ .x = 1.0f * GetRandomValue(25 + 3 * 8, 40 + 3 * 8), .y = 0.0f }, 2.0f * PI * i / 8);
                    meteors[n_meteors].speed = Vector2Rotate((Vector2){ .x = 100.0, .y = 0}, theta + PI);
                    meteors[n_meteors].angular_speed = GetRandomValue(-10, 10) / 10.0;
                    meteors[n_meteors].is_new = true;
                    n_meteors++;
                }
                int tmp = 0, tmpscr = score; while (tmpscr) { tmp++; tmpscr /= 10; }
                meteor_timer += meteor_cooldown / (tmp + 1);
            }
            // meteor move
            for (int i = 0; i < n_meteors; ++i) {
                meteors[i].center = Vector2Add(meteors[i].center, Vector2Scale(meteors[i].speed, dt));
                for (int j = 0; j < meteors[i].size; ++j) meteors[i].nodes[j] = Vector2Rotate(meteors[i].nodes[j], meteors[i].angular_speed * dt);
            }
            // meteor screen limit
            for (int i = 0; i < n_meteors; ++i) {
                float mix = meteors[i].center.x, miy = meteors[i].center.y;
                if (meteors[i].is_new && mix > 60.0f && mix < screen_width - 60.0f && miy > 60.0f && miy < screen_height - 60.0f) meteors[i].is_new = false;
                if (!meteors[i].is_new) {
                    if (mix < 0.0f) meteors[i].center.x += screen_width;
                    else if (mix >= screen_width) meteors[i].center.x -= screen_width;
                    if (miy < 0.0f) meteors[i].center.y += screen_height;
                    else if (miy >= screen_height) meteors[i].center.y -= screen_height;
                }
            }
            // timers
            shoot_timer -= dt; if (shoot_timer < EPSILON) shoot_timer = 0.0f;
            shooter_timer -= dt; if (shooter_timer < EPSILON) shooter_timer = 0.0f;
            meteor_timer -= dt; if (meteor_timer < EPSILON) meteor_timer = 0.0f;
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
                if (shoot_timer > EPSILON) {
                    float t = (shoot_cooldown - shoot_timer) / shoot_cooldown;
                    Vector2 lead = Vector2Add(Vector2Add(ship_pos, d), Vector2Rotate((Vector2){ .x = 40.0f + 480.0f * t, .y = 0.0f }, ship_angle));
                    DrawLineEx(Vector2Add(Vector2Add(ship_pos, d), Vector2Rotate((Vector2){ .x = 40.0f + 480.0f * t * t, .y = 0.0f }, ship_angle)), lead, 4.0f, RAYWHITE);
                    // meteor physics
                    for (int i = 0; i < n_meteors; ++i) if (CheckCollisionPointPoly(Vector2Subtract(lead, meteors[i].center), meteors[i].nodes, meteors[i].size)) {
                        if ((dx != 0 || dy != 0) && meteors[i].is_new) continue;
                        Meteor tmp = meteors[i];
                        meteors[i] = meteors[n_meteors - 1];
                        if (tmp.size > 5) {
                            for (int j = n_meteors - 1; j <= n_meteors && j < max_n_meteors; ++j) {
                                meteors[j] = tmp; meteors[j].size--; meteors[j].is_new = false;
                                for (int k = 0; k < meteors[j].size; ++k) meteors[j].nodes[k] = Vector2Rotate((Vector2){ .x = 1.0f * GetRandomValue(25 + 3 * meteors[j].size, 40 + 3 * meteors[j].size), .y = 0.0f }, 2.0f * PI * k / meteors[j].size);
                                meteors[j].speed = Vector2Rotate(meteors[j].speed, 1.0 * (2 * (j - n_meteors) + 1));
                                meteors[j].angular_speed = GetRandomValue(-10, 10) / 10.0;
                            }
                            if (n_meteors < max_n_meteors - 1) n_meteors++;
                        }
                        else n_meteors--;
                        score++; shoot_timer = 0.0f; break;
                    }
                }
                // meteor graphics
                for (int i = 0; i < n_meteors; ++i) if (!meteors[i].is_new) for (int j = 0; j < meteors[i].size; ++j) DrawLineEx(Vector2Add(d, Vector2Add(meteors[i].center, meteors[i].nodes[j])), Vector2Add(d, Vector2Add(meteors[i].center, meteors[i].nodes[(j + 1) % meteors[i].size])), 2.0f, RAYWHITE);
            }
            // meteor graphics cont.
            for (int i = 0; i < n_meteors; ++i) if (meteors[i].is_new) for (int j = 0; j < meteors[i].size; ++j) DrawLineEx(Vector2Add(meteors[i].center, meteors[i].nodes[j]), Vector2Add(meteors[i].center, meteors[i].nodes[(j + 1) % meteors[i].size]), 2.0f, RAYWHITE);
            // texts
            DrawText(TextFormat("SCORE :: %08d / %08d", score, highest), 20, 20, 20, GRAY);
            DrawText(TextFormat("FPS :: %d", GetFPS()), 20, screen_height - 40, 20, GRAY);
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
                if (score > highest) highest = score;
                ship_pos = (Vector2){ .x = (float)screen_width / 2.0f, .y = (float)screen_height / 2.0f };
                shoot_timer = meteor_timer = ship_angle = 0.0f; score = 0; n_meteors = 0; hp = 1;
                game_scene = GS_GAME;
            } else if (IsKeyReleased(KEY_M)) game_scene = GS_MENU;
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
    SaveFileData("data.bin", &highest, sizeof highest);
    UnloadShader(space_shader);
    CloseWindow();
    return 0;
}
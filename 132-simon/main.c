#include <stdio.h>
#include <math.h>
#include <raylib.h>
#include "vec.h"

typedef int* vec_int;

enum MoveState {
    SmBegin,
    SmWaiting,
    SmShowPressed,
    SmSpace,
    SmChangeSide,
    SmMiss,
    SmFail,
};

struct GameState {
    vec_int sequence;
    int lives;
    bool players_move;
    bool level_up;
    enum MoveState move_state;
    int timer;
    int index;
};

struct Assets {
    Sound begin;
    Sound correct[4];
    Sound succeed;
    Sound miss;
    Sound fail;
};

double clamp(double x, double lower, double upper) {
    return min(upper, max(x, lower));
}

double repeat(double t, double length) {
    return clamp(t - floor(t / length) * length, 0.0f, length);
}

double ping_pong(double t, double length) {
    t = repeat(t, length * 2);
    return length - fabs(t - length);
}

int main() {

    const int screen_width = 800;
    const int screen_height = 450;

    const int triangle_width = 50;
    const int triangle_height = 60;

    const int life_radius = 30;

    const Vector2 up_v1 = {  ((float)screen_width / 2) - ((float)triangle_width / 2), ((float)screen_height / 2) - ((float)triangle_height / 2) };
    const Vector2 up_v2 = {  ((float)screen_width / 2) + ((float)triangle_width / 2), ((float)screen_height / 2) - ((float)triangle_height / 2) };
    const Vector2 up_v3 = {  ((float)screen_width / 2), ((float)screen_height / 2) - ((float)triangle_height * 1.5f) };

    const Vector2 right_v1 = {  ((float)screen_width / 2) + ((float)triangle_width / 2), ((float)screen_height / 2) + ((float)triangle_width / 2) };
    const Vector2 right_v2 = {  ((float)screen_width / 2) + ((float)triangle_height * 1.5f), ((float)screen_height / 2) };
    const Vector2 right_v3 = {  ((float)screen_width / 2) + ((float)triangle_width / 2), ((float)screen_height / 2) - ((float)triangle_width / 2) };

    const Vector2 down_v1 = {  ((float)screen_width / 2) - ((float)triangle_width / 2), ((float)screen_height / 2) + ((float)triangle_height / 2) };
    const Vector2 down_v2 = {  ((float)screen_width / 2) + ((float)triangle_width / 2), ((float)screen_height / 2) + ((float)triangle_height / 2) };
    const Vector2 down_v3 = {  ((float)screen_width / 2), ((float)screen_height / 2) + ((float)triangle_height * 1.5f) };

    const Vector2 left_v1 = {  ((float)screen_width / 2) - ((float)triangle_width / 2), ((float)screen_height / 2) + ((float)triangle_width / 2) };
    const Vector2 left_v2 = {  ((float)screen_width / 2) - ((float)triangle_height * 1.5f), ((float)screen_height / 2) };
    const Vector2 left_v3 = {  ((float)screen_width / 2) - ((float)triangle_width / 2), ((float)screen_height / 2) - ((float)triangle_width / 2) };

    struct GameState game;
    struct Assets assets;

    InitWindow(screen_width, screen_height, "Simon Says");

    InitAudioDevice();

    SetTargetFPS(60);

    const int begin_duration = 15;
    const int show_pressed_duration = 15;
    const int space_duration = 7;
    const int miss_duration = 30;
    const int level_up_duration = 30;

    game.sequence = vector_create();
    game.lives = 3;
    game.players_move = false;
    game.level_up = false;
    game.move_state = SmBegin;
    game.timer = begin_duration;
    game.index = 0;

    assets.begin = LoadSound("begin.wav");
    assets.correct[0] = LoadSound("0.wav");
    assets.correct[1] = LoadSound("1.wav");
    assets.correct[2] = LoadSound("2.wav");
    assets.correct[3] = LoadSound("3.wav");
    assets.succeed = LoadSound("succeed.wav");
    assets.miss = LoadSound("miss.wav");
    assets.fail = LoadSound("fail.wav");

    vector_add(&game.sequence, int, rand() % 4);// NOLINT(cert-msc50-cpp)

#ifdef DEBUG
    vec_int pressed_sequence = vector_create();
#endif

    int pressed = -1;
    while (!WindowShouldClose()) {
        int turn = vector_size(game.sequence);

        if (IsKeyPressed(KEY_SPACE)) PlaySound(assets.begin);

        // update timer
        if (game.timer > 0) game.timer--;

        switch (game.move_state) {
            case SmBegin:
                if (game.timer == begin_duration - 1) PlaySound(assets.begin);
                if (game.timer == 0) game.move_state = SmWaiting;
                break;
            case SmShowPressed:
                if (game.timer == 0) {
                    printf("players_move: %d, index: %d, turn: %d\n", game.players_move, game.index, turn);
                    game.timer = space_duration;

                    if (game.index == turn) {
                        game.players_move = !game.players_move;
                        vector_free(pressed_sequence);
                        pressed_sequence = vector_create();
                        if (!game.players_move) {
                            int next = (int)((rand() % 40) / 10.0f);
                            if (vector_size(game.sequence) >= 4) {
                                int group;
                                do {
                                    next = (int)((rand() % 40) / 10.0f);
                                    group = 0;
                                    for (vec_size_t i = vector_size(game.sequence) - 1; i--; i >= vector_size(game.sequence) - 4) {
                                        if (game.sequence[i] == next) group++;
                                    }
                                } while (group >= 2);
                            }
                            vector_add(&game.sequence, int, next);
                            game.timer = level_up_duration;
                        }
                        game.index = 0;
                    }
                    game.move_state = SmSpace;
                }
                break;
            case SmSpace:
                game.level_up = false;
            case SmMiss:
                if (game.timer == 0) game.move_state = SmWaiting;
                break;
            case SmFail:
            case SmWaiting:
            case SmChangeSide:
                break;
        }

        if (game.players_move) {
            if (game.move_state == SmFail) {
                if (IsKeyDown(KEY_R)) {
                    vector_free(game.sequence);
                    game.sequence = vector_create();
                    game.move_state = SmWaiting;
                    game.lives = 3;
                    game.timer = 0;
                    game.index = 0;
                    game.players_move = false;

                    vector_free(pressed_sequence);
                    pressed_sequence = vector_create();
                    continue;
                } else if (IsKeyDown(KEY_ESCAPE)) {
                    CloseWindow();
                    continue;
                }
            }
            if (game.move_state == SmWaiting) {
                if (IsKeyDown(KEY_UP)) pressed = 0;
                if (IsKeyDown(KEY_LEFT)) pressed = 1;
                if (IsKeyDown(KEY_RIGHT)) pressed = 2;
                if (IsKeyDown(KEY_DOWN)) pressed = 3;

                if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_DOWN)) {
                    int item = game.sequence[game.index];
#if DEBUG
                    vector_add(&pressed_sequence, int, pressed);
#endif
                    if (item != pressed) {
                        // they got it wrong, so make it a bit easier for them by removing the last item
                        if (turn > 1) {
                            vector_remove(game.sequence, int, turn - 1);
                        }
                        game.lives--;
                        if (game.lives == 0) {
                            PlaySound(assets.fail);
                            game.move_state = SmFail;
                        } else {
                            PlaySound(assets.miss);
                            game.move_state = SmMiss;
                            game.timer = miss_duration;
                            game.players_move = false;
                        }
                        game.index = 0;
                        vector_free(pressed_sequence);
                        pressed_sequence = vector_create();
                    }
                    else {
                        game.index++;
                        game.move_state = SmShowPressed;
                        game.timer = show_pressed_duration;
                        if (game.index == turn) {
                            PlaySound(assets.succeed);
                            game.level_up = true;
                        }
                        else PlaySound(assets.correct[pressed]);
                    }
                }
            }
        } else {
            // get next in sequence
            if (game.move_state == SmWaiting) {
                pressed = game.sequence[game.index];
                PlaySound(assets.correct[pressed]);
                game.move_state = SmShowPressed;
                game.timer = show_pressed_duration;
                game.index++;
            }
        }

        // draw
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawTriangleLines(up_v1, up_v2, up_v3, BLUE);
            DrawTriangleLines(left_v1, left_v2, left_v3, MAGENTA);
            DrawTriangleLines(right_v1, right_v2, right_v3, YELLOW);
            DrawTriangleLines(down_v1, down_v2, down_v3, BLACK);

            // draw the four sides
            if (game.move_state == SmShowPressed) {
                if (pressed == 0) DrawTriangle(up_v1, up_v2, up_v3, BLUE);
                if (pressed == 1) DrawTriangle(left_v1, left_v3, left_v2, MAGENTA);
                if (pressed == 2) DrawTriangle(right_v1, right_v2, right_v3, YELLOW);
                if (pressed == 3) DrawTriangle(down_v1, down_v3, down_v2, BLACK);
            }

#if DEBUG
            DrawText("current sequence: ", 10, 10, 12, BLACK);
            for (int i = 0; i < vector_size(game.sequence); i++) {
                char text[8];
                sprintf(text, "%d", game.sequence[i]);
                DrawText(text, 128 + (i * 10), 10, 12, BLACK);
            }

            DrawText("pressed sequence: ", 10, 40, 12, BLACK);
            for (int i = 0; i < vector_size(pressed_sequence); i++) {
                char text[8];
                sprintf(text, "%d", pressed_sequence[i]);
                DrawText(text, 128 + (i * 10), 40, 12, BLACK);
            }
#endif
            if (game.move_state == SmMiss) {
                const char* level_down_text = "Level Down";

                int size = 30;
                int offset = MeasureText(level_down_text, size) / 2;

                DrawText(level_down_text, screen_width / 2 - offset, screen_height / 7, size, RED);
            }

            if (game.level_up) {
                const char* level_down_text = "Level Up!";

                int size = 30;
                int offset = MeasureText(level_down_text, size) / 2;

                DrawText(level_down_text, screen_width / 2 - offset, screen_height / 7, size, BLUE);
            }

            if (game.move_state == SmFail) {
                const char* you_failed_text = "You Failed";
                const char* retry_text = "Press R to Retry or Escape to Quit";

                float bounce = (float)ping_pong(GetTime() * 3.0f, 6.0f);

                float fail_size = 30.0f + bounce;
                float retry_size = 12.0f + (bounce * 12.0f / 30.0f);

                float fail_text_offset = MeasureTextEx(GetFontDefault(), you_failed_text, fail_size, fail_size / 10.0f).x / 2.0f;
                float retry_text_offset = MeasureTextEx(GetFontDefault(), retry_text, retry_size, retry_size / 10.0f).x / 2.0f;

                // void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint);
                DrawTextEx(GetFontDefault(), you_failed_text, (Vector2){ .x = screen_width / 2 - fail_text_offset, .y = screen_height / 6 }, fail_size, fail_size / 10.0f, MAROON);
                DrawTextEx(GetFontDefault(), retry_text, (Vector2){ screen_width / 2 - retry_text_offset, screen_height / 6 + fail_size }, retry_size, retry_size / 10.0f, BLACK);
            }

            for (int i = 0; i < 3; i++) {
                int radius_with_padding = life_radius + 10;
                DrawCircleLines(screen_width - radius_with_padding * (i * 2 + 1), radius_with_padding, (float)life_radius, RED);
                if (i < game.lives) DrawCircle(screen_width - radius_with_padding * (i * 2 + 1), radius_with_padding, (float)life_radius, RED);
            }

        EndDrawing();
    }

    vector_free(game.sequence);
    vector_free(pressed_sequence);

    CloseAudioDevice();

    CloseWindow();
    return 0;
}

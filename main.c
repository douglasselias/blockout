#include "raylib.h"
#include "raymath.h"

void draw_texture(Texture2D texture, Rectangle dest) {
  DrawTexturePro(texture, (Rectangle){0, 0, texture.width, texture.height},
                 dest, (Vector2){0, 0}, 0, WHITE);
}

int main() {
  SetTraceLogLevel(LOG_WARNING);
  int screen_width = 1280;
  int screen_height = 720;
  int half_screen_width = screen_width / 2;
  int half_screen_height = screen_height / 2;
  InitWindow(screen_width, screen_height, "Blockout");
  SetTargetFPS(60);
  SetRandomSeed(GetTime());
  InitAudioDevice();

  Texture2D heart_texture = LoadTexture("assets/heart.png");

  Texture2D block_texture = LoadTexture("assets/block.png");

  Texture2D ball_texture = LoadTexture("assets/ball.png");
  Vector2 ball_position = {110, 0};
  Vector2 ball_direction = {1, 0};
  int ball_speed = 10;
  int ball_size = 30;

  Texture2D happy_face_texture = LoadTexture("assets/happy_face.png");

  Texture2D paddle_texture = LoadTexture("assets/paddle.png");
  float paddle_scale = 4.0;
  Vector2 paddle_position = {
      half_screen_width - ((paddle_texture.width / paddle_scale) / 2.0),
      screen_height - paddle_texture.height / paddle_scale};
  int paddle_speed = 20;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    if (IsKeyDown(KEY_A)) {
      paddle_position.x -= paddle_speed;
    }
    if (IsKeyDown(KEY_D)) {
      paddle_position.x += paddle_speed;
    }
    paddle_position.x =
        Clamp(paddle_position.x, 0,
              screen_width - paddle_texture.width / paddle_scale);

    if (ball_position.x + ball_size > screen_width) {
      ball_direction.x *= -1;
    } else if (ball_position.x < 0) {
      ball_direction.x *= -1;
    }
    ball_position.x += ball_direction.x * ball_speed;

    BeginDrawing();
    ClearBackground(GOLD);

    DrawTexture(heart_texture, 0, 0, WHITE);
    DrawTexture(block_texture, 400, 0, WHITE);

    // DrawTextureV(paddle_texture, paddle_position, WHITE);
    draw_texture(paddle_texture,
                 (Rectangle){paddle_position.x, paddle_position.y,
                             paddle_texture.width / paddle_scale,
                             paddle_texture.height / paddle_scale});
    DrawTexture(happy_face_texture,
                paddle_position.x + (paddle_texture.width / 2.0) -
                    (happy_face_texture.width / 2.0),
                paddle_position.y, WHITE);

    draw_texture(ball_texture, (Rectangle){ball_position.x, ball_position.y,
                                           ball_size, ball_size});
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
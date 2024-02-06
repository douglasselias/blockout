#include "raylib.h"
#include "raymath.h"

const int screen_width = 1280;
const int screen_height = 720;
int half_screen_width = screen_width / 2;
int half_screen_height = screen_height / 2;

typedef enum {
  how_to_play,
  to_serve,
  playing,
  game_over,
} GameState;

void populate_blocks(Rectangle *blocks, int total_blocks) {
  float block_width = 70;
  float block_height = 30;
  float all_blocks_width = block_width * 10;
  float all_blocks_height = block_height * 3;
  float left_offset = half_screen_width - (all_blocks_width / 2.0);
  float top_offset = 30;
  int column = 0;
  int line = 0;

  for (int i = 0; i < total_blocks; i++) {
    blocks[i] = (Rectangle){left_offset + (column * block_width),
                            top_offset + (line * block_height), block_width,
                            block_height};
    column++;
    if (column == 10) {
      column = 0;
      line++;
    }
  }
}

void draw_texture(Texture2D texture, Rectangle dest) {
  DrawTexturePro(texture, (Rectangle){0, 0, texture.width, texture.height},
                 dest, (Vector2){0, 0}, 0, WHITE);
}

int main() {
  SetTraceLogLevel(LOG_WARNING);
  InitWindow(screen_width, screen_height, "Blockout");
  SetTargetFPS(60);
  SetRandomSeed(GetTime());
  InitAudioDevice();

  Music bgm = LoadMusicStream("assets/bgm.wav");
  SetMusicVolume(bgm, 0.5);

  Sound collision_sfx = LoadSound("assets/collision.ogg");
  Sound lose_sfx = LoadSound("assets/lose.wav");
  Sound hurt_sfx = LoadSound("assets/hurt.wav");

  Texture2D mouse_texture = LoadTexture("assets/mouse_horizontal.png");
  Texture2D mouse_left_texture = LoadTexture("assets/mouse_left.png");

  int hearts = 3;
  Texture2D heart_texture = LoadTexture("assets/heart.png");
  Vector2 heart_position = {0, 0};
  float heart_scale = 4.0;

#define total_blocks 40
  Rectangle blocks[total_blocks] = {};
  populate_blocks(blocks, total_blocks);

  Texture2D block_texture = LoadTexture("assets/block.png");
  Rectangle block_rect = {300, 0, 70, 30};

  Texture2D sad_face_texture = LoadTexture("assets/faces/sad.png");
  Texture2D dead_face_texture = LoadTexture("assets/faces/dead.png");
  Texture2D happy_face_texture = LoadTexture("assets/faces/happy.png");

  Texture2D pupil_texture = LoadTexture("assets/faces/pupil.png");
  Texture2D neutral_face_texture =
      LoadTexture("assets/faces/template_neutral.png");
  float neutral_face_scale = 2.0;
  Texture2D current_face = neutral_face_texture;
  int neutral_texture_id = neutral_face_texture.id;
  int face_timeout = 0;

  Texture2D paddle_texture = LoadTexture("assets/paddle.png");
  float paddle_scale = 4.0;
  float paddle_midpoint = ((paddle_texture.width / paddle_scale) / 2.0);
  Vector2 paddle_position = {half_screen_width - paddle_midpoint,
                             screen_height -
                                 paddle_texture.height / paddle_scale};
  int paddle_speed = 20;
  float paddle_edge = 30;

#define total_particles 50
  Vector2 particles[total_particles] = {};
  Texture2D particle_texture = LoadTexture("assets/particle.png");
  int particles_index = 0;

  int ball_size = 30;
  int ball_radius = ball_size / 2;
  Texture2D ball_texture = LoadTexture("assets/ball.png");
  Vector2 ball_position = {paddle_position.x + paddle_midpoint,
                           paddle_position.y - ball_radius};
  Vector2 ball_direction = {0, 0};
  int ball_speed = 10;

  GameState game_state = how_to_play;
  bool hit_paddle_last_frame = false;
  bool hit_block_last_frame = false;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    UpdateMusicStream(bgm);

    Vector2 mouse_position = GetMousePosition();
    if (game_state != game_over)
      paddle_position.x = mouse_position.x;

    if (IsMouseButtonPressed(0)) {
      if (game_state == how_to_play) {
        PlayMusicStream(bgm);
      }
      if (game_state == how_to_play || game_state == to_serve) {
        game_state = playing;
        int range = GetRandomValue(-15, 15);
        ball_direction = (Vector2){cos((90 + range) * DEG2RAD), -1};
      }
      if (game_state == game_over) {
        game_state = to_serve;
        hearts = 3;
        paddle_position =
            (Vector2){half_screen_width - paddle_midpoint,
                      screen_height - paddle_texture.height / paddle_scale};
        ball_position = (Vector2){paddle_position.x + paddle_midpoint,
                                  paddle_position.y - ball_radius};
        populate_blocks(blocks, total_blocks);
        PlayMusicStream(bgm);
        current_face = neutral_face_texture;
        ball_speed = 5;
      }
    }

    paddle_position.x =
        Clamp(paddle_position.x, 0,
              screen_width - paddle_texture.width / paddle_scale);

    Rectangle paddle_rect = {paddle_position.x, paddle_position.y,
                             paddle_texture.width / paddle_scale,
                             paddle_texture.height / paddle_scale};

    if (ball_position.x + ball_radius > screen_width) {
      ball_direction.x *= -1;
      PlaySound(collision_sfx);
    } else if (ball_position.x - ball_radius < 0) {
      ball_direction.x *= -1;
      PlaySound(collision_sfx);
    }
    if (ball_position.y + ball_radius > screen_height) {
      ball_direction.y *= -1;
      hearts--;
      current_face = sad_face_texture;
      face_timeout = 30;
      PlaySound(hurt_sfx);
      if (hearts == 0) {
        PlaySound(lose_sfx);
      }
      for (int i = 0; i < total_particles; i++) {
        particles[i] = (Vector2){-100, -100};
      }
      game_state = to_serve;
    } else if (ball_position.y - ball_radius < 0) {
      ball_direction.y *= -1;
      PlaySound(collision_sfx);
    }

    if (CheckCollisionCircleRec(ball_position, ball_radius, paddle_rect)) {
      PlaySound(collision_sfx);
      if (!hit_paddle_last_frame) {
        hit_paddle_last_frame = true;
        ball_speed *= 1.09;

        if (paddle_position.x - ball_radius < ball_position.x &&
            ball_position.x < paddle_position.x + paddle_edge) {
          // TraceLog(LOG_WARNING, "Hit left corner");
          ball_direction.y *= -1;
          int range = GetRandomValue(-15, 15);
          ball_direction.x = cos(((105 + range) * DEG2RAD));
        } else if (paddle_position.x + paddle_edge < ball_position.x &&
                   ball_position.x < paddle_position.x + (paddle_midpoint * 2) -
                                         paddle_edge) {
          // TraceLog(LOG_WARNING, "Hit center");
          ball_direction.y *= -1;
        } else if (paddle_position.x + (paddle_midpoint * 2) - paddle_edge <
                       ball_position.x &&
                   ball_position.x < paddle_position.x + (paddle_midpoint * 2) +
                                         ball_radius) {
          // TraceLog(LOG_WARNING, "Hit right corner");
          ball_direction.y *= -1;
          int range = GetRandomValue(-15, 15);
          ball_direction.x = cos(((45 + range) * DEG2RAD));
        } else if (ball_position.y > paddle_position.y) {
          // TraceLog(LOG_WARNING, "Hit side");
          ball_direction.x *= -1;
        } else {
          // TraceLog(LOG_WARNING, "Hit but...");
        }
      }
    } else {
      hit_paddle_last_frame = false;
    }

    for (int i = 0; i < total_blocks; i++) {
      if (CheckCollisionCircleRec(ball_position, ball_radius, blocks[i])) {
        if (!hit_block_last_frame) {
          hit_block_last_frame = true;

          /// @todo: maybe someday do a better collision detection
          /// it would need some way to detect gaps to act as
          /// individual blocks or whole blocks
          // Rectangle block = blocks[i];
          // if (block.x - ball_radius < ball_position.x &&
          //     ball_position.x < block.x + paddle_edge) {
          //   TraceLog(LOG_WARNING, "Hit left corner");
          //   ball_direction.y *= -1;
          //   int range = GetRandomValue(-15, 15);
          //   ball_direction.x = cos(((105 + range) * DEG2RAD));
          // } else if (block.x + paddle_edge < ball_position.x &&
          //            ball_position.x < block.x + block.width - paddle_edge) {
          //   TraceLog(LOG_WARNING, "Hit center");
          //   ball_direction.y *= -1;
          // } else if (block.x + block.width - paddle_edge < ball_position.x &&
          //            ball_position.x < block.x + block.width + ball_radius) {
          //   TraceLog(LOG_WARNING, "Hit right corner");
          //   ball_direction.y *= -1;
          //   int range = GetRandomValue(-15, 15);
          //   ball_direction.x = cos(((45 + range) * DEG2RAD));
          // } else if (ball_position.y > block.y) {
          //   TraceLog(LOG_WARNING, "Hit side");
          //   ball_direction.x *= -1;
          // } else {
          //   TraceLog(LOG_WARNING, "Hit but...");
          // }

          ball_direction.y *= -1;
          current_face = happy_face_texture;
          face_timeout = 30;
          PlaySound(collision_sfx);
          blocks[i] = (Rectangle){0, 0, 0, 0};
        }
      } else {
        hit_block_last_frame = false;
      }
    }

    if (game_state == how_to_play || game_state == to_serve) {
      ball_position.x = paddle_position.x + paddle_midpoint;
      ball_position.y = paddle_position.y - ball_radius;
    }

    if (game_state == playing) {
      particles[particles_index] = ball_position;
      particles_index++;
      if (particles_index == total_particles) {
        particles_index = 0;
      }
      ball_position.x += ball_direction.x * ball_speed;
      ball_position.y += ball_direction.y * ball_speed;
    }

    if (hearts == 0) {
      current_face = dead_face_texture;
      game_state = game_over;
      StopMusicStream(bgm);
    } else if (face_timeout > 0) {
      face_timeout--;
    } else if (face_timeout == 0) {
      current_face = neutral_face_texture;
    }

    BeginDrawing();
    ClearBackground(GOLD);

    for (int i = 0; i < hearts; i++) {
      float left_offset = 20;
      float top_offset = 10;
      draw_texture(heart_texture,
                   (Rectangle){(heart_position.x + left_offset) + (i * 40),
                               heart_position.y + top_offset,
                               heart_texture.width / heart_scale,
                               heart_texture.height / heart_scale});
    }

    for (int i = 0; i < total_blocks; i++) {
      draw_texture(block_texture, blocks[i]);
    }

    draw_texture(paddle_texture, paddle_rect);

    draw_texture(
        current_face,
        (Rectangle){
            paddle_position.x + ((paddle_texture.width / paddle_scale) / 2.0) -
                ((neutral_face_texture.width / neutral_face_scale) / 2.0),
            paddle_position.y,
            neutral_face_texture.width / neutral_face_scale,
            neutral_face_texture.height / neutral_face_scale,
        });

    if (current_face.id == neutral_texture_id) {
      Vector2 left_eye_position = {paddle_position.x + paddle_midpoint - 20,
                                   paddle_position.y + 6};
      Vector2 left_pupil_position =
          Vector2Subtract(ball_position, left_eye_position);
      left_pupil_position = Vector2Normalize(left_pupil_position);
      left_pupil_position.x = left_pupil_position.x * 6;
      left_pupil_position.y = left_pupil_position.y * 6;

      draw_texture(pupil_texture,
                   (Rectangle){left_eye_position.x + left_pupil_position.x,
                               left_eye_position.y + left_pupil_position.y, 10,
                               10});

      Vector2 right_eye_position = {paddle_position.x + paddle_midpoint + 10,
                                    paddle_position.y + 6};
      Vector2 right_pupil_position =
          Vector2Subtract(ball_position, right_eye_position);
      right_pupil_position = Vector2Normalize(right_pupil_position);
      right_pupil_position.x = right_pupil_position.x * 6;
      right_pupil_position.y = right_pupil_position.y * 6;
      draw_texture(pupil_texture,
                   (Rectangle){right_eye_position.x + right_pupil_position.x,
                               right_eye_position.y + right_pupil_position.y,
                               10, 10});
    }

    for (int i = 0; i < total_particles; i++) {
      draw_texture(particle_texture, (Rectangle){particles[i].x - ball_radius,
                                                 particles[i].y - ball_radius,
                                                 ball_size, ball_size});
    }

    draw_texture(ball_texture, (Rectangle){ball_position.x - ball_radius,
                                           ball_position.y - ball_radius,
                                           ball_size, ball_size});

    if (game_state == how_to_play) {
      DrawText("Move", half_screen_width - 120, half_screen_height - 80, 34,
               RED);
      DrawTexture(mouse_texture, half_screen_width - 140,
                  half_screen_height - 50, WHITE);

      DrawText("Serve", half_screen_width + 100, half_screen_height - 80, 34,
               RED);
      draw_texture(mouse_left_texture,
                   (Rectangle){half_screen_width + 100, half_screen_height - 27,
                               mouse_texture.width / 1.5,
                               mouse_texture.height / 1.5});
    }

    if (game_state == game_over) {
      DrawText("Restart", half_screen_width - 70, half_screen_height - 80, 34,
               RED);
      draw_texture(mouse_left_texture,
                   (Rectangle){half_screen_width - (130 / 2.0),
                               half_screen_height - 30, 130, 130});
    }

    if (false) {
      DrawRectangleRec((Rectangle){paddle_position.x, paddle_position.y,
                                   paddle_edge,
                                   screen_height - paddle_position.y},
                       MAGENTA);
      DrawRectangleRec((Rectangle){paddle_position.x + paddle_edge,
                                   paddle_position.y,
                                   (paddle_midpoint * 2) - paddle_edge,
                                   screen_height - paddle_position.y},
                       PURPLE);
      DrawRectangleRec(
          (Rectangle){paddle_position.x + (paddle_midpoint * 2) - paddle_edge,
                      paddle_position.y, paddle_edge,
                      screen_height - paddle_position.y},
          MAGENTA);
      DrawCircleV(ball_position, ball_radius, LIME);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
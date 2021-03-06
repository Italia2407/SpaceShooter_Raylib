#include "raylib.h"
#include "easings.h"
#include "stdio.h"
#include "memory.h"
#include "TracyC.h"

#define PROFILE_BEGIN(name) TracyCZoneNS(tracy__var__ ## name, #name, 12, 1)
#define PROFILE_END(name) TracyCZoneEnd(tracy__var__ ## name)

#define SCALE (4)
#define SCREEN_WIDTH (256 * SCALE)
#define SCREEN_HEIGHT SCREEN_WIDTH

#define PLAYER_SPEED (5)
#define LASER_SPEED (10)

#define ENEMY_ANIM_DURATION (1)

typedef struct LaserBeam LaserBeam;
struct LaserBeam
{
    bool is_enemy_beam;
    bool left_active;
    bool right_active;
    int x;
    int y;
};

#define MAX_LASER_COUNT (99)
LaserBeam lasers[MAX_LASER_COUNT];

typedef struct Enemy Enemy;
struct Enemy
{
    bool active;
    int x;
    int y;

    int begin_x;
    int begin_y;

    int target_x;
    int target_y;

    float timer;
    float laser_beam_timer;
};

#define MAX_ENEMY_COUNT (10)
Enemy enemies[MAX_ENEMY_COUNT];

#define FONT_ANIM_DURATION (0.3f)
float font_anim_timer;
bool font_retracting;
bool font_anim_playing;

Texture player_texture;
Texture background_texture;
Texture player_laser_texture;
Texture enemy_texture;
Texture enemy_laser_texture;

int player_x;
int player_y;

int player_lives;
int score;

bool enemies_are_alive;

#define PLAYER_ANIM_FLASH_COUNT (3)
#define PLAYER_DAMAGE_FLASH_ANIM_DURATION (0.15f)

float player_damaged_anim_playing;
float player_damaged_anim_timer;
bool player_damaged_anim_retracting;
int player_anim_flash_counter;

void reset_game(void)
{
	PROFILE_BEGIN(reset);
	
    score = 0;
    player_lives = 3;

    memset(lasers, 0, sizeof(lasers));
    memset(enemies, 0, sizeof(enemies));

    // Set player position
    player_x = SCREEN_WIDTH  / 2 - player_texture.width  / 2;
    player_y = SCREEN_HEIGHT / 2 - player_texture.height / 2 - player_texture.height;

    // Set the enemies to random positions
    for (int i = 0; i < MAX_ENEMY_COUNT; i++)
    {
        enemies[i].active = true;
        enemies[i].x = enemies[i].begin_x = GetRandomValue(0, SCREEN_WIDTH - enemy_texture.width);
        enemies[i].y = enemies[i].begin_y = GetRandomValue(0, (SCREEN_HEIGHT / 2) - enemy_texture.height);

        enemies[i].target_x = GetRandomValue(0, SCREEN_WIDTH - enemy_texture.width);
        enemies[i].target_y = GetRandomValue(0, (SCREEN_HEIGHT / 2) - enemy_texture.height);
        enemies[i].laser_beam_timer = GetRandomValue(1, 5);
    }
    
    PROFILE_END(reset);
}

int main(void)
{
	// Initialisation
	PROFILE_BEGIN(game_init);
	{
		// Window creation
		PROFILE_BEGIN(window_init);
		{
			InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "DOOM Eternal");
			SetTargetFPS(60);
		}
		PROFILE_END(window_init);
		
		// Texture loading
		PROFILE_BEGIN(textures_load);
		{
			background_texture = LoadTexture(ASSET_PATH"background.png");
			player_texture = LoadTexture(ASSET_PATH"spaceship.png");
			player_laser_texture = LoadTexture(ASSET_PATH"player-laser.png");
			enemy_texture = LoadTexture(ASSET_PATH"tsoding.png");
			enemy_laser_texture = LoadTexture(ASSET_PATH"php.png");
		}
		PROFILE_END(textures_load);
		
		reset_game();
	}
	PROFILE_END(game_init);

    while (!WindowShouldClose())
    {
    	PROFILE_BEGIN(main_loop);
        // Update
        PROFILE_BEGIN(game_update);
        {
            // Update player position based on input
            if (IsKeyDown(KEY_A)) player_x -= PLAYER_SPEED;
            if (IsKeyDown(KEY_D)) player_x += PLAYER_SPEED;
            if (IsKeyDown(KEY_W)) player_y -= PLAYER_SPEED;
            if (IsKeyDown(KEY_S)) player_y += PLAYER_SPEED;

            // Check if player is in the game area
            if (player_y < SCREEN_WIDTH / 2) player_y = SCREEN_WIDTH / 2;
            if (player_x < 0) player_x = 0;
            if (player_x + player_texture.width > SCREEN_WIDTH) player_x = SCREEN_WIDTH - player_texture.width;
            if (player_y + player_texture.height > SCREEN_HEIGHT) player_y = SCREEN_HEIGHT - player_texture.height;

            if ((player_lives == 0 || !enemies_are_alive) && IsKeyPressed(KEY_ENTER))
            {
                reset_game();
            }

            // Shoot lasers
            if (IsKeyPressed(KEY_SPACE))
            {
                for (int i = 0; i < MAX_LASER_COUNT; i++)
                {
                    if (!lasers[i].left_active && !lasers[i].right_active)
                    {
                        lasers[i].is_enemy_beam = false;
                        lasers[i].left_active = true;
                        lasers[i].right_active = true;
                        lasers[i].x = player_x;
                        lasers[i].y = player_y;
                        break;
                    }
                }
            }

            // Update lasers
            for (int i = 0; i < MAX_LASER_COUNT; i++)
            {
                if ((lasers[i].left_active || lasers[i].right_active) && lasers[i].is_enemy_beam) // Enemy beam
                {
                    // Beam goes down
                    lasers[i].y += LASER_SPEED;

                    // Disable beam if its out of the screen
                    if (lasers[i].y > SCREEN_HEIGHT)
                    {
                        lasers[i].left_active  = false;
                        lasers[i].right_active = false;
                    }

                    Rectangle left_laser_collider = {
                        (float) lasers[i].x,
                        (float) lasers[i].y,
                        (float) player_laser_texture.width,
                        (float) player_laser_texture.height
                    };

                    Rectangle right_laser_collider = {
                        (float) (lasers[i].x + player_texture.width - player_laser_texture.width),
                        (float) lasers[i].y,
                        (float) player_laser_texture.width,
                        (float) player_laser_texture.height
                    };

                    Rectangle player_collider = { player_x, player_y, player_texture.width, player_texture.height };

                    if (lasers[i].left_active && CheckCollisionRecs(left_laser_collider, player_collider))
                    {
                        lasers[i].left_active = false;
                        if (player_lives > 0) player_lives--;
                        player_damaged_anim_playing = true;
                        player_anim_flash_counter = PLAYER_ANIM_FLASH_COUNT;
                    }

                    if (lasers[i].right_active && CheckCollisionRecs(right_laser_collider, player_collider))
                    {
                        lasers[i].right_active = false;
                        if (player_lives > 0) player_lives--;
                        player_damaged_anim_playing = true;
                        player_anim_flash_counter = PLAYER_ANIM_FLASH_COUNT;
                    }
                }
                else if ((lasers[i].left_active || lasers[i].right_active) && !lasers[i].is_enemy_beam) // Player beam
                {
                    // Beam goes up
                    lasers[i].y -= LASER_SPEED;

                    // Disable beam if its out of the screen
                    if (lasers[i].y + player_laser_texture.height < 0)
                    {
                        lasers[i].left_active = false;
                        lasers[i].right_active = false;
                    }
                    else // Check if laser beam reaps & tears an enemy
                    {
                        Rectangle left_laser_collider = {
                            (float) lasers[i].x,
                            (float) lasers[i].y,
                            (float) player_laser_texture.width,
                            (float) player_laser_texture.height
                        };

                        Rectangle right_laser_collider = {
                            (float) (lasers[i].x + player_texture.width - player_laser_texture.width),
                            (float) lasers[i].y,
                            (float) player_laser_texture.width,
                            (float) player_laser_texture.height
                        };

                        // Check collision with enemies
                        for (int j = 0; j < MAX_ENEMY_COUNT; j++)
                        {
                            if (enemies[j].active)
                            {
                                Rectangle enemy_collider = {
                                    (float) enemies[j].x,
                                    (float) enemies[j].y,
                                    (float) enemy_texture.width,
                                    (float) enemy_texture.height
                                };

                                if (lasers[i].left_active && CheckCollisionRecs(enemy_collider, left_laser_collider))
                                {
                                    enemies[j].active = false;
                                    lasers[i].left_active = false;
                                    score += 100;
                                    font_anim_playing = true;
                                }

                                if (lasers[i].right_active && CheckCollisionRecs(enemy_collider, right_laser_collider))
                                {
                                    enemies[j].active = false;
                                    lasers[i].right_active = false;
                                    score += 100;
                                    font_anim_playing = true;
                                }
                            }
                        }
                    }
                }
            }

            // Update enemies
            enemies_are_alive = false;
            for (int i = 0; i < MAX_ENEMY_COUNT; i++)
            {
                enemies_are_alive = enemies_are_alive || enemies[i].active;

                if (enemies[i].active)
                {
                    enemies[i].x = (int) EaseQuadInOut(enemies[i].timer, enemies[i].begin_x, enemies[i].target_x - enemies[i].begin_x, ENEMY_ANIM_DURATION);
                    enemies[i].y = (int) EaseQuadInOut(enemies[i].timer, enemies[i].begin_y, enemies[i].target_y - enemies[i].begin_y, ENEMY_ANIM_DURATION);
                    enemies[i].timer += GetFrameTime();
                    enemies[i].laser_beam_timer -= GetFrameTime();

                    if (enemies[i].timer >= ENEMY_ANIM_DURATION)
                    {
                        enemies[i].x = enemies[i].begin_x = enemies[i].target_x;
                        enemies[i].y = enemies[i].begin_y = enemies[i].target_y;

                        enemies[i].target_x = GetRandomValue(0, SCREEN_WIDTH - enemy_texture.width);
                        enemies[i].target_y = GetRandomValue(0, (SCREEN_HEIGHT / 2) - enemy_texture.height);

                        enemies[i].timer = 0;
                    }

                    // If the enemy decides to shoot
                    if (enemies[i].laser_beam_timer < 0)
                    {
                        for (int j = 0; j < MAX_LASER_COUNT; j++)
                        {
                            if (!lasers[j].left_active && !lasers[j].right_active)
                            {
                                lasers[j].is_enemy_beam = true;
                                lasers[j].left_active   = true;
                                lasers[j].right_active  = true;
                                lasers[j].x = enemies[i].x;
                                lasers[j].y = enemies[i].y;
                                break;
                            }
                        }

                        enemies[i].laser_beam_timer = GetRandomValue(1, 5);
                    }
                }
            }
        }
        PROFILE_END(game_update);

        // Render
        PROFILE_BEGIN(render);
        {
            BeginDrawing();

            ClearBackground(WHITE);

            DrawTextureEx(background_texture, (Vector2){0}, 0, SCALE, WHITE);

            // Render the player
            if (player_lives > 0)
            {
                unsigned char color_begin = (player_damaged_anim_retracting) ? 255 : 0;
                unsigned char color_end   = (player_damaged_anim_retracting) ?   0 : 255;
                unsigned char color = EaseQuadInOut(player_damaged_anim_timer, color_begin, color_end - color_begin, PLAYER_DAMAGE_FLASH_ANIM_DURATION);

                if (player_damaged_anim_timer > PLAYER_DAMAGE_FLASH_ANIM_DURATION)
                {
                    if (player_damaged_anim_retracting)
                    {
                        player_anim_flash_counter--;
                        if (player_anim_flash_counter == 0)
                        {
                            player_damaged_anim_playing = false;
                        }
                    }

                    player_damaged_anim_retracting = !player_damaged_anim_retracting;
                    player_damaged_anim_timer = 0;
                }

                if (player_damaged_anim_playing) player_damaged_anim_timer += GetFrameTime();

                DrawTexture(player_texture, player_x, player_y, WHITE);
                DrawTexture(player_texture, player_x, player_y, (Color) { RED.r, RED.g, RED.b, color });
            }
            // Render game over screen
            else
            {
                int font_size = GetFontDefault().baseSize * 5;
                Vector2 text_size = MeasureTextEx(GetFontDefault(), "GAME OVER", font_size, 2);
                Vector2 text_pos  = (Vector2) { SCREEN_WIDTH / 2 - text_size.x / 2, SCREEN_HEIGHT / 2 - text_size.y / 2 };
                DrawTextEx(GetFontDefault(), "GAME OVER", text_pos, font_size, 2, WHITE);
            }

            // Drawing the enemies
            for (int i = 0; i < MAX_ENEMY_COUNT; i++)
            {
                if (enemies[i].active)
                {
                    DrawTexture(enemy_texture, enemies[i].x, enemies[i].y, WHITE);
                }
            }

            // Drawing the laser
            for (int i = 0; i < MAX_LASER_COUNT; i++)
            {
                if (lasers[i].left_active)  DrawTexture(lasers[i].is_enemy_beam ? enemy_laser_texture : player_laser_texture, lasers[i].x, lasers[i].y, WHITE);
                if (lasers[i].right_active) DrawTexture(lasers[i].is_enemy_beam ? enemy_laser_texture : player_laser_texture, lasers[i].x + player_texture.width - player_laser_texture.width, lasers[i].y, WHITE);
            }

            // Render the score
            {
                // Animate the font size
                int font_size_small = GetFontDefault().baseSize * 3;
                int font_size_big   = GetFontDefault().baseSize * 6;

                int font_begin = (font_retracting) ? font_size_big : font_size_small;
                int font_target = (font_retracting) ? font_size_small : font_size_big;

                int font_size       = (int) EaseQuadInOut(font_anim_timer, font_begin, font_target - font_begin, FONT_ANIM_DURATION);

                // Animate the font color
                Color color_begin = (font_retracting)  ? YELLOW : WHITE;
                Color color_target = (font_retracting) ? WHITE : YELLOW;

                unsigned char r = EaseQuadInOut(font_anim_timer, color_begin.r, color_target.r - color_begin.r, FONT_ANIM_DURATION);
                unsigned char g = EaseQuadInOut(font_anim_timer, color_begin.g, color_target.g - color_begin.g, FONT_ANIM_DURATION);
                unsigned char b = EaseQuadInOut(font_anim_timer, color_begin.b, color_target.b - color_begin.b, FONT_ANIM_DURATION);

                Color font_color = { r, g, b, 255 };

                if (font_anim_playing)
                    font_anim_timer += GetFrameTime();

                if (font_anim_timer > FONT_ANIM_DURATION)
                {
                    if (font_retracting)
                    {
                        font_anim_playing = false;
                    }

                    font_anim_timer = 0;
                    font_retracting = !font_retracting;
                }

                DrawText(TextFormat("Score: %d", score), 20, 20, font_size, font_color);
            }

            // Render HP
            {
                int font_size = GetFontDefault().baseSize * 3;
                const char* text = TextFormat("HP: %d", player_lives * 1000);
                int text_width = MeasureText(text, font_size);
                DrawText(text, SCREEN_WIDTH - text_width - 20, 20, font_size, WHITE);
            }

            if (!enemies_are_alive)
            {
                int font_size = GetFontDefault().baseSize * 5;
                Vector2 text_size = MeasureTextEx(GetFontDefault(), "YOU WIN DOOM ETERNAL", font_size, 2);
                Vector2 text_pos  = (Vector2) { SCREEN_WIDTH / 2 - text_size.x / 2, SCREEN_HEIGHT / 2 - text_size.y / 2 };
                DrawTextEx(GetFontDefault(), "YOU WIN DOOM ETERNAL", text_pos, font_size, 2, WHITE);
            }

            EndDrawing();
        }
        PROFILE_END(render);
    	
    	PROFILE_END(main_loop);
    }
}
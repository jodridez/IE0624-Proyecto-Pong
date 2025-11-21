/**
 * @file game_logic.c
 * @brief Implementación de la lógica del juego Pong
 * 
 * Basado en código funcional con LTDC
 */

#include "game_logic.h"
#include <stdlib.h>
#include <stdio.h>

/* Parámetros del sensor */
#define MIN_DISTANCE  1  // cm
#define MAX_DISTANCE  6  // cm

/* Funciones auxiliares de dibujo */
static void clear_screen(layer1_pixel *fb, uint16_t color);
static void draw_filled_rect(layer1_pixel *fb, int16_t x, int16_t y, 
                             int16_t w, int16_t h, uint16_t color);
static void draw_char(layer1_pixel *fb, int16_t x, int16_t y, 
                     char c, uint16_t color);
static void draw_number(layer1_pixel *fb, int16_t x, int16_t y, 
                       uint8_t num, uint16_t color);

/* Font 5x7 simple */
static const uint8_t font_5x7[][5] = {
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 0
    {0x00, 0x21, 0x7F, 0x01, 0x00}, // 1
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 2
    {0x22, 0x49, 0x49, 0x49, 0x36}, // 3
    {0x0C, 0x14, 0x24, 0x7F, 0x04}, // 4
    {0x72, 0x51, 0x51, 0x51, 0x4E}, // 5
    {0x3E, 0x49, 0x49, 0x49, 0x26}, // 6
    {0x60, 0x47, 0x48, 0x50, 0x60}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x32, 0x49, 0x49, 0x49, 0x3E}, // 9
};

/* Implementación de funciones de dibujo */
static void clear_screen(layer1_pixel *fb, uint16_t color) {
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        fb[i] = color;
    }
}

static void draw_filled_rect(layer1_pixel *fb, int16_t x, int16_t y, 
                             int16_t w, int16_t h, uint16_t color) {
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            int16_t px = x + i;
            int16_t py = y + j;
            if (px >= 0 && px < LCD_WIDTH && py >= 0 && py < LCD_HEIGHT) {
                fb[py * LCD_WIDTH + px] = color;
            }
        }
    }
}

static void draw_char(layer1_pixel *fb, int16_t x, int16_t y, 
                     char c, uint16_t color) {
    if (c >= '0' && c <= '9') {
        int idx = c - '0';
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 8; j++) {
                if (font_5x7[idx][i] & (1 << j)) {
                    int16_t px = x + i * 2;
                    int16_t py = y + j * 2;
                    draw_filled_rect(fb, px, py, 2, 2, color);
                }
            }
        }
    }
}

static void draw_number(layer1_pixel *fb, int16_t x, int16_t y, 
                       uint8_t num, uint16_t color) {
    char buffer[4];
    snprintf(buffer, sizeof(buffer), "%d", num);
    int16_t cx = x;
    for (char *p = buffer; *p; p++) {
        draw_char(fb, cx, y, *p, color);
        cx += 12;
    }
}

/* Implementación de funciones públicas */
void game_init(Game *game) {
    game->ball.x = LCD_WIDTH / 2;
    game->ball.y = LCD_HEIGHT / 2;
    game->ball.vx = BALL_SPEED_X;
    game->ball.vy = BALL_SPEED_Y;
    
    game->player.y = LCD_HEIGHT / 2 - PADDLE_HEIGHT / 2;
    game->player.score = 0;
    
    game->cpu.y = LCD_HEIGHT / 2 - PADDLE_HEIGHT / 2;
    game->cpu.score = 0;
    
    game->running = true;
    game->game_over = false;
    game->frame_count = 0;
}

void game_update_player_paddle(Game *game, uint16_t distance)
{
    if (distance == 0xFFFF) return;

    if (distance < MIN_DISTANCE) distance = MIN_DISTANCE;
    if (distance > MAX_DISTANCE) distance = MAX_DISTANCE;

    /* Filtro interno de la distancia (beta pequeño porque ya filtró el driver) */
    static float dist_filtered = 0.0f;
    static bool df_initialized = false;
    const float beta = 0.15f;  /* 0.0 = sin filtro, >0 más suavizado */

    if (!df_initialized) {
        dist_filtered = (float)distance;
        df_initialized = true;
    } else {
        dist_filtered = dist_filtered * (1.0f - beta) + (float)distance * beta;
    }

    /* Mapear la distancia filtrada */
    int16_t target_y = (int16_t)(((dist_filtered - MIN_DISTANCE) * 
                       (LCD_HEIGHT - PADDLE_HEIGHT)) /
                       (float)(MAX_DISTANCE - MIN_DISTANCE));

    /* Movimiento suavizado hacia target (interpolación exponencial) */
    const float alpha = 0.45f; /* 0.0 muy suave, 1.0 instantáneo */
    float new_y = (float)game->player.y * (1.0f - alpha) + (float)target_y * alpha;
    game->player.y = (int16_t)new_y;

    /* Limitar dentro del área */
    if (game->player.y < 0) game->player.y = 0;
    if (game->player.y > LCD_HEIGHT - PADDLE_HEIGHT)
        game->player.y = LCD_HEIGHT - PADDLE_HEIGHT;
}



void game_update(Game *game) {
    game->frame_count++;
    
    /* Actualizar pelota */
    game->ball.x += game->ball.vx;
    game->ball.y += game->ball.vy;
    
    /* Colisión con bordes superior/inferior */
    if (game->ball.y <= 0 || game->ball.y >= LCD_HEIGHT - BALL_SIZE) {
        game->ball.vy = -game->ball.vy;
    }
    
    /* Colisión con jugador (izquierda) */
    if (game->ball.x <= PADDLE_WIDTH && 
        game->ball.y + BALL_SIZE >= game->player.y && 
        game->ball.y <= game->player.y + PADDLE_HEIGHT) {
        game->ball.vx = abs(game->ball.vx);
        int16_t hit_pos = (game->ball.y - game->player.y) - (PADDLE_HEIGHT / 2);
        game->ball.vy = hit_pos / 5;
    }
    
    /* Colisión con CPU (derecha) */
    if (game->ball.x >= LCD_WIDTH - PADDLE_WIDTH - BALL_SIZE && 
        game->ball.y + BALL_SIZE >= game->cpu.y && 
        game->ball.y <= game->cpu.y + PADDLE_HEIGHT) {
        game->ball.vx = -abs(game->ball.vx);
        int16_t hit_pos = (game->ball.y - game->cpu.y) - (PADDLE_HEIGHT / 2);
        game->ball.vy = hit_pos / 5;
    }
    
    /* Puntos - Pelota salió por la izquierda */
    if (game->ball.x < 0) {
        game->cpu.score++;
        game->ball.x = LCD_WIDTH / 2;
        game->ball.y = LCD_HEIGHT / 2;
        game->ball.vx = BALL_SPEED_X;
        game->ball.vy = BALL_SPEED_Y;
    }
    
    /* Puntos - Pelota salió por la derecha */
    if (game->ball.x > LCD_WIDTH) {
        game->player.score++;
        game->ball.x = LCD_WIDTH / 2;
        game->ball.y = LCD_HEIGHT / 2;
        game->ball.vx = -BALL_SPEED_X;
        game->ball.vy = BALL_SPEED_Y;
    }
    
    /* IA del CPU - Sigue la pelota */
    if (game->ball.y < game->cpu.y + PADDLE_HEIGHT / 2) {
        game->cpu.y -= PADDLE_SPEED - 1;
    } else if (game->ball.y > game->cpu.y + PADDLE_HEIGHT / 2) {
        game->cpu.y += PADDLE_SPEED - 1;
    }
    
    /* Limitar CPU a la pantalla */
    if (game->cpu.y < 0) game->cpu.y = 0;
    if (game->cpu.y > LCD_HEIGHT - PADDLE_HEIGHT) {
        game->cpu.y = LCD_HEIGHT - PADDLE_HEIGHT;
    }
    
    /* Verificar victoria */
    if (game->player.score >= WINNING_SCORE || game->cpu.score >= WINNING_SCORE) {
        game->game_over = true;
    }
}

void game_render(const Game *game, layer1_pixel *framebuffer) {
    /* Limpiar pantalla */
    clear_screen(framebuffer, COLOR_BLACK);
    
    /* Línea central punteada */
    for (int16_t i = 0; i < LCD_HEIGHT; i += 15) {
        draw_filled_rect(framebuffer, LCD_WIDTH / 2 - 1, i, 2, 8, COLOR_WHITE);
    }
    
    /* Paleta del jugador (izquierda, verde) */
    draw_filled_rect(framebuffer, 0, game->player.y, 
                     PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_GREEN);
    
    /* Paleta del CPU (derecha, roja) */
    draw_filled_rect(framebuffer, LCD_WIDTH - PADDLE_WIDTH, game->cpu.y, 
                     PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_RED);
    
    /* Pelota (amarilla) */
    draw_filled_rect(framebuffer, game->ball.x, game->ball.y, 
                     BALL_SIZE, BALL_SIZE, COLOR_YELLOW);
    
    /* Marcadores */
    draw_number(framebuffer, LCD_WIDTH / 2 - 40, 20, 
               game->player.score, COLOR_GREEN);
    draw_number(framebuffer, LCD_WIDTH / 2 + 20, 20, 
               game->cpu.score, COLOR_RED);
    
    /* Mensaje de fin de juego */
    if (game->game_over) {
        draw_filled_rect(framebuffer, LCD_WIDTH / 2 - 50, 
                        LCD_HEIGHT / 2 - 20, 100, 40, COLOR_BLUE);
    }
}

void game_reset(Game *game) {
    game_init(game);
}

bool game_is_active(const Game *game) {
    return game->running && !game->game_over;
}

bool game_is_over(const Game *game) {
    return game->game_over;
}
/**
 * @file game_logic.c
 * @brief Implementación de la lógica del juego Pong
 * 
 * Basado en código funcional con LTDC
 */

#include "game_logic.h"
#include <stdlib.h>
#include <stdio.h>

/* Parámetros del sensor - AJUSTAR SEGÚN TU HARDWARE */
#define MIN_DISTANCE  5   // cm - distancia mínima de operación
#define MAX_DISTANCE  30  // cm - distancia máxima de operación

/* Funciones auxiliares de dibujo */
static void clear_screen(layer1_pixel *fb, uint16_t color);
static void draw_filled_rect(layer1_pixel *fb, int16_t x, int16_t y, 
                             int16_t w, int16_t h, uint16_t color);
static void draw_char(layer1_pixel *fb, int16_t x, int16_t y, 
                     char c, uint16_t color);
static void draw_number(layer1_pixel *fb, int16_t x, int16_t y, 
                       uint8_t num, uint16_t color);
static void draw_string(layer1_pixel *fb, int16_t x, int16_t y, 
                       const char *str, uint16_t color);

static void transform_coords(int16_t *x, int16_t *y) {
    *x = LCD_WIDTH - 1 - *x;
    *y = LCD_HEIGHT - 1 - *y;
}


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

/* Letras adicionales para "GAME OVER" y "WIN" */
static const uint8_t font_letters[][5] = {
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // E
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x48, 0x48, 0x48, 0x7F}, // A
    {0x41, 0x7F, 0x49, 0x49, 0x41}, // Y
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // U
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // S
    {0x7F, 0x40, 0x7C, 0x40, 0x7F}, // W
    {0x7F, 0x01, 0x01, 0x01, 0x7F}, // I
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // G
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // M
    {0x7F, 0x40, 0x40, 0x40, 0x7F}, // V
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // R
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
                // CORREGIDO: invertir el orden de los bits
                if (font_5x7[idx][i] & (1 << (7 - j))) {
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

static void draw_letter(layer1_pixel *fb, int16_t x, int16_t y, 
                       char c, uint16_t color) {
    int idx = -1;
    switch(c) {
        case 'P': idx = 0; break;
        case 'E': idx = 1; break;
        case 'L': idx = 2; break;
        case 'A': idx = 3; break;
        case 'Y': idx = 4; break;
        case 'U': idx = 5; break;
        case 'O': idx = 6; break;
        case 'S': idx = 7; break;
        case 'W': idx = 8; break;
        case 'I': idx = 9; break;
        case 'N': idx = 10; break;
        case 'G': idx = 11; break;
        case 'M': idx = 12; break;
        case 'V': idx = 13; break;
        case 'R': idx = 14; break;
        case ' ': return; // Espacio
        default: return;
    }
    
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 8; j++) {
            if (font_letters[idx][i] & (1 << (7 - j))) {
                int16_t px = x + i * 2;
                int16_t py = y + j * 2;
                draw_filled_rect(fb, px, py, 2, 2, color);
            }
        }
    }
}

static void draw_string(layer1_pixel *fb, int16_t x, int16_t y, 
                       const char *str, uint16_t color) {
    int16_t cx = x;
    for (const char *p = str; *p; p++) {
        draw_letter(fb, cx, y, *p, color);
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

    // Limitar a rango válido
    if (distance < MIN_DISTANCE) distance = MIN_DISTANCE;
    if (distance > MAX_DISTANCE) distance = MAX_DISTANCE;

    // SIMPLIFICADO: Un solo filtro exponencial suave
    static float paddle_y_filtered = 160.0f;  // Centro de pantalla
    static bool initialized = false;
    
    if (!initialized) {
        paddle_y_filtered = (float)(LCD_HEIGHT / 2 - PADDLE_HEIGHT / 2);
        initialized = true;
    }

    // Mapear distancia a posición Y (invertir si es necesario)
    float target_y = ((float)(distance - MIN_DISTANCE) * 
                     (LCD_HEIGHT - PADDLE_HEIGHT)) /
                     (float)(MAX_DISTANCE - MIN_DISTANCE);

    // Filtro exponencial único con alpha más alto para respuesta rápida
    const float alpha = 0.3f;  // Mayor = más rápido, menor = más suave
    paddle_y_filtered = paddle_y_filtered * (1.0f - alpha) + target_y * alpha;

    game->player.y = (int16_t)paddle_y_filtered;

    // Limitar dentro del área
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
        // Fondo azul
        draw_filled_rect(framebuffer, LCD_WIDTH / 2 - 80, 
                        LCD_HEIGHT / 2 - 30, 160, 60, COLOR_BLUE);
        
        // Determinar ganador y mostrar mensaje
        if (game->player.score >= WINNING_SCORE) {
            draw_string(framebuffer, LCD_WIDTH / 2 - 50, 
                       LCD_HEIGHT / 2 - 20, "YOU WIN", COLOR_YELLOW);
        } else {
            draw_string(framebuffer, LCD_WIDTH / 2 - 60, 
                       LCD_HEIGHT / 2 - 20, "GAME OVER", COLOR_RED);
        }
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
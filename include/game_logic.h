/**
 * @file game_logic.h
 * @brief Lógica del juego Pong
 * 
 * Basado en código funcional con LTDC
 * 
 * IE0624 - Laboratorio de Microcontroladores
 */

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdint.h>
#include <stdbool.h>

/* Resolución de la pantalla */
#define LCD_WIDTH  240
#define LCD_HEIGHT 320

/* Colores RGB565 */
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

/* Parámetros del juego */
#define PADDLE_WIDTH  6
#define PADDLE_HEIGHT 40
#define BALL_SIZE     6
#define PADDLE_SPEED  4
#define BALL_SPEED_X  2
#define BALL_SPEED_Y  2
#define WINNING_SCORE 5

/* Tipo de píxel RGB565 */
typedef uint16_t layer1_pixel;

/* Estructura de la pelota */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t vx;
    int16_t vy;
} Ball;

/* Estructura de una paleta */
typedef struct {
    int16_t y;
    uint8_t score;
} Paddle;

/* Estado del juego */
typedef struct {
    Ball ball;
    Paddle player;
    Paddle cpu;
    bool running;
    bool game_over;
    uint32_t frame_count;
} Game;

/**
 * @brief Inicializa el juego
 */
void game_init(Game *game);

/**
 * @brief Actualiza la lógica del juego
 */
void game_update(Game *game);

/**
 * @brief Renderiza el juego en el framebuffer
 */
void game_render(const Game *game, layer1_pixel *framebuffer);

/**
 * @brief Actualiza la paleta del jugador desde distancia del sensor
 * 
 * @param game Puntero al estado del juego
 * @param distance Distancia en cm del sensor (0xFFFF si error)
 */
void game_update_player_paddle(Game *game, uint16_t distance);

/**
 * @brief Reinicia el juego
 */
void game_reset(Game *game);

/**
 * @brief Verifica si el juego está activo
 */
bool game_is_active(const Game *game);

/**
 * @brief Verifica si el juego terminó
 */
bool game_is_over(const Game *game);

#endif /* GAME_LOGIC_H */
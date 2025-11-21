/**
 * @file game_logic.h
 * @brief Lógica del juego Pong
 * 
 * Implementa la máquina de estados, física del juego,
 * detección de colisiones y sistema de puntuación.
 * 
 * IE0624 - Laboratorio de Microcontroladores
 */

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include "lcd_graphics.h"

/* Dimensiones del juego */
#define GAME_WIDTH      LCD_WIDTH
#define GAME_HEIGHT     LCD_HEIGHT

/* Paletas */
#define PADDLE_WIDTH    8
#define PADDLE_HEIGHT   40
#define PADDLE_SPEED    5
#define PADDLE_MARGIN   10

/* Pelota */
#define BALL_SIZE       8
#define BALL_SPEED_X    3
#define BALL_SPEED_Y    2

/* Puntuación */
#define MAX_SCORE       5

/* Estados del juego */
typedef enum {
    GAME_STATE_INIT,        /**< Inicialización */
    GAME_STATE_READY,       /**< Listo para iniciar */
    GAME_STATE_COUNTDOWN,   /**< Cuenta regresiva */
    GAME_STATE_PLAYING,     /**< Jugando */
    GAME_STATE_SCORE,       /**< Animación de anotación */
    GAME_STATE_GAME_OVER,   /**< Fin del juego */
    GAME_STATE_PAUSED       /**< Pausado */
} game_state_t;

/* Estructura de la pelota */
typedef struct {
    int16_t x;              /**< Posición X del centro */
    int16_t y;              /**< Posición Y del centro */
    int16_t vx;             /**< Velocidad X */
    int16_t vy;             /**< Velocidad Y */
    uint16_t size;          /**< Tamaño (radio) */
    color_t color;          /**< Color de la pelota */
} ball_t;

/* Estructura de una paleta */
typedef struct {
    int16_t x;              /**< Posición X */
    int16_t y;              /**< Posición Y */
    uint16_t width;         /**< Ancho */
    uint16_t height;        /**< Altura */
    color_t color;          /**< Color de la paleta */
    uint8_t score;          /**< Puntuación del jugador */
} paddle_t;

/* Estructura del estado del juego */
typedef struct {
    game_state_t state;     /**< Estado actual */
    paddle_t paddle_left;   /**< Paleta izquierda (jugador 1) */
    paddle_t paddle_right;  /**< Paleta derecha (jugador 2) */
    ball_t ball;            /**< Pelota */
    uint32_t frame_count;   /**< Contador de frames */
    uint8_t countdown;      /**< Contador para countdown */
    uint8_t winner;         /**< Ganador (1 o 2, 0 si no hay) */
} game_t;

/**
 * @brief Inicializa el juego
 * 
 * @param game Puntero a estructura del juego
 */
void game_init(game_t *game);

/**
 * @brief Resetea el juego a estado inicial
 * 
 * @param game Puntero a estructura del juego
 */
void game_reset(game_t *game);

/**
 * @brief Resetea la pelota al centro
 * 
 * @param game Puntero a estructura del juego
 * @param direction Dirección inicial (-1 izquierda, 1 derecha, 0 aleatorio)
 */
void game_reset_ball(game_t *game, int8_t direction);

/**
 * @brief Actualiza la posición de una paleta
 * 
 * @param game Puntero a estructura del juego
 * @param paddle Puntero a la paleta (left o right)
 * @param position Nueva posición Y (será limitada a rango válido)
 */
void game_update_paddle(game_t *game, paddle_t *paddle, int16_t position);

/**
 * @brief Actualiza la física de la pelota
 * 
 * @param game Puntero a estructura del juego
 */
void game_update_ball(game_t *game);

/**
 * @brief Detecta colisiones de la pelota
 * 
 * Detecta colisiones con paredes, paletas y zonas de anotación.
 * Actualiza velocidad y puntuación según corresponda.
 * 
 * @param game Puntero a estructura del juego
 * @return true si hubo gol, false en otro caso
 */
bool game_check_collisions(game_t *game);

/**
 * @brief Actualiza el estado del juego (FSM)
 * 
 * @param game Puntero a estructura del juego
 */
void game_update_state(game_t *game);

/**
 * @brief Renderiza el juego completo
 * 
 * Dibuja paletas, pelota, marcador y elementos de UI.
 * 
 * @param game Puntero a estructura del juego
 */
void game_render(const game_t *game);

/**
 * @brief Renderiza la paleta izquierda
 * 
 * @param paddle Puntero a la paleta
 */
void game_render_paddle_left(const paddle_t *paddle);

/**
 * @brief Renderiza la paleta derecha
 * 
 * @param paddle Puntero a la paleta
 */
void game_render_paddle_right(const paddle_t *paddle);

/**
 * @brief Renderiza la pelota
 * 
 * @param ball Puntero a la pelota
 */
void game_render_ball(const ball_t *ball);

/**
 * @brief Renderiza el marcador
 * 
 * @param game Puntero a estructura del juego
 */
void game_render_score(const game_t *game);

/**
 * @brief Renderiza la línea central divisoria
 */
void game_render_center_line(void);

/**
 * @brief Renderiza pantalla de Game Over
 * 
 * @param winner Número del jugador ganador (1 o 2)
 */
void game_render_game_over(uint8_t winner);

/**
 * @brief Renderiza cuenta regresiva
 * 
 * @param count Número a mostrar (3, 2, 1)
 */
void game_render_countdown(uint8_t count);

/**
 * @brief Inicia el juego (transición de READY a COUNTDOWN)
 * 
 * @param game Puntero a estructura del juego
 */
void game_start(game_t *game);

/**
 * @brief Pausa/reanuda el juego
 * 
 * @param game Puntero a estructura del juego
 */
void game_toggle_pause(game_t *game);

/**
 * @brief Verifica si el juego está activo
 * 
 * @param game Puntero a estructura del juego
 * @return true si está en estado PLAYING
 */
bool game_is_playing(const game_t *game);

/**
 * @brief Obtiene el estado actual del juego
 * 
 * @param game Puntero a estructura del juego
 * @return Estado actual
 */
game_state_t game_get_state(const game_t *game);

#endif /* GAME_LOGIC_H */
/**
 * @file game_logic.c
 * @brief Implementación de la lógica del juego Pong
 */

#include "game_logic.h"
#include <stdlib.h>
#include <string.h>

/* Forward declaration for internal pixel drawing */
extern void lcd_draw_pixel(int x, int y, uint16_t color);

/* Estado anterior de la pelota (para borrado eficiente) */
static ball_t prev_ball;
static paddle_t prev_paddle_left;
static paddle_t prev_paddle_right;

void game_init(game_t *game) {
    memset(game, 0, sizeof(game_t));
    
    /* Inicializar paleta izquierda */
    game->paddle_left.x = PADDLE_MARGIN;
    game->paddle_left.y = (GAME_HEIGHT - PADDLE_HEIGHT) / 2;
    game->paddle_left.width = PADDLE_WIDTH;
    game->paddle_left.height = PADDLE_HEIGHT;
    game->paddle_left.color = COLOR_GREEN;
    game->paddle_left.score = 0;
    
    /* Inicializar paleta derecha */
    game->paddle_right.x = GAME_WIDTH - PADDLE_MARGIN - PADDLE_WIDTH;
    game->paddle_right.y = (GAME_HEIGHT - PADDLE_HEIGHT) / 2;
    game->paddle_right.width = PADDLE_WIDTH;
    game->paddle_right.height = PADDLE_HEIGHT;
    game->paddle_right.color = COLOR_CYAN;
    game->paddle_right.score = 0;
    
    /* Inicializar pelota */
    game->ball.x = GAME_WIDTH / 2;
    game->ball.y = GAME_HEIGHT / 2;
    game->ball.vx = BALL_SPEED_X;
    game->ball.vy = BALL_SPEED_Y;
    game->ball.size = BALL_SIZE;
    game->ball.color = COLOR_WHITE;
    
    /* Estado inicial */
    game->state = GAME_STATE_INIT;
    game->frame_count = 0;
    game->countdown = 3;
    game->winner = 0;
    
    /* Copiar estados previos */
    prev_ball = game->ball;
    prev_paddle_left = game->paddle_left;
    prev_paddle_right = game->paddle_right;
}

void game_reset(game_t *game) {
    game->paddle_left.score = 0;
    game->paddle_right.score = 0;
    game->winner = 0;
    game_reset_ball(game, 0);
    game->state = GAME_STATE_READY;
}

void game_reset_ball(game_t *game, int8_t direction) {
    game->ball.x = GAME_WIDTH / 2;
    game->ball.y = GAME_HEIGHT / 2;
    
    /* Dirección: -1 izquierda, 1 derecha, 0 aleatorio */
    if (direction == 0) {
        direction = (rand() % 2) ? 1 : -1;
    }
    
    game->ball.vx = BALL_SPEED_X * direction;
    
    /* Velocidad Y aleatoria */
    game->ball.vy = (rand() % 2) ? BALL_SPEED_Y : -BALL_SPEED_Y;
}

void game_update_paddle(game_t *game, paddle_t *paddle, int16_t position) {
    /* Guardar posición anterior */
    if (paddle == &game->paddle_left) {
        prev_paddle_left = *paddle;
    } else {
        prev_paddle_right = *paddle;
    }
    
    /* Limitar posición al rango válido */
    if (position < 0) {
        position = 0;
    }
    if (position > GAME_HEIGHT - paddle->height) {
        position = GAME_HEIGHT - paddle->height;
    }
    
    paddle->y = position;
}

void game_update_ball(game_t *game) {
    /* Guardar posición anterior */
    prev_ball = game->ball;
    
    /* Actualizar posición */
    game->ball.x += game->ball.vx;
    game->ball.y += game->ball.vy;
}

bool game_check_collisions(game_t *game) {
    bool goal = false;
    
    /* Colisión con paredes superior e inferior */
    if (game->ball.y - game->ball.size <= 0) {
        game->ball.y = game->ball.size;
        game->ball.vy = -game->ball.vy;
    }
    if (game->ball.y + game->ball.size >= GAME_HEIGHT) {
        game->ball.y = GAME_HEIGHT - game->ball.size;
        game->ball.vy = -game->ball.vy;
    }
    
    /* Colisión con paleta izquierda */
    if (game->ball.vx < 0) {  // Solo si va hacia la izquierda
        if (game->ball.x - game->ball.size <= game->paddle_left.x + game->paddle_left.width) {
            if (game->ball.y >= game->paddle_left.y && 
                game->ball.y <= game->paddle_left.y + game->paddle_left.height) {
                
                game->ball.x = game->paddle_left.x + game->paddle_left.width + game->ball.size;
                game->ball.vx = -game->ball.vx;
                
                /* Modificar ángulo según dónde golpeó */
                int16_t hit_pos = game->ball.y - (game->paddle_left.y + game->paddle_left.height / 2);
                game->ball.vy = hit_pos / 5;
            }
        }
    }
    
    /* Colisión con paleta derecha */
    if (game->ball.vx > 0) {  // Solo si va hacia la derecha
        if (game->ball.x + game->ball.size >= game->paddle_right.x) {
            if (game->ball.y >= game->paddle_right.y && 
                game->ball.y <= game->paddle_right.y + game->paddle_right.height) {
                
                game->ball.x = game->paddle_right.x - game->ball.size;
                game->ball.vx = -game->ball.vx;
                
                /* Modificar ángulo según dónde golpeó */
                int16_t hit_pos = game->ball.y - (game->paddle_right.y + game->paddle_right.height / 2);
                game->ball.vy = hit_pos / 5;
            }
        }
    }
    
    /* Verificar gol */
    if (game->ball.x - game->ball.size <= 0) {
        /* Gol para jugador 2 */
        game->paddle_right.score++;
        goal = true;
    } else if (game->ball.x + game->ball.size >= GAME_WIDTH) {
        /* Gol para jugador 1 */
        game->paddle_left.score++;
        goal = true;
    }
    
    return goal;
}

void game_update_state(game_t *game) {
    game->frame_count++;
    
    switch (game->state) {
        case GAME_STATE_INIT:
            game->state = GAME_STATE_READY;
            break;
            
        case GAME_STATE_READY:
            /* Esperar comando de inicio */
            break;
            
        case GAME_STATE_COUNTDOWN:
            /* Countdown de 3 segundos (aproximado con frames) */
            if (game->frame_count % 30 == 0) {  // Asumiendo 30 FPS
                game->countdown--;
                if (game->countdown == 0) {
                    game->state = GAME_STATE_PLAYING;
                    game->countdown = 3;  // Resetear para próxima vez
                }
            }
            break;
            
        case GAME_STATE_PLAYING:
            /* Actualizar física */
            game_update_ball(game);
            
            /* Verificar colisiones */
            if (game_check_collisions(game)) {
                /* Hubo gol */
                game->state = GAME_STATE_SCORE;
                game->frame_count = 0;
                
                /* Verificar si alguien ganó */
                if (game->paddle_left.score >= MAX_SCORE) {
                    game->winner = 1;
                } else if (game->paddle_right.score >= MAX_SCORE) {
                    game->winner = 2;
                }
            }
            break;
            
        case GAME_STATE_SCORE:
            /* Mostrar animación de gol por ~2 segundos */
            if (game->frame_count > 60) {  // 2 segundos a 30 FPS
                if (game->winner != 0) {
                    game->state = GAME_STATE_GAME_OVER;
                } else {
                    /* Resetear pelota y continuar */
                    int8_t direction = (game->paddle_left.score > game->paddle_right.score) ? -1 : 1;
                    game_reset_ball(game, direction);
                    game->state = GAME_STATE_COUNTDOWN;
                    game->frame_count = 0;
                }
            }
            break;
            
        case GAME_STATE_GAME_OVER:
            /* Esperar reset */
            break;
            
        case GAME_STATE_PAUSED:
            /* Esperar resume */
            break;
    }
}

void game_render(const game_t *game) {
    /* No limpiar toda la pantalla para mejor performance */
    /* Solo borrar posiciones anteriores y dibujar nuevas */
    
    /* Renderizar fondo (solo primera vez) */
    static bool first_render = true;
    if (first_render) {
        lcd_clear(COLOR_BLACK);
        game_render_center_line();
        first_render = false;
    }
    
    /* Borrar y redibujar paletas si cambiaron */
    if (prev_paddle_left.y != game->paddle_left.y) {
        rect_t rect = {prev_paddle_left.x, prev_paddle_left.y, 
                       prev_paddle_left.width, prev_paddle_left.height};
        lcd_fill_rect(&rect, COLOR_BLACK);
        game_render_paddle_left(&game->paddle_left);
    }
    
    if (prev_paddle_right.y != game->paddle_right.y) {
        rect_t rect = {prev_paddle_right.x, prev_paddle_right.y,
                       prev_paddle_right.width, prev_paddle_right.height};
        lcd_fill_rect(&rect, COLOR_BLACK);
        game_render_paddle_right(&game->paddle_right);
    }
    
    /* Borrar pelota anterior */
    lcd_fill_circle(prev_ball.x, prev_ball.y, prev_ball.size, COLOR_BLACK);
    
    /* Dibujar pelota nueva */
    if (game->state == GAME_STATE_PLAYING || 
        game->state == GAME_STATE_COUNTDOWN) {
        game_render_ball(&game->ball);
    }
    
    /* Renderizar marcador (siempre) */
    game_render_score(game);
    
    /* Renderizar overlays según estado */
    if (game->state == GAME_STATE_COUNTDOWN) {
        game_render_countdown(game->countdown);
    } else if (game->state == GAME_STATE_GAME_OVER) {
        game_render_game_over(game->winner);
    }
}

void game_render_paddle_left(const paddle_t *paddle) {
    rect_t rect = {paddle->x, paddle->y, paddle->width, paddle->height};
    lcd_fill_rect(&rect, paddle->color);
}

void game_render_paddle_right(const paddle_t *paddle) {
    rect_t rect = {paddle->x, paddle->y, paddle->width, paddle->height};
    lcd_fill_rect(&rect, paddle->color);
}

void game_render_ball(const ball_t *ball) {
    lcd_fill_circle(ball->x, ball->y, ball->size, ball->color);
}

void game_render_score(const game_t *game) {
    /* Borrar área del marcador */
    rect_t score_area = {0, 0, GAME_WIDTH, 20};
    lcd_fill_rect(&score_area, COLOR_BLACK);
    
    /* Dibujar marcador */
    char score_str[20];
    
    /* Jugador 1 */
    lcd_draw_string(40, 5, "P1:", COLOR_GREEN, COLOR_BLACK);
    lcd_draw_number(64, 5, game->paddle_left.score, COLOR_WHITE, COLOR_BLACK);
    
    /* Jugador 2 */
    lcd_draw_string(140, 5, "P2:", COLOR_CYAN, COLOR_BLACK);
    lcd_draw_number(164, 5, game->paddle_right.score, COLOR_WHITE, COLOR_BLACK);
}

void game_render_center_line(void) {
    /* Línea punteada vertical en el centro */
    for (int16_t y = 0; y < GAME_HEIGHT; y += 10) {
        lcd_draw_vline(GAME_WIDTH / 2, y, 5, COLOR_DARKGRAY);
    }
}

void game_render_game_over(uint8_t winner) {
    /* Overlay semi-transparente (simulado con color oscuro) */
    rect_t overlay = {40, GAME_HEIGHT/2 - 40, GAME_WIDTH - 80, 80};
    lcd_fill_rect(&overlay, COLOR_DARKGRAY);
    lcd_draw_rect(&overlay, COLOR_WHITE);
    
    /* Texto de Game Over */
    lcd_draw_string(70, GAME_HEIGHT/2 - 20, "GAME OVER", COLOR_WHITE, COLOR_DARKGRAY);
    
    /* Ganador */
    char msg[20];
    if (winner == 1) {
        lcd_draw_string(60, GAME_HEIGHT/2, "P1 WINS!", COLOR_GREEN, COLOR_DARKGRAY);
    } else {
        lcd_draw_string(60, GAME_HEIGHT/2, "P2 WINS!", COLOR_CYAN, COLOR_DARKGRAY);
    }
}

void game_render_countdown(uint8_t count) {
    if (count == 0) return;
    
    /* Dibujar número grande en el centro */
    int16_t x = GAME_WIDTH / 2 - 4;
    int16_t y = GAME_HEIGHT / 2 - 4;
    
    /* Escalar carácter 3x */
    for (int dy = 0; dy < 24; dy += 3) {
        for (int dx = 0; dx < 24; dx += 3) {
            lcd_fill_rect(&(rect_t){x + dx - 12, y + dy - 12, 3, 3}, COLOR_YELLOW);
        }
    }
    
    lcd_draw_number(x, y, count, COLOR_YELLOW, COLOR_BLACK);
}

void game_start(game_t *game) {
    if (game->state == GAME_STATE_READY) {
        game->state = GAME_STATE_COUNTDOWN;
        game->frame_count = 0;
    }
}

void game_toggle_pause(game_t *game) {
    if (game->state == GAME_STATE_PLAYING) {
        game->state = GAME_STATE_PAUSED;
    } else if (game->state == GAME_STATE_PAUSED) {
        game->state = GAME_STATE_PLAYING;
    }
}

bool game_is_playing(const game_t *game) {
    return (game->state == GAME_STATE_PLAYING);
}

game_state_t game_get_state(const game_t *game) {
    return game->state;
}
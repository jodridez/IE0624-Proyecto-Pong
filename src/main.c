/**
 * @file main.c
 * @brief Programa principal del juego Pong
 * 
 * Integra todos los módulos: sensores ultrasónicos,
 * gráficos LCD y lógica del juego.
 * 
 * IE0624 - Laboratorio de Microcontroladores
 * Proyecto: Pong con control ultrasónico STM32F429I-DISC1
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#include "clock.h"
#include "ultrasonic.h"
#include "lcd_graphics.h"
#include "game_logic.h"

/* Variables globales */
static game_t game;
static volatile uint32_t sensor_update_flag = false;

/* Configuración del sistema */
#define TARGET_FPS 30
#define FRAME_TIME_MS (1000 / TARGET_FPS)
#define SENSOR_UPDATE_INTERVAL_MS 50

/* Variable externa de mtime */
static uint32_t last_sensor_update = 0;

/**
 * @brief Configura el reloj del sistema a 168 MHz
 */
static void clock_setup_local(void) {
    /* Usar la función de clock.c */
    clock_setup();
}

/**
 * @brief Obtiene el tiempo actual en ms
 */
static uint32_t get_time_ms(void) {
    return mtime();
}

/**
 * @brief Delay en milisegundos
 */
static void delay_ms(uint32_t ms) {
    msleep(ms);
}

/**
 * @brief Configura botón de usuario para start/reset
 */
static void button_setup(void) {
    /* Botón de usuario en PA0 del STM32F429-Discovery */
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
}

/**
 * @brief Lee el estado del botón
 * @return true si está presionado
 */
static bool button_is_pressed(void) {
    return gpio_get(GPIOA, GPIO0) != 0;
}

/**
 * @brief Actualiza las posiciones de las paletas desde los sensores
 */
static void update_paddles_from_sensors(void) {
    uint32_t current_time = get_time_ms();
    
    /* Solo actualizar cada SENSOR_UPDATE_INTERVAL_MS */
    if (current_time - last_sensor_update < SENSOR_UPDATE_INTERVAL_MS) {
        return;
    }
    
    last_sensor_update = current_time;

        /* Usar sensores simulados en lugar de reales */
    simulate_sensors();
    
    
    /* Leer sensores */
    ultrasonic_trigger(SENSOR_LEFT);
    delay_ms(5);  /* Pequeño delay entre triggers */
    ultrasonic_trigger(SENSOR_RIGHT);
    
    /* Esperar mediciones (timeout) */
    uint32_t timeout = 100;
    uint32_t start = get_time_ms();
    while ((!ultrasonic_is_ready(SENSOR_LEFT) || 
            !ultrasonic_is_ready(SENSOR_RIGHT)) &&
           (get_time_ms() - start < timeout)) {
        __asm__("nop");
    }
    
    /* Actualizar paletas si hay mediciones válidas */
    if (ultrasonic_is_ready(SENSOR_LEFT)) {
        uint32_t pos = ultrasonic_map_to_position(SENSOR_LEFT, 0, 
                                                   GAME_HEIGHT - PADDLE_HEIGHT);
        game_update_paddle(&game, &game.paddle_left, pos);
    }
    
    if (ultrasonic_is_ready(SENSOR_RIGHT)) {
        uint32_t pos = ultrasonic_map_to_position(SENSOR_RIGHT, 0,
                                                   GAME_HEIGHT - PADDLE_HEIGHT);
        game_update_paddle(&game, &game.paddle_right, pos);
    }
}

/**
 * @brief Muestra pantalla de bienvenida
 */
static void show_welcome_screen(void) {
    lcd_clear(COLOR_BLACK);
    
    /* Título */
    lcd_draw_string(70, 100, "PONG GAME", COLOR_WHITE, COLOR_BLACK);
    
    /* Instrucciones */
    lcd_draw_string(30, 140, "Use your hands", COLOR_YELLOW, COLOR_BLACK);
    lcd_draw_string(30, 155, "near sensors", COLOR_YELLOW, COLOR_BLACK);
    
    /* Info */
    lcd_draw_string(20, 200, "Press button to start", COLOR_GREEN, COLOR_BLACK);
    
    lcd_draw_string(30, 280, "IE0624 - UCR", COLOR_GRAY, COLOR_BLACK);
    lcd_draw_string(40, 295, "STM32F429I", COLOR_GRAY, COLOR_BLACK);
    
    delay_ms(2000);
}

/**
 * @brief Calibración inicial de sensores
 */
static void calibrate_sensors(void) {
    lcd_clear(COLOR_BLACK);
    lcd_draw_string(50, 140, "Calibrating...", COLOR_YELLOW, COLOR_BLACK);
    lcd_draw_string(20, 160, "Place hands at center", COLOR_WHITE, COLOR_BLACK);
    
    delay_ms(1000);
    
    //ultrasonic_calibrate();
    
    lcd_draw_string(60, 200, "Ready!", COLOR_GREEN, COLOR_BLACK);
    delay_ms(1000);
}

/**
 * @brief Bucle principal del juego
 */
static void game_loop(void) {
    uint32_t last_frame_time = get_time_ms();
    uint32_t frame_count = 0;
    bool button_pressed_last = false;
    
    while (1) {
        uint32_t current_time = get_time_ms();
        uint32_t elapsed = current_time - last_frame_time;
        
        /* Mantener frame rate objetivo */
        if (elapsed < FRAME_TIME_MS) {
            continue;
        }
        
        last_frame_time = current_time;
        frame_count++;
        
        /* Actualizar sensores periódicamente */
        if (game_is_playing(&game) || 
            game_get_state(&game) == GAME_STATE_COUNTDOWN) {
            update_paddles_simulated();
        }
        
        /* Detectar presión de botón (con debounce) */
        bool button_pressed = button_is_pressed();
        if (button_pressed && !button_pressed_last) {
            game_state_t state = game_get_state(&game);
            
            if (state == GAME_STATE_READY) {
                game_start(&game);
            } else if (state == GAME_STATE_GAME_OVER) {
                game_reset(&game);
            } else if (state == GAME_STATE_PLAYING || 
                       state == GAME_STATE_PAUSED) {
                game_toggle_pause(&game);
            }
        }
        button_pressed_last = button_pressed;
        
        /* Actualizar lógica del juego */
        game_update_state(&game);
        
        /* Renderizar */
        game_render(&game);
        
        /* Debug: mostrar FPS cada segundo (opcional) */
        #ifdef DEBUG_FPS
        if (frame_count % TARGET_FPS == 0) {
            char fps_str[20];
            uint32_t fps = (frame_count * 1000) / current_time;
            lcd_draw_string(5, 5, "FPS:", COLOR_RED, COLOR_BLACK);
            lcd_draw_number(35, 5, fps, COLOR_RED, COLOR_BLACK);
        }
        #endif
    }
}


/**
 * @brief Simula sensores cuando no están conectados
 */
static void simulate_sensors(void) {
    /* Posición simulada basada en tiempo */
    uint32_t time = get_time_ms();
    uint32_t left_pos = (time / 20) % (GAME_HEIGHT - PADDLE_HEIGHT);
    uint32_t right_pos = (time / 25) % (GAME_HEIGHT - PADDLE_HEIGHT);
    
    game_update_paddle(&game, &game.paddle_left, left_pos);
    game_update_paddle(&game, &game.paddle_right, right_pos);
}

/**
 * @brief Actualiza paletas con sensores simulados
 */
static void update_paddles_simulated(void) {
    uint32_t current_time = get_time_ms();
    
    if (current_time - last_sensor_update < SENSOR_UPDATE_INTERVAL_MS) {
        return;
    }
    
    last_sensor_update = current_time;
    simulate_sensors();
}

/**
 * @brief Función principal
 */
int main(void) {
    /* Configuración del sistema */
    clock_setup_local();
    button_setup();
    
    /* Inicializar subsistemas */
    lcd_init();
    ultrasonic_init();
    
    /* Pantalla de bienvenida */
    show_welcome_screen();
    
    /* Calibración de sensores */
    calibrate_sensors();
    
    /* Inicializar juego */
    game_init(&game);
    
    /* Preparar para iniciar */
    lcd_clear(COLOR_BLACK);
    game.state = GAME_STATE_READY;
    
    /* Mensaje inicial */
    lcd_draw_string(40, 150, "Press button", COLOR_WHITE, COLOR_BLACK);
    lcd_draw_string(50, 165, "to start!", COLOR_WHITE, COLOR_BLACK);
    lcd_update();
    
    /* Esperar botón para comenzar */
    while (!button_is_pressed()) {
        delay_ms(10);
    }
    delay_ms(200);  /* Debounce */
    
    /* Iniciar juego */
    game_start(&game);
    
    /* Bucle principal */
    game_loop();
    
    /* Nunca debería llegar aquí */
    return 0;
}



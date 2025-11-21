/**
 * @file ultrasonic.h
 * @brief Driver para sensores ultrasónicos HY-SRF05
 * 
 * Implementa la interfaz para leer distancias de dos sensores
 * ultrasónicos usando GPIO y timers con interrupciones.
 * 
 * IE0624 - Laboratorio de Microcontroladores
 * Proyecto: Pong con control ultrasónico
 */

#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Constantes de configuración */
#define ULTRASONIC_MIN_DISTANCE_CM  5      /**< Distancia mínima detectable */
#define ULTRASONIC_MAX_DISTANCE_CM  200    /**< Distancia máxima útil */
#define ULTRASONIC_TIMEOUT_US       25000  /**< Timeout para medición */

/* Número de sensores */
#define NUM_SENSORS 2

/* IDs de sensores */
typedef enum {
    SENSOR_LEFT = 0,   /**< Sensor izquierdo (jugador 1) */
    SENSOR_RIGHT = 1   /**< Sensor derecho (jugador 2) */
} sensor_id_t;

/* Estado de medición */
typedef enum {
    ULTRASONIC_IDLE,
    ULTRASONIC_MEASURING,
    ULTRASONIC_READY,
    ULTRASONIC_ERROR
} ultrasonic_state_t;

/* Estructura de datos del sensor */
typedef struct {
    uint32_t distance_cm;           /**< Distancia medida en cm */
    uint32_t raw_time_us;           /**< Tiempo de pulso en microsegundos */
    ultrasonic_state_t state;       /**< Estado actual */
    uint32_t timestamp;             /**< Timestamp de última medición */
    uint8_t error_count;            /**< Contador de errores consecutivos */
} ultrasonic_data_t;

/**
 * @brief Inicializa el módulo de sensores ultrasónicos
 * 
 * Configura GPIOs para TRIGGER y ECHO, configura timers
 * para medición de pulsos con interrupciones.
 */
void ultrasonic_init(void);

/**
 * @brief Dispara una medición en un sensor específico
 * 
 * @param sensor ID del sensor a medir
 */
void ultrasonic_trigger(sensor_id_t sensor);

/**
 * @brief Obtiene la distancia medida por un sensor
 * 
 * @param sensor ID del sensor
 * @return Distancia en centímetros (0 si no válida)
 */
uint32_t ultrasonic_get_distance(sensor_id_t sensor);

/**
 * @brief Obtiene los datos completos de un sensor
 * 
 * @param sensor ID del sensor
 * @return Puntero a estructura con datos del sensor
 */
const ultrasonic_data_t* ultrasonic_get_data(sensor_id_t sensor);

/**
 * @brief Verifica si hay una medición lista
 * 
 * @param sensor ID del sensor
 * @return true si hay medición válida disponible
 */
bool ultrasonic_is_ready(sensor_id_t sensor);

/**
 * @brief Obtiene el estado de un sensor
 * 
 * @param sensor ID del sensor
 * @return Estado actual del sensor
 */
ultrasonic_state_t ultrasonic_get_state(sensor_id_t sensor);

/**
 * @brief Realiza calibración inicial de sensores
 * 
 * Toma múltiples mediciones para establecer rangos válidos
 * y filtrar ruido inicial.
 */
void ultrasonic_calibrate(void);

/**
 * @brief Mapea distancia a posición de paleta
 * 
 * Convierte la distancia medida a una posición en el rango
 * de movimiento de la paleta del juego.
 * 
 * @param sensor ID del sensor
 * @param min_pos Posición mínima de la paleta
 * @param max_pos Posición máxima de la paleta
 * @return Posición mapeada
 */
uint32_t ultrasonic_map_to_position(sensor_id_t sensor, 
                                    uint32_t min_pos, 
                                    uint32_t max_pos);

/**
 * @brief Resetea el estado de un sensor
 * 
 * @param sensor ID del sensor a resetear
 */
void ultrasonic_reset(sensor_id_t sensor);

/**
 * @brief Handler de interrupción del timer (llamado desde ISR)
 * 
 * @param sensor ID del sensor que generó la interrupción
 * @param rising true si es flanco de subida, false si bajada
 */
void ultrasonic_timer_isr_handler(sensor_id_t sensor, bool rising);

#endif /* ULTRASONIC_H */
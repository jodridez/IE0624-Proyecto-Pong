/**
 * @file ultrasonic.h
 * @brief Driver para sensor ultrasónico HC-SR05
 * 
 * Basado en código funcional con polling
 * 
 * IE0624 - Laboratorio de Microcontroladores
 */

#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>
#include <stdbool.h>

/* Parámetros del sensor HC-SR05 */
#define MIN_DISTANCE  5   /**< Distancia mínima en cm */
#define MAX_DISTANCE  30  /**< Distancia máxima en cm */
#define TRIGGER_TIME  10  /**< Duración del pulso trigger en us */

/**
 * @brief Inicializa el sensor ultrasónico HC-SR05
 * 
 * Configura:
 * - PB0: Trigger (Output)
 * - PB1: Echo (Input con EXTI)
 */
void hcsr05_setup(void);

/**
 * @brief Lee la distancia del sensor
 * 
 * @return Distancia en centímetros, o 0xFFFF si hay error/timeout
 */
uint16_t hcsr05_read_distance(void);

#endif /* ULTRASONIC_H */
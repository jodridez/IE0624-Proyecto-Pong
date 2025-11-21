/**
 * @file ultrasonic.c
 * @brief Implementación del driver para HC-SR05
 * 
 * Basado en código funcional con polling
 */

#include "ultrasonic.h"
#include "clock.h"
#include <stdlib.h>  // Para abs()
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>

/* Calibración para 168MHz */
#define NOP_CYCLES_PER_US 42

#define WINDOW 3  // Aumentado a 3 para mejor filtrado de mediana
static uint16_t buf[WINDOW];
static uint8_t i = 0;
static uint16_t last = 0;

/* Variable global para la ISR */
static volatile bool echo_received = false;

/**
 * @brief Delay en microsegundos usando busy-wait
 */
static void delay_us(uint32_t us) {
    uint32_t count = us * NOP_CYCLES_PER_US;
    while (count--) {
        __asm__("nop");
    }
}

/**
 * @brief ISR para EXTI1 (pin Echo)
 */
void exti1_isr(void) {
    if (!gpio_get(GPIOB, GPIO1)) {
        /* Echo bajó (fin de pulso) */
        echo_received = true;
    }
    exti_reset_request(EXTI1);
}

void hcsr05_setup(void) {
    /* Habilitar clocks */
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_SYSCFG);
    
    /* Trigger: PB0 (Output) */
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO0);
    gpio_clear(GPIOB, GPIO0);
    
    /* Echo: PB1 (Input) */
    gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO1);
    
    /* Configurar EXTI1 para detectar fin de pulso */
    nvic_enable_irq(NVIC_EXTI1_IRQ);
    exti_select_source(EXTI1, GPIOB);
    exti_set_trigger(EXTI1, EXTI_TRIGGER_BOTH);
    exti_enable_request(EXTI1);
}

uint16_t median_filter(uint16_t new_val) {
    buf[i] = new_val;
    i = (i + 1) % WINDOW;

    uint16_t temp[WINDOW];
    memcpy(temp, buf, sizeof(temp));
    
    // Ordenar (bubble sort simple)
    for (int a = 0; a < WINDOW; a++)
        for (int b = a + 1; b < WINDOW; b++)
            if (temp[b] < temp[a]) {
                uint16_t t = temp[a];
                temp[a] = temp[b];
                temp[b] = t;
            }

    return temp[WINDOW / 2];
}

uint16_t hcsr05_read_distance(void) {
    echo_received = false;

    /* Enviar pulso trigger de 10us */
    gpio_clear(GPIOB, GPIO0);
    delay_us(2);
    gpio_set(GPIOB, GPIO0);
    delay_us(TRIGGER_TIME);
    gpio_clear(GPIOB, GPIO0);

    /* Esperar a que el pin Echo suba */
    uint32_t start_time_ms = mtime();
    while (!gpio_get(GPIOB, GPIO1)) {
        if ((mtime() - start_time_ms) > 1) {
            return 0xFFFF; /* Timeout esperando subida */
        }
    }

    /* Medir duración del pulso Echo usando polling */
    uint32_t cycles = 0;
    uint32_t max_cycles = 30000 * NOP_CYCLES_PER_US; /* 30ms timeout */

    while (gpio_get(GPIOB, GPIO1) && cycles < max_cycles) {
        cycles++;
    }

    if (cycles >= max_cycles) {
        return 0xFFFF; /* Timeout */
    }

    uint32_t duration_us = cycles / NOP_CYCLES_PER_US;
    uint32_t distance_cm = duration_us / 58;

    /* --- Validaciones básicas --- */
    if (distance_cm == 0 || distance_cm > 150) {
        return 0xFFFF; // distancia inválida
    }

    /* Inicializar ventana de mediana en la primera lectura válida */
    static bool window_initialized = false;
    if (!window_initialized) {
        for (int k = 0; k < WINDOW; k++) buf[k] = (uint16_t)distance_cm;
        last = (uint16_t)distance_cm;
        window_initialized = true;
    }

    /* 1) Filtrar con mediana (reduce picos) */
    distance_cm = median_filter((uint16_t)distance_cm);

    /* 2) Anti-glitch: si el salto es demasiado grande, devolver la última estable */
    int diff = (int)distance_cm - (int)last;
    if (last != 0 && abs(diff) > 15) {  // Umbral reducido de 30 a 15 cm
        return last;
    }

    /* Guardar última lectura estable y devolver */
    last = (uint16_t)distance_cm;

    return (uint16_t)distance_cm;
}
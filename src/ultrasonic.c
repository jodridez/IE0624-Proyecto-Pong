/**
 * @file ultrasonic.c
 * @brief Implementación del driver para HC-SR05
 * 
 * Basado en código funcional con polling
 */

#include "ultrasonic.h"
#include "clock.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>

/* Calibración para 168MHz */
#define NOP_CYCLES_PER_US 42

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
    
    /*
     * Cálculo de distancia:
     * - Duración en us = cycles / NOP_CYCLES_PER_US
     * - Distancia en cm = Duración_us / 58
     * 
     * (Velocidad del sonido: 343 m/s → 29.15 us/cm ida → 58 us/cm ida y vuelta)
     */
    uint32_t duration_us = cycles / NOP_CYCLES_PER_US;
    uint32_t distance_cm = duration_us / 58;
    
    return (uint16_t)distance_cm;
}
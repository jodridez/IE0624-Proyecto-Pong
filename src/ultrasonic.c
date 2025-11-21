/**
 * @file ultrasonic.c
 * @brief Implementación del driver para sensores HY-SRF05
 */

#include "ultrasonic.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

/* Configuración de pines GPIO */
#define TRIG_PORT_LEFT    GPIOB
#define TRIG_PIN_LEFT     GPIO0
#define ECHO_PORT_LEFT    GPIOB
#define ECHO_PIN_LEFT     GPIO1

#define TRIG_PORT_RIGHT   GPIOB
#define TRIG_PIN_RIGHT    GPIO2
#define ECHO_PORT_RIGHT   GPIOB
#define ECHO_PIN_RIGHT    GPIO3

/* Timers para cada sensor */
#define TIMER_LEFT        TIM2
#define TIMER_RIGHT       TIM3

/* Constantes de cálculo */
#define SOUND_SPEED_CM_US  0.0343f  // 343 m/s = 0.0343 cm/us
#define TRIGGER_PULSE_US   10

/* Variables globales */
static ultrasonic_data_t sensor_data[NUM_SENSORS];
static volatile uint32_t start_time[NUM_SENSORS];
static volatile uint32_t end_time[NUM_SENSORS];
static volatile bool measuring[NUM_SENSORS];

/* Calibración */
static uint32_t min_distance[NUM_SENSORS] = {ULTRASONIC_MIN_DISTANCE_CM, ULTRASONIC_MIN_DISTANCE_CM};
static uint32_t max_distance[NUM_SENSORS] = {ULTRASONIC_MAX_DISTANCE_CM, ULTRASONIC_MAX_DISTANCE_CM};

/**
 * @brief Delay en microsegundos usando busy-wait
 */
static void delay_us(uint32_t us) {
    /* Asumiendo 180 MHz, aproximadamente 180 ciclos por us */
    volatile uint32_t cycles = us * 180;
    while (cycles--) {
        __asm__("nop");
    }
}

/**
 * @brief Configura GPIO para un sensor
 */
static void setup_gpio_for_sensor(sensor_id_t sensor) {
    uint32_t trig_port, echo_port;
    uint16_t trig_pin, echo_pin;
    
    if (sensor == SENSOR_LEFT) {
        trig_port = TRIG_PORT_LEFT;
        trig_pin = TRIG_PIN_LEFT;
        echo_port = ECHO_PORT_LEFT;
        echo_pin = ECHO_PIN_LEFT;
    } else {
        trig_port = TRIG_PORT_RIGHT;
        trig_pin = TRIG_PIN_RIGHT;
        echo_port = ECHO_PORT_RIGHT;
        echo_pin = ECHO_PIN_RIGHT;
    }
    
    /* TRIGGER: salida digital */
    gpio_mode_setup(trig_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, trig_pin);
    gpio_set_output_options(trig_port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, trig_pin);
    gpio_clear(trig_port, trig_pin);
    
    /* ECHO: entrada con modo timer (captura) */
    gpio_mode_setup(echo_port, GPIO_MODE_AF, GPIO_PUPD_NONE, echo_pin);
    
    if (sensor == SENSOR_LEFT) {
        gpio_set_af(echo_port, GPIO_AF1, echo_pin); // TIM2
    } else {
        gpio_set_af(echo_port, GPIO_AF2, echo_pin); // TIM3
    }
}

/**
 * @brief Configura timer para medición de pulsos
 */
static void setup_timer_for_sensor(sensor_id_t sensor) {
    uint32_t timer;
    uint32_t nvic_irq;
    
    if (sensor == SENSOR_LEFT) {
        timer = TIMER_LEFT;
        nvic_irq = NVIC_TIM2_IRQ;
        rcc_periph_clock_enable(RCC_TIM2);
    } else {
        timer = TIMER_RIGHT;
        nvic_irq = NVIC_TIM3_IRQ;
        rcc_periph_clock_enable(RCC_TIM3);
    }
    
    /* Reset timer */
    timer_set_counter(timer, 0);  // reset the counter to 0

    
    /* Configurar timer base: 1 MHz (1 us por tick) */
    timer_set_mode(timer, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(timer, 84 - 1);  // 84 MHz / 84 = 1 MHz
    timer_set_period(timer, 0xFFFFFFFF); // Máximo período
    
    /* Configurar canal de captura (canal 1) */
    timer_ic_set_input(timer, TIM_IC1, TIM_IC_IN_TI1);
    timer_ic_set_filter(timer, TIM_IC1, TIM_IC_OFF);
    timer_ic_set_polarity(timer, TIM_IC1, TIM_IC_RISING);
    timer_ic_enable(timer, TIM_IC1);
    
    /* Habilitar interrupciones de captura */
    timer_enable_irq(timer, TIM_DIER_CC1IE);
    nvic_enable_irq(nvic_irq);
    
    /* Iniciar timer */
    timer_enable_counter(timer);
}

void ultrasonic_init(void) {
    /* Habilitar clocks */
    rcc_periph_clock_enable(RCC_GPIOB);
    
    /* Inicializar estructuras de datos */
    for (int i = 0; i < NUM_SENSORS; i++) {
        sensor_data[i].distance_cm = 0;
        sensor_data[i].raw_time_us = 0;
        sensor_data[i].state = ULTRASONIC_IDLE;
        sensor_data[i].timestamp = 0;
        sensor_data[i].error_count = 0;
        measuring[i] = false;
    }
    
    /* Configurar cada sensor */
    for (sensor_id_t s = SENSOR_LEFT; s < NUM_SENSORS; s++) {
        setup_gpio_for_sensor(s);
        setup_timer_for_sensor(s);
    }
}

void ultrasonic_trigger(sensor_id_t sensor) {
    if (sensor >= NUM_SENSORS) return;
    
    uint32_t trig_port;
    uint16_t trig_pin;
    
    if (sensor == SENSOR_LEFT) {
        trig_port = TRIG_PORT_LEFT;
        trig_pin = TRIG_PIN_LEFT;
    } else {
        trig_port = TRIG_PORT_RIGHT;
        trig_pin = TRIG_PIN_RIGHT;
    }
    
    /* Marcar como midiendo */
    measuring[sensor] = true;
    sensor_data[sensor].state = ULTRASONIC_MEASURING;
    
    /* Enviar pulso de trigger (10 us) */
    gpio_set(trig_port, trig_pin);
    delay_us(TRIGGER_PULSE_US);
    gpio_clear(trig_port, trig_pin);
}

uint32_t ultrasonic_get_distance(sensor_id_t sensor) {
    if (sensor >= NUM_SENSORS) return 0;
    return sensor_data[sensor].distance_cm;
}

const ultrasonic_data_t* ultrasonic_get_data(sensor_id_t sensor) {
    if (sensor >= NUM_SENSORS) return NULL;
    return &sensor_data[sensor];
}

bool ultrasonic_is_ready(sensor_id_t sensor) {
    if (sensor >= NUM_SENSORS) return false;
    return (sensor_data[sensor].state == ULTRASONIC_READY);
}

ultrasonic_state_t ultrasonic_get_state(sensor_id_t sensor) {
    if (sensor >= NUM_SENSORS) return ULTRASONIC_ERROR;
    return sensor_data[sensor].state;
}

void ultrasonic_calibrate(void) {
    /* Tomar varias mediciones para establecer baseline */
    const int num_samples = 10;
    uint32_t sum[NUM_SENSORS] = {0, 0};
    
    for (int i = 0; i < num_samples; i++) {
        for (sensor_id_t s = SENSOR_LEFT; s < NUM_SENSORS; s++) {
            ultrasonic_trigger(s);
            
            /* Esperar medición */
            uint32_t timeout = 1000000;
            while (!ultrasonic_is_ready(s) && timeout--) {
                delay_us(1);
            }
            
            if (ultrasonic_is_ready(s)) {
                sum[s] += ultrasonic_get_distance(s);
            }
        }
        delay_us(60000); // Esperar 60ms entre mediciones
    }
    
    /* Calcular promedios como punto medio */
    for (sensor_id_t s = SENSOR_LEFT; s < NUM_SENSORS; s++) {
        uint32_t avg = sum[s] / num_samples;
        min_distance[s] = (avg > 20) ? (avg - 20) : ULTRASONIC_MIN_DISTANCE_CM;
        max_distance[s] = avg + 20;
    }
}

uint32_t ultrasonic_map_to_position(sensor_id_t sensor, 
                                    uint32_t min_pos, 
                                    uint32_t max_pos) {
    if (sensor >= NUM_SENSORS) return min_pos;
    
    uint32_t distance = sensor_data[sensor].distance_cm;
    
    /* Limitar al rango calibrado */
    if (distance < min_distance[sensor]) distance = min_distance[sensor];
    if (distance > max_distance[sensor]) distance = max_distance[sensor];
    
    /* Mapear linealmente */
    uint32_t range_in = max_distance[sensor] - min_distance[sensor];
    uint32_t range_out = max_pos - min_pos;
    
    if (range_in == 0) return min_pos;
    
    uint32_t position = min_pos + 
                       ((distance - min_distance[sensor]) * range_out) / range_in;
    
    return position;
}

void ultrasonic_reset(sensor_id_t sensor) {
    if (sensor >= NUM_SENSORS) return;
    
    sensor_data[sensor].state = ULTRASONIC_IDLE;
    sensor_data[sensor].error_count = 0;
    measuring[sensor] = false;
}

void ultrasonic_timer_isr_handler(sensor_id_t sensor, bool rising) {
    if (sensor >= NUM_SENSORS) return;
    
    uint32_t timer = (sensor == SENSOR_LEFT) ? TIMER_LEFT : TIMER_RIGHT;
    uint32_t capture_value = timer_get_counter(timer);
    
    if (rising) {
        /* Flanco de subida: inicio del pulso ECHO */
        start_time[sensor] = capture_value;
        
        /* Cambiar polaridad para capturar flanco de bajada */
        timer_ic_set_polarity(timer, TIM_IC1, TIM_IC_FALLING);
    } else {
        /* Flanco de bajada: fin del pulso ECHO */
        end_time[sensor] = capture_value;
        
        /* Calcular tiempo de pulso */
        uint32_t pulse_time_us;
        if (end_time[sensor] >= start_time[sensor]) {
            pulse_time_us = end_time[sensor] - start_time[sensor];
        } else {
            /* Overflow del timer */
            pulse_time_us = (0xFFFFFFFF - start_time[sensor]) + end_time[sensor];
        }
        
        sensor_data[sensor].raw_time_us = pulse_time_us;
        
        /* Calcular distancia: d = (t * velocidad_sonido) / 2 */
        uint32_t distance = (uint32_t)((float)pulse_time_us * SOUND_SPEED_CM_US / 2.0f);
        
        /* Validar medición */
        if (distance >= ULTRASONIC_MIN_DISTANCE_CM && 
            distance <= ULTRASONIC_MAX_DISTANCE_CM) {
            sensor_data[sensor].distance_cm = distance;
            sensor_data[sensor].state = ULTRASONIC_READY;
            sensor_data[sensor].error_count = 0;
        } else {
            sensor_data[sensor].state = ULTRASONIC_ERROR;
            sensor_data[sensor].error_count++;
        }
        
        measuring[sensor] = false;
        
        /* Restaurar polaridad para siguiente medición */
        timer_ic_set_polarity(timer, TIM_IC1, TIM_IC_RISING);
    }
}

/* ISRs de timers */
void tim2_isr(void) {
    if (timer_get_flag(TIM2, TIM_SR_CC1IF)) {
        timer_clear_flag(TIM2, TIM_SR_CC1IF);
        bool rising = (TIM_CCER(TIM2) & TIM_CCER_CC1P) == 0;
        ultrasonic_timer_isr_handler(SENSOR_LEFT, rising);
    }
}

void tim3_isr(void) {
    if (timer_get_flag(TIM3, TIM_SR_CC1IF)) {
        timer_clear_flag(TIM3, TIM_SR_CC1IF);
        bool rising = (TIM_CCER(TIM3) & TIM_CCER_CC1P) == 0;
        ultrasonic_timer_isr_handler(SENSOR_RIGHT, rising);
    }
}
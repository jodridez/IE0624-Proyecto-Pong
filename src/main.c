/**
 * @file main.c
 * @brief Programa principal - Juego Pong con sensor ultrasónico
 * 
 * Hardware:
 * - STM32F429I-DISC1
 * - HC-SR05: Trigger=PB0, Echo=PB1
 * - Botón Usuario: PA0
 * 
 * IE0624 - Laboratorio de Microcontroladores
 * Universidad de Costa Rica
 */

#include <stdio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/ltdc.h>

#include "clock.h"
#include "console.h"
#include "sdram.h"
#include "lcd-spi.h"
#include "ultrasonic.h"
#include "game_logic.h"

/* Configuración del LCD LTDC */
#define REFRESH_RATE 70 /* Hz */
#define HSYNC       10
#define HBP         20
#define HFP         10
#define VSYNC        2
#define VBP          2
#define VFP          4

/* Framebuffer en SDRAM */
layer1_pixel *const lcd_frame_buffer = (void *)SDRAM_BASE_ADDRESS;
#define LCD_LAYER1_PIXFORMAT LTDC_LxPFCR_RGB565
#define LCD_PIXEL_SIZE (sizeof(layer1_pixel))

/* Estado global del juego */
static Game game;

/**
 * @brief Inicializa el controlador LCD con LTDC
 */
static void lcd_ltdc_init(void) {
    /* Habilitar clocks para GPIO del LCD */
    rcc_periph_clock_enable(RCC_GPIOA | RCC_GPIOB | RCC_GPIOC |
                RCC_GPIOD | RCC_GPIOF | RCC_GPIOG);

    /* Configurar pines GPIO como AF14 (LTDC) */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
            GPIO3 | GPIO4 | GPIO6 | GPIO11 | GPIO12);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                GPIO3 | GPIO4 | GPIO6 | GPIO11 | GPIO12);
    gpio_set_af(GPIOA, GPIO_AF14, GPIO3 | GPIO4 | GPIO6 | GPIO11 | GPIO12);

    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE,
            GPIO8 | GPIO9 | GPIO10 | GPIO11);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                GPIO8 | GPIO9 | GPIO10 | GPIO11);
    gpio_set_af(GPIOB, GPIO_AF14, GPIO8 | GPIO9 | GPIO10 | GPIO11);

    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE,
            GPIO6 | GPIO7 | GPIO10);
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                GPIO6 | GPIO7 | GPIO10);
    gpio_set_af(GPIOC, GPIO_AF14, GPIO6 | GPIO7 | GPIO10);

    gpio_mode_setup(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE,
            GPIO3 | GPIO6);
    gpio_set_output_options(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                GPIO3 | GPIO6);
    gpio_set_af(GPIOD, GPIO_AF14, GPIO3 | GPIO6);

    gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);
    gpio_set_output_options(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO10);
    gpio_set_af(GPIOF, GPIO_AF14, GPIO10);

    gpio_mode_setup(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE,
            GPIO6 | GPIO7 | GPIO10 | GPIO11 | GPIO12);
    gpio_set_output_options(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                GPIO6 | GPIO7 | GPIO10 | GPIO11 | GPIO12);
    gpio_set_af(GPIOG, GPIO_AF9, GPIO10 | GPIO12);
    gpio_set_af(GPIOG, GPIO_AF14, GPIO6 | GPIO7 | GPIO11);

    /* Configurar PLL SAI para el clock del LCD */
    uint32_t sain = 192;
    uint32_t saiq = (RCC_PLLSAICFGR >> RCC_PLLSAICFGR_PLLSAIQ_SHIFT) &
            RCC_PLLSAICFGR_PLLSAIQ_MASK;
    uint32_t sair = 4;
    RCC_PLLSAICFGR = (sain << RCC_PLLSAICFGR_PLLSAIN_SHIFT |
              saiq << RCC_PLLSAICFGR_PLLSAIQ_SHIFT |
              sair << RCC_PLLSAICFGR_PLLSAIR_SHIFT);
    RCC_DCKCFGR |= RCC_DCKCFGR_PLLSAIDIVR_DIVR_8 << RCC_DCKCFGR_PLLSAIDIVR_SHIFT;
    RCC_CR |= RCC_CR_PLLSAION;
    while ((RCC_CR & RCC_CR_PLLSAIRDY) == 0);
    
    /* Habilitar clock del LTDC */
    RCC_APB2ENR |= RCC_APB2ENR_LTDCEN;

    /* Configurar timings del LCD */
    LTDC_SSCR = (HSYNC - 1) << LTDC_SSCR_HSW_SHIFT |
                (VSYNC - 1) << LTDC_SSCR_VSH_SHIFT;
    LTDC_BPCR = (HSYNC + HBP - 1) << LTDC_BPCR_AHBP_SHIFT |
                (VSYNC + VBP - 1) << LTDC_BPCR_AVBP_SHIFT;
    LTDC_AWCR = (HSYNC + HBP + LCD_WIDTH - 1) << LTDC_AWCR_AAW_SHIFT |
                (VSYNC + VBP + LCD_HEIGHT - 1) << LTDC_AWCR_AAH_SHIFT;
    LTDC_TWCR = (HSYNC + HBP + LCD_WIDTH + HFP - 1) << LTDC_TWCR_TOTALW_SHIFT |
                (VSYNC + VBP + LCD_HEIGHT + VFP - 1) << LTDC_TWCR_TOTALH_SHIFT;

    LTDC_GCR |= LTDC_GCR_PCPOL_ACTIVE_HIGH;
    LTDC_BCCR = 0x00000000; /* Color de fondo negro */

    /* Configurar interrupciones del LTDC */
    LTDC_IER = LTDC_IER_RRIE;
    nvic_enable_irq(NVIC_LCD_TFT_IRQ);

    /* Configurar Layer 1 (única capa) */
    uint32_t h_start = HSYNC + HBP + 0;
    uint32_t h_stop = HSYNC + HBP + LCD_WIDTH - 1;
    LTDC_L1WHPCR = h_stop << LTDC_LxWHPCR_WHSPPOS_SHIFT |
                   h_start << LTDC_LxWHPCR_WHSTPOS_SHIFT;
    
    uint32_t v_start = VSYNC + VBP + 0;
    uint32_t v_stop = VSYNC + VBP + LCD_HEIGHT - 1;
    LTDC_L1WVPCR = v_stop << LTDC_LxWVPCR_WVSPPOS_SHIFT |
                   v_start << LTDC_LxWVPCR_WVSTPOS_SHIFT;

    LTDC_L1PFCR = LCD_LAYER1_PIXFORMAT;
    LTDC_L1CFBAR = (uint32_t)lcd_frame_buffer;

    uint32_t pitch = LCD_WIDTH * LCD_PIXEL_SIZE;
    uint32_t length = LCD_WIDTH * LCD_PIXEL_SIZE + 3;
    LTDC_L1CFBLR = pitch << LTDC_LxCFBLR_CFBP_SHIFT |
                   length << LTDC_LxCFBLR_CFBLL_SHIFT;

    LTDC_L1CFBLNR = LCD_HEIGHT;
    LTDC_L1CACR = 0x000000FF;
    LTDC_L1BFCR = LTDC_LxBFCR_BF1_PIXEL_ALPHA_x_CONST_ALPHA |
                  LTDC_LxBFCR_BF2_PIXEL_ALPHA_x_CONST_ALPHA;

    /* Habilitar layer y LTDC */
    LTDC_L1CR |= LTDC_LxCR_LAYER_ENABLE;
    LTDC_SRCR |= LTDC_SRCR_VBR;
    LTDC_GCR |= LTDC_GCR_LTDC_ENABLE;
}

/**
 * @brief ISR del LCD - Se llama en cada frame (70Hz)
 */
void lcd_tft_isr(void) {
    LTDC_ICR |= LTDC_ICR_CRRIF;
    
    /* Actualizar juego si está activo */
    if (game_is_active(&game)) {
        game_update(&game);
    }
    
    /* Renderizar siempre */
    game_render(&game, lcd_frame_buffer);
    
    /* Recargar configuración de shadow */
    LTDC_SRCR |= LTDC_SRCR_VBR;
}

/**
 * @brief Configura botón de usuario
 */
static void button_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
}

/**
 * @brief Lee el botón con debounce
 */
static bool button_pressed(void) {
    if (gpio_get(GPIOA, GPIO0)) {
        milli_sleep(50);
        if (gpio_get(GPIOA, GPIO0)) {
            while (gpio_get(GPIOA, GPIO0)); /* Esperar release */
            return true;
        }
    }
    return false;
}

/**
 * @brief Función principal
 */
int main(void) {
    /* Inicializar reloj del sistema (168MHz con SysTick) */
    clock_setup();
    
    /* Inicializar consola UART para debug */
    console_setup(115200);
    console_stdio_setup();
    printf("Pong Game - STM32F429I-DISC1\n");
    
    /* Inicializar botón de usuario */
    button_setup();
    
    /* Inicializar SDRAM para framebuffer */
    printf("Initializing SDRAM...\n");
    sdram_init();
    
    /* Inicializar sensor ultrasónico */
    printf("Initializing HC-SR05 sensor...\n");
    hcsr05_setup();
    
    /* Inicializar LCD con LTDC */
    printf("Initializing LCD (LTDC)...\n");
    lcd_ltdc_init();
    lcd_spi_init(); /* Para inicialización del chip ILI9341 */
    
    /* Inicializar juego */
    printf("Starting game...\n");
    game_init(&game);
    
    printf("Game running! Use button to reset.\n");
    
    /* Loop principal */
    while (1) {
        /* Leer sensor cada 3 frames (~23 veces/segundo) */
        if (game.frame_count % 3 == 0) {
            uint16_t distance = hcsr05_read_distance();

                // DEBUG: Imprimir cada segundo aproximadamente
            if (game.frame_count % 70 == 0) {
                if (distance != 0xFFFF) {
                    printf("Distancia: %u cm, Paleta Y: %d\n", 
                    distance, game.player.y);
                } else {
                    printf("Error de lectura\n");
                }
            }

            game_update_player_paddle(&game, distance);
        }
        
        /* Reiniciar juego con botón */
        if (button_pressed()) {
            game_reset(&game);
            printf("Game reset!\n");
        }
        
        /* Pequeño delay para no saturar el loop */
        milli_sleep(1);
    }
    
    return 0;
}
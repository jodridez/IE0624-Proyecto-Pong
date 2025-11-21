/**
 * @file lcd_graphics.c
 * @brief Implementación del wrapper gráfico
 * 
 * Adapta las funciones de gfx.c y lcd-spi.c para nuestro proyecto
 */

#include "lcd_graphics.h"
#include "sdram.h"
#include <string.h>

/* Variables externas del framebuffer (definidas en lcd-spi.c) */
extern uint16_t *cur_frame;


void lcd_init(void) {
    /* Inicializar SDRAM para framebuffer */
    sdram_init();
    
    /* Inicializar LCD SPI */
    lcd_spi_init();
    
    /* Inicializar librería gráfica con la función lcd_draw_pixel de lcd-spi.c */
    gfx_init(gfx_drawPixel, LCD_WIDTH, LCD_HEIGHT);
    /* Limpiar pantalla */
    lcd_clear(COLOR_BLACK);
    lcd_update();
}

void lcd_clear(color_t color) {
    gfx_fillScreen(color);
}

void lcd_draw_hline(int16_t x, int16_t y, uint16_t width, color_t color) {
    gfx_drawFastHLine(x, y, width, color);
}

void lcd_draw_vline(int16_t x, int16_t y, uint16_t height, color_t color) {
    gfx_drawFastVLine(x, y, height, color);
}

void lcd_fill_rect(const rect_t *rect, color_t color) {
    gfx_fillRect(rect->x, rect->y, rect->width, rect->height, color);
}

void lcd_draw_rect(const rect_t *rect, color_t color) {
    gfx_drawRect(rect->x, rect->y, rect->width, rect->height, color);
}

void lcd_draw_string(int16_t x, int16_t y, const char *str, 
                     color_t color, color_t bg_color) {
    gfx_setTextColor(color, bg_color);
    gfx_setTextSize(1);
    gfx_setCursor(x, y);
    gfx_puts((char *)str);
}

void lcd_draw_number(int16_t x, int16_t y, int32_t num, 
                     color_t color, color_t bg_color) {
    char buffer[12];
    int idx = 0;
    
    if (num < 0) {
        buffer[idx++] = '-';
        num = -num;
    }
    
    if (num == 0) {
        buffer[idx++] = '0';
    } else {
        char temp[11];
        int temp_idx = 0;
        while (num > 0) {
            temp[temp_idx++] = '0' + (num % 10);
            num /= 10;
        }
        for (int i = temp_idx - 1; i >= 0; i--) {
            buffer[idx++] = temp[i];
        }
    }
    
    buffer[idx] = '\0';
    lcd_draw_string(x, y, buffer, color, bg_color);
}

color_t* lcd_get_framebuffer(void) {
    return cur_frame;
}

void lcd_show_frame(void) {
    gfx_update(); // Llama a la función de actualización de gfx
}

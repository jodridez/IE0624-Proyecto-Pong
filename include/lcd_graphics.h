/**
 * @file lcd_graphics.h
 * @brief Wrapper para la librería gráfica GFX
 * 
 * Adapta la librería gfx.h existente de LibOpenCM3
 * para nuestro proyecto Pong.
 * 
 * IE0624 - Laboratorio de Microcontroladores
 */

#ifndef LCD_GRAPHICS_H
#define LCD_GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>

/* Incluir las librerías existentes primero */
#include "lcd-spi.h"
#include "gfx.h"

/* Resolución de la pantalla */
#define LCD_WIDTH   240
#define LCD_HEIGHT  320

/* Tipo de color RGB565 */
typedef uint16_t color_t;

/* Colores usando las definiciones de gfx.h */
#define COLOR_BLACK       GFX_COLOR_BLACK
#define COLOR_WHITE       GFX_COLOR_WHITE
#define COLOR_RED         GFX_COLOR_RED
#define COLOR_GREEN       GFX_COLOR_GREEN
#define COLOR_BLUE        GFX_COLOR_BLUE
#define COLOR_YELLOW      GFX_COLOR_YELLOW
#define COLOR_CYAN        GFX_COLOR_CYAN
#define COLOR_MAGENTA     GFX_COLOR_MAGENTA
#define COLOR_GRAY        GFX_COLOR_GREY
#define COLOR_DARKGRAY    0x4208
#define COLOR_LIGHTGRAY   0xC618

/* Estructura para representar un rectángulo */
typedef struct {
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
} rect_t;

/**
 * @brief Inicializa el LCD y los gráficos
 */
void lcd_init(void);

/**
 * @brief Limpia toda la pantalla con un color
 */
void lcd_clear(color_t color);

/**
 * @brief Dibuja una línea horizontal
 */
void lcd_draw_hline(int16_t x, int16_t y, uint16_t width, color_t color);

/**
 * @brief Dibuja una línea vertical
 */
void lcd_draw_vline(int16_t x, int16_t y, uint16_t height, color_t color);

/**
 * @brief Dibuja un rectángulo relleno
 */
void lcd_fill_rect(const rect_t *rect, color_t color);

/**
 * @brief Dibuja un rectángulo sin relleno (solo borde)
 */
void lcd_draw_rect(const rect_t *rect, color_t color);

/**
 * @brief Dibuja un círculo relleno
 */
static inline void lcd_fill_circle(int16_t x, int16_t y, uint16_t radius, color_t color) {
    gfx_fillCircle(x, y, radius, color);
}

/**
 * @brief Dibuja un círculo sin relleno
 */
static inline void lcd_draw_circle(int16_t x, int16_t y, uint16_t radius, color_t color) {
    gfx_drawCircle(x, y, radius, color);
}

/**
 * @brief Dibuja una cadena de texto
 */
void lcd_draw_string(int16_t x, int16_t y, const char *str, 
                     color_t color, color_t bg_color);

/**
 * @brief Dibuja un número entero
 */
void lcd_draw_number(int16_t x, int16_t y, int32_t num, 
                     color_t color, color_t bg_color);

/**
 * @brief Convierte RGB888 a RGB565
 */
static inline color_t lcd_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @brief Actualiza la pantalla (muestra el frame actual)
 */
static inline void lcd_update(void) {
    lcd_show_frame();
}

/**
 * @brief Obtiene puntero al framebuffer
 */
color_t* lcd_get_framebuffer(void);

#endif /* LCD_GRAPHICS_H */
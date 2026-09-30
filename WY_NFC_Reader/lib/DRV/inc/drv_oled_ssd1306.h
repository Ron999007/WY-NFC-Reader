/**
 * @file    drv_oled_ssd1306.h
 * @brief   Platform-Independent SSD1306/SSD1315 OLED Driver Header.
 * @details Supports multiple resolutions (128x32, 128x64), algorithmic font scaling, 
 * and dynamic framebuffer allocation.
 */

#ifndef __DRV_OLED_SSD1306_H__
#define __DRV_OLED_SSD1306_H__

#include <stdint.h>
#include <stdbool.h>
#include "i2c_io.h"

/* Max Display Specifications to allocate SRAM */
#define OLED_MAX_WIDTH       128
#define OLED_MAX_HEIGHT      64
#define OLED_MAX_PAGES       (OLED_MAX_HEIGHT / 8)
#define OLED_MAX_BUFFER_SIZE (OLED_MAX_WIDTH * OLED_MAX_PAGES)

/**
 * @brief Enumeration for supported OLED resolutions.
 */
typedef enum {
    OLED_RES_128X32 = 0, /* Typical 0.91" OLED */
    OLED_RES_128X64      /* Typical 0.96" OLED (SSD1315/SSD1306) */
} oled_resolution_e;

/**
 * @brief Enumeration for dynamic algorithmic font scaling.
 */
typedef enum {
    OLED_SCALE_1X   = 1, /* Standard 5x8 font (Footprint: 6x8 pixels) */
    OLED_SCALE_1_5X = 2, /* Algorithmic 7x12 font (Footprint: 8x12 pixels) */
    OLED_SCALE_2X   = 3  /* Algorithmic 10x16 font (Footprint: 12x16 pixels) */
} oled_scale_e;

/**
 * @brief SSD1306/SSD1315 Driver Object structure.
 */
typedef struct {
    i2c_io_interface_t *io;              /* Injected hardware abstraction contract */
    uint8_t i2c_addr;                    /* 7-bit I2C slave address (e.g., 0x3C) */
    uint8_t width;                       /* Active display width in pixels */
    uint8_t height;                      /* Active display height in pixels */
    uint8_t pages;                       /* Number of vertical RAM pages */
    uint8_t buffer[OLED_MAX_BUFFER_SIZE];/* Internal SRAM Framebuffer (1024 Bytes) */
} ssd1306_obj_t;

/**
 * @brief Initializes the OLED hardware based on the target resolution.
 * @param obj      Pointer to the OLED driver object instance.
 * @param io       Pointer to the implemented I2C IO interface contract.
 * @param i2c_addr The 7-bit I2C address of the module (typically 0x3C).
 * @param res      The physical resolution of the mounted OLED module.
 */
void drv_oled_ssd1306_init(ssd1306_obj_t *obj, i2c_io_interface_t *io, uint8_t i2c_addr, oled_resolution_e res);

/**
 * @brief Clears the entire internal SRAM framebuffer (fills with 0x00).
 * @param obj      Pointer to the OLED driver object instance.
 */
void drv_oled_ssd1306_clear(ssd1306_obj_t *obj);

/**
 * @brief Draws or clears a single pixel in the local SRAM framebuffer.
 * @param obj   Pointer to the OLED driver object instance.
 * @param x     X-coordinate of the pixel.
 * @param y     Y-coordinate of the pixel.
 * @param color Pixel state: true to turn ON (white), false to turn OFF (black).
 */
void drv_oled_ssd1306_draw_pixel(ssd1306_obj_t *obj, int16_t x, int16_t y, bool color);

/**
 * @brief Renders a monochrome bitmap array into the local SRAM framebuffer.
 * @param obj    Pointer to the OLED driver object instance.
 * @param x      Starting X-coordinate.
 * @param y      Starting Y-coordinate.
 * @param bitmap Pointer to the constant bitmap array.
 * @param w      Width of the bitmap in pixels.
 * @param h      Height of the bitmap in pixels.
 */
void drv_oled_ssd1306_draw_bitmap(ssd1306_obj_t *obj, int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h);

/**
 * @brief Draws a single ASCII character with dynamic interpolation scaling.
 * @param obj   Pointer to the OLED driver object instance.
 * @param x     X-coordinate.
 * @param y     Y-coordinate.
 * @param c     The ASCII character to draw.
 * @param scale The size multiplier.
 */
void drv_oled_ssd1306_draw_char(ssd1306_obj_t *obj, int16_t x, int16_t y, char c, oled_scale_e scale);

/**
 * @brief Prints a string into the SRAM framebuffer with dynamic scaling.
 * @param obj   Pointer to the OLED driver object instance.
 * @param x     Starting X-coordinate.
 * @param y     Starting Y-coordinate.
 * @param str   Pointer to the null-terminated string to print.
 * @param scale The size multiplier.
 */
void drv_oled_ssd1306_print(ssd1306_obj_t *obj, int16_t x, int16_t y, const char *str, oled_scale_e scale);

/**
 * @brief Pushes a specific page from the SRAM to the OLED via I2C.
 * @param obj  Pointer to the OLED driver object instance.
 * @param page The specific page index to update.
 */
void drv_oled_ssd1306_update_page(ssd1306_obj_t *obj, uint8_t page);

#endif /* __DRV_OLED_SSD1306_H__ */
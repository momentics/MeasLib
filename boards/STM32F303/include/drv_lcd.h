/**
 * @file drv_lcd.h
 * @brief Low-level LCD Driver API for STM32F303 (ILI9341/ST7789)
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Provides rudimentary drawing primitives and initialization for the display.
 * Designed to be used by the higher-level UI rendering system
 * (`meas_render_api_t`).
 */

#ifndef MAX_DRV_LCD_H
#define MAX_DRV_LCD_H

#include "measlib/drivers/hal.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize the LCD Driver (GPIO, SPI, DMA, Controller).
 *
 * This must be called after system clock initialization.
 * It will auto-detect the controller type (ILI9341 vs ST7789).
 * @return Pointer to the Display Driver Context
 */
void *meas_drv_lcd_init(void);

/**
 * @brief Get the Display HAL API VTable.
 * @return Pointer to the API VTable
 */
const meas_hal_display_api_t *meas_drv_lcd_get_api(void);

/**
 * @brief Set the active drawing window.
 *
 * @param ctx Driver Context
 * @param x Start X coordinate
 * @param y Start Y coordinate
 * @param w Width of the window
 * @param h Height of the window
 */
void meas_drv_lcd_set_window(void *ctx, uint16_t x, uint16_t y, uint16_t w,
                             uint16_t h);

/**
 * @brief Fill a rectangular area with a solid color.
 *
 * Uses DMA for high-speed filling. This function blocks until the
 * operation is complete (safe for shared bus usage).
 *
 * @param ctx Driver Context
 * @param x Start X coordinate
 * @param y Start Y coordinate
 * @param w Width of the rectangle
 * @param h Height of the rectangle
 * @param color 16-bit RGB565 color
 */
void meas_drv_lcd_fill_rect(void *ctx, uint16_t x, uint16_t y, uint16_t w,
                            uint16_t h, uint16_t color);

/**
 * @brief Draw a bitmap (Blit) to a rectangular area.
 *
 * Uses DMA to transfer the buffer. Blocks until completion.
 *
 * @param ctx Driver Context
 * @param x Start X coordinate
 * @param y Start Y coordinate
 * @param w Width of the rectangle
 * @param h Height of the rectangle
 * @param pixels Pointer to the source buffer (array of RGB565 colors).
 *               Must be accessible by DMA (SRAM).
 */
void meas_drv_lcd_blit(void *ctx, uint16_t x, uint16_t y, uint16_t w,
                       uint16_t h, const uint16_t *pixels);

/**
 * @brief Set the display orientation and subpixel order.
 *
 * Useful for handling ClearType subpixel rendering (RGB vs BGR).
 *
 * @param ctx Driver Context
 * @param rotation Rotation index (0-3)
 * @param bgr_order True for BGR, False for RGB.
 */
void meas_drv_lcd_set_orientation(void *ctx, uint8_t rotation, bool bgr_order);

/**
 * @brief Get the display width in pixels.
 * @param ctx Driver Context
 * @return Width
 */
uint16_t meas_drv_lcd_get_width(void *ctx);

/**
 * @brief Get the display height in pixels.
 * @param ctx Driver Context
 * @return Height
 */
uint16_t meas_drv_lcd_get_height(void *ctx);

#endif // MAX_DRV_LCD_H

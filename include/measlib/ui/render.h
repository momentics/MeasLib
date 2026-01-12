/**
 * @file render.h
 * @brief Graphics HAL (Hardware Abstraction Layer).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the low-level drawing interface that must be implemented by the
 * specific display driver (ILI9341, ST7789, etc).
 */

#ifndef MEASLIB_UI_RENDER_H
#define MEASLIB_UI_RENDER_H

#include "measlib/types.h"
#include <stdint.h>

/**
 * @brief Render Context
 * Describes the target buffer or clipping region for drawing operations.
 * Important for Tile Rendering strategies.
 */
typedef struct {
  meas_pixel_t *buffer;  /**< Pointer to framebuffer or tile buffer */
  int16_t width;         /**< Width of the drawing area */
  int16_t height;        /**< Height of the drawing area */
  int16_t x_offset;      /**< Global X offset (if drawing to a tile) */
  int16_t y_offset;      /**< Global Y offset (if drawing to a tile) */
  meas_pixel_t fg_color; /**< Current Foreground Color */
  meas_pixel_t bg_color; /**< Current Background Color */
  meas_rect_t clip_rect; /**< Drawing Clip Region */
} meas_render_ctx_t;

/**
 * @brief Drawing API (VTable)
 */
typedef struct {
  void (*draw_pixel)(meas_render_ctx_t *ctx, int16_t x, int16_t y);
  void (*draw_line)(meas_render_ctx_t *ctx, int16_t x0, int16_t y0, int16_t x1,
                    int16_t y1);
  void (*draw_polyline)(meas_render_ctx_t *ctx, const meas_point_t *points,
                        uint16_t count);
  void (*fill_rect)(meas_render_ctx_t *ctx, int16_t x, int16_t y, int16_t w,
                    int16_t h);
  void (*fill_polygon)(meas_render_ctx_t *ctx, const meas_point_t *points,
                       uint16_t count);
  void (*blit)(meas_render_ctx_t *ctx, int16_t x, int16_t y, int16_t w,
               int16_t h, const void *img);
  void (*draw_text)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                    const char *text);
  void (*fill_gradient_v)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                          int16_t w, int16_t h, meas_pixel_t c1,
                          meas_pixel_t c2);
  void (*fill_gradient_h)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                          int16_t w, int16_t h, meas_pixel_t c1,
                          meas_pixel_t c2);
  void (*get_dims)(meas_render_ctx_t *ctx, int16_t *w, int16_t *h);
  void (*set_clip_rect)(meas_render_ctx_t *ctx, meas_rect_t rect);
  void (*draw_rect)(meas_render_ctx_t *ctx, meas_rect_t rect);
  void (*draw_circle)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                      int16_t radius);
  void (*fill_circle)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                      int16_t radius);
} meas_render_api_t;

#endif // MEASLIB_UI_RENDER_H

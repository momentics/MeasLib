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
#include "measlib/ui/fonts.h"
#include <stdint.h>

/**
 * @brief Text Metrics
 */
typedef struct {
  int16_t width;
  int16_t height;
  int16_t ascent;  /**< Units above baseline */
  int16_t descent; /**< Units below baseline */
} meas_text_metrics_t;

/**
 * @brief Maximum depth of the Clipping Stack.
 */
#define MEAS_MAX_CLIP_STACK 8

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
  meas_rect_t
      clip_stack[MEAS_MAX_CLIP_STACK]; /**< Stack for nested clipping regions */
  uint8_t clip_stack_ptr;              /**< Current Stack Pointer */
  const meas_font_t *font;             /**< Active Font */
} meas_render_ctx_t;

/**
 * @brief Common Alpha Values (Opacity).
 * 0 = Fully Transparent, 255 = Fully Opaque.
 */
typedef enum {
  MEAS_ALPHA_TRANSPARENT = 0,
  MEAS_ALPHA_25 = 64,
  MEAS_ALPHA_50 = 127,
  MEAS_ALPHA_75 = 191,
  MEAS_ALPHA_OPAQUE = 255
} meas_alpha_t;

/**
 * @brief Text Alignment Flags.
 * Combine Horizontal and Vertical flags using OR.
 * Defaults: LEFT | TOP.
 */
typedef enum {
  MEAS_ALIGN_LEFT = 0x00,   /**< Align text to the Left (Default) */
  MEAS_ALIGN_CENTER = 0x01, /**< Center text horizontally */
  MEAS_ALIGN_RIGHT = 0x02,  /**< Align text to the Right */

  MEAS_ALIGN_TOP = 0x00,     /**< Align text to the Top (Default) */
  MEAS_ALIGN_VCENTER = 0x04, /**< Center text vertically */
  MEAS_ALIGN_BOTTOM = 0x08   /**< Align text to the Bottom */
} meas_align_t;

/**
 * @brief Line Patterns.
 * 8-bit pattern repeated along the line.
 * 1 = Pixel Drawn, 0 = Pixel Skipped (Transparent).
 */
typedef enum {
  MEAS_PATTERN_SOLID = 0xFF, /**< Solid Line (11111111) */
  MEAS_PATTERN_DOT = 0xAA,   /**< Dotted Line (10101010) */
  MEAS_PATTERN_DASH = 0xCC,  /**< Dashed Line (11001100) */
  MEAS_PATTERN_DASH_DOT =
      0xF8 /**< Dash-Dot (11111000... repeats 8px?) No, Fixed pattern 8 bits */
} meas_line_pattern_t;

// NOTE: Standard dash-dot usually requires more than 8 bits for good look, but
// we stick to 8-bit cycle for efficiency. Better Dash-Dot: 11100100 -> E4

typedef struct {
  void (*draw_pixel)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                     uint8_t alpha);
  meas_pixel_t (*get_pixel)(meas_render_ctx_t *ctx, int16_t x, int16_t y);

  void (*draw_line)(meas_render_ctx_t *ctx, int16_t x0, int16_t y0, int16_t x1,
                    int16_t y1, uint8_t alpha);
  void (*draw_polyline)(meas_render_ctx_t *ctx, const meas_point_t *points,
                        uint16_t count, uint8_t alpha);
  void (*fill_rect)(meas_render_ctx_t *ctx, int16_t x, int16_t y, int16_t w,
                    int16_t h, uint8_t alpha);
  void (*fill_polygon)(meas_render_ctx_t *ctx, const meas_point_t *points,
                       uint16_t count, uint8_t alpha);
  void (*blit)(meas_render_ctx_t *ctx, int16_t x, int16_t y, int16_t w,
               int16_t h, const void *img, uint8_t alpha);
  void (*draw_text)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                    const char *text, uint8_t alpha);
  void (*fill_gradient_v)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                          int16_t w, int16_t h, meas_pixel_t c1,
                          meas_pixel_t c2, uint8_t alpha);
  void (*fill_gradient_h)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                          int16_t w, int16_t h, meas_pixel_t c1,
                          meas_pixel_t c2, uint8_t alpha);
  void (*get_dims)(meas_render_ctx_t *ctx, int16_t *w, int16_t *h);
  void (*draw_rect)(meas_render_ctx_t *ctx, meas_rect_t rect, uint8_t alpha);
  void (*draw_circle)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                      int16_t radius, uint8_t alpha);
  void (*fill_circle)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                      int16_t radius, uint8_t alpha);
  void (*draw_round_rect)(meas_render_ctx_t *ctx, meas_rect_t rect, int16_t r,
                          uint8_t alpha);
  void (*fill_round_rect)(meas_render_ctx_t *ctx, meas_rect_t rect, int16_t r,
                          uint8_t alpha);

  // Font API
  void (*set_font)(meas_render_ctx_t *ctx, const meas_font_t *font);
  int16_t (*get_text_width)(meas_render_ctx_t *ctx, const char *text);
  int16_t (*get_text_height)(meas_render_ctx_t *ctx, const char *text);
  void (*measure_text)(meas_render_ctx_t *ctx, const char *text,
                       meas_text_metrics_t *out_metrics);

  // Extended API
  void (*draw_text_rotated)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                            const char *text, int16_t angle, uint8_t alpha);
  void (*draw_text_aligned)(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                            const char *text, uint8_t align, uint8_t alpha);
  void (*draw_text_rect)(meas_render_ctx_t *ctx, meas_rect_t rect,
                         const char *text, uint8_t align, uint8_t alpha);
  void (*invert_rect)(meas_render_ctx_t *ctx, int16_t x, int16_t y, int16_t w,
                      int16_t h);
  meas_rect_t (*get_clip_rect)(meas_render_ctx_t *ctx);
  void (*draw_line_patt)(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                         int16_t x1, int16_t y1, uint8_t pattern,
                         uint8_t alpha);
  void (*draw_line_thick)(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                          int16_t x1, int16_t y1, uint8_t width, uint8_t alpha);

  // Stack Clipping API
  void (*push_clip_rect)(meas_render_ctx_t *ctx, meas_rect_t rect);
  meas_rect_t (*pop_clip_rect)(meas_render_ctx_t *ctx);

  // Triangle API
  void (*fill_triangle)(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                        int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                        uint8_t alpha);
  void (*draw_triangle)(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                        int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                        uint8_t alpha);

  // Arcs & Sectors
  void (*draw_arc)(meas_render_ctx_t *ctx, int16_t x, int16_t y, int16_t r,
                   int16_t start_angle, int16_t end_angle, uint8_t alpha);
  void (*fill_pie)(meas_render_ctx_t *ctx, int16_t x, int16_t y, int16_t r,
                   int16_t start_angle, int16_t end_angle, uint8_t alpha);
} meas_render_api_t;

#endif // MEASLIB_UI_RENDER_H

/**
 * @file render_std.c
 * @brief Standard Software Rasterizer Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Provides generic rasterization primitives (line, rect, blit) that operate
 * directly on a meas_render_ctx_t buffer (Tile/Framebuffer).
 */

#include "measlib/ui/render.h"
#include <stdlib.h> // abs
#include <string.h> // memcpy

// --- Primitives (Cell/Tile Operations) ---

static void cell_draw_pixel(meas_render_ctx_t *ctx, int16_t x, int16_t y) {
  if (!ctx || !ctx->buffer)
    return;
  int16_t lx = x - ctx->x_offset;
  int16_t ly = y - ctx->y_offset;

  if (lx >= 0 && lx < ctx->width && ly >= 0 && ly < ctx->height) {
    ctx->buffer[ly * ctx->width + lx] = ctx->fg_color;
  }
}

static void cell_draw_line(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1) {
  if (!ctx || !ctx->buffer)
    return;

  // Optimization: Trivial Rejection for full line?
  // Bresenham
  int16_t dx = abs(x1 - x0);
  int16_t dy = abs(y1 - y0);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = (dx > dy ? dx : -dy) / 2;
  int16_t e2;

  while (1) {
    cell_draw_pixel(ctx, x0, y0);
    if (x0 == x1 && y0 == y1)
      break;
    e2 = err;
    if (e2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dy) {
      err += dx;
      y0 += sy;
    }
  }
}

static void cell_fill_rect(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                           int16_t w, int16_t h) {
  if (!ctx || !ctx->buffer)
    return;

  int16_t lx = x - ctx->x_offset;
  int16_t ly = y - ctx->y_offset;

  if (lx >= ctx->width || ly >= ctx->height || (lx + w) <= 0 || (ly + h) <= 0)
    return;

  if (lx < 0) {
    w += lx;
    lx = 0;
  }
  if (ly < 0) {
    h += ly;
    ly = 0;
  }
  if (lx + w > ctx->width)
    w = ctx->width - lx;
  if (ly + h > ctx->height)
    h = ctx->height - ly;

  if (w <= 0 || h <= 0)
    return;

  meas_pixel_t *row = &ctx->buffer[ly * ctx->width + lx];
  meas_pixel_t color = ctx->fg_color;

  for (int16_t i = 0; i < h; i++) {
    for (int16_t j = 0; j < w; j++) {
      row[j] = color;
    }
    row += ctx->width;
  }
}

static void cell_blit(meas_render_ctx_t *ctx, int16_t x, int16_t y, int16_t w,
                      int16_t h, const void *img) {
  if (!ctx || !ctx->buffer || !img)
    return;

  int16_t lx = x - ctx->x_offset;
  int16_t ly = y - ctx->y_offset;
  const meas_pixel_t *src = (const meas_pixel_t *)img;

  if (lx >= ctx->width || ly >= ctx->height || (lx + w) <= 0 || (ly + h) <= 0)
    return;

  // X-Clip Source skipping logic needed for correctness if we have sprite
  // source
  int16_t src_skip_x = 0;

  if (lx < 0) {
    src_skip_x = -lx;
    w += lx;
    lx = 0;
  }
  if (ly < 0) {
    // Vertical clip: Skip rows in source
    // Assuming source is exactly 'original w' wide.
    // If we assume the passed 'w' is the width of the image, then we skip w *
    // -ly Use the original (unclipped) w for stride
    src += (-ly) * w;
    h += ly;
    ly = 0;
  }
  if (lx + w > ctx->width)
    w = ctx->width - lx;
  if (ly + h > ctx->height)
    h = ctx->height - ly;

  if (w <= 0 || h <= 0)
    return;

  // Stride of Source is strictly assumed to be the original width 'w' passed to
  // the function? The API `blit(ctx, x, y, w, h, img)` usually means: draw IMG
  // of size WxH at X,Y. So Source Stride = (Argument W). If we clipped X, we
  // must skip `src_skip_x` pixels at start of row, AND skip `(Original W -
  // Clipped W)` at end of row. Let Original W = O_W.  Argument W = O_W. Clipped
  // W = C_W.

  // Re-read argument `w`: "int16_t w, int16_t h".
  // Argument w is the Width of the Image AND the Width of the Rect to Draw.
  int16_t src_stride = w; // The W passed by caller is the Image Width.

  // Adjust src pointer for X-clip
  src += src_skip_x;

  meas_pixel_t *dst = &ctx->buffer[ly * ctx->width + lx];

  for (int16_t i = 0; i < h; i++) {
    memcpy(dst, src, w * sizeof(meas_pixel_t)); // w is the CLIPPED width
    dst += ctx->width;
    src += src_stride; // Jump by ORIGINAL width
  }
}

static void cell_get_dims(meas_render_ctx_t *ctx, int16_t *w, int16_t *h) {
  if (w)
    *w = ctx->width;
  if (h)
    *h = ctx->height;
}

// --- API Definition ---

const meas_render_api_t meas_render_cell_api = {.draw_pixel = cell_draw_pixel,
                                                .draw_line = cell_draw_line,
                                                .fill_rect = cell_fill_rect,
                                                .blit = cell_blit,
                                                .draw_text =
                                                    NULL, // TODO: Font System
                                                .get_dims = cell_get_dims};

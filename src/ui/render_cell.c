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

// --- Gradient Primitives ---

static inline meas_pixel_t lerp_color(meas_pixel_t c1, meas_pixel_t c2,
                                      uint8_t ratio) {
  // RGB565: R(5) G(6) B(5)
  // Extract
  uint16_t r1 = (c1 >> 11) & 0x1F;
  uint16_t g1 = (c1 >> 5) & 0x3F;
  uint16_t b1 = c1 & 0x1F;

  uint16_t r2 = (c2 >> 11) & 0x1F;
  uint16_t g2 = (c2 >> 5) & 0x3F;
  uint16_t b2 = c2 & 0x1F;

  // Mix
  // ratio is 0..255. 0 = c1, 255 = c2.
  uint16_t r = r1 + ((int32_t)(r2 - r1) * ratio) / 255;
  uint16_t g = g1 + ((int32_t)(g2 - g1) * ratio) / 255;
  uint16_t b = b1 + ((int32_t)(b2 - b1) * ratio) / 255;

  return (r << 11) | (g << 5) | b;
}

static void cell_fill_gradient_v(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                                 int16_t w, int16_t h, meas_pixel_t c1,
                                 meas_pixel_t c2) {
  if (!ctx || !ctx->buffer)
    return;

  int16_t lx = x - ctx->x_offset;
  int16_t ly = y - ctx->y_offset;

  int16_t full_h = h; // Needed for interpolation

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

  for (int16_t i = 0; i < h; i++) {
    // Current Global Y of this row
    int16_t current_global_y = ctx->y_offset + ly + i;
    // Position within the original rect
    int16_t relative_y = current_global_y - y;

    // Ratio 0..255
    uint8_t ratio = (relative_y * 255) / (full_h - 1 > 0 ? full_h - 1 : 1);

    meas_pixel_t col = lerp_color(c1, c2, ratio);

    for (int16_t j = 0; j < w; j++) {
      row[j] = col;
    }
    row += ctx->width;
  }
}

static void cell_fill_gradient_h(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                                 int16_t w, int16_t h, meas_pixel_t c1,
                                 meas_pixel_t c2) {
  if (!ctx || !ctx->buffer)
    return;

  int16_t lx = x - ctx->x_offset;
  int16_t ly = y - ctx->y_offset;

  int16_t full_w = w;

  if (lx >= ctx->width || ly >= ctx->height || (lx + w) <= 0 || (ly + h) <= 0)
    return;

  // Offset into Gradient Logic if Clipped Left
  int16_t grad_x_skip = 0;

  if (lx < 0) {
    grad_x_skip = -lx;
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

  // Pre-calculate X line? No, just loop.
  // Optimization: Pre-calculate the row buffer since it's same for all rows?

  meas_pixel_t *row_start = &ctx->buffer[ly * ctx->width + lx];

  for (int16_t i = 0; i < h; i++) {
    meas_pixel_t *p = row_start;
    for (int16_t j = 0; j < w; j++) {
      int16_t relative_x = grad_x_skip + j;
      uint8_t ratio = (relative_x * 255) / (full_w - 1 > 0 ? full_w - 1 : 1);
      *p++ = lerp_color(c1, c2, ratio);
    }
    row_start += ctx->width;
  }
}

static void cell_get_dims(meas_render_ctx_t *ctx, int16_t *w, int16_t *h) {
  if (w)
    *w = ctx->width;
  if (h)
    *h = ctx->height;
}
}

// --- Poly Primitives ---

static void cell_draw_polyline(meas_render_ctx_t *ctx,
                               const meas_point_t *points, uint16_t count) {
  if (!ctx || !points || count < 2)
    return;

  for (uint16_t i = 0; i < count - 1; i++) {
    cell_draw_line(ctx, points[i].x, points[i].y, points[i + 1].x,
                   points[i + 1].y);
  }
}

// Comparison function for qsort
static int compare_nodes(const void *a, const void *b) {
  return (*(int16_t *)a - *(int16_t *)b);
}

// Scanline Polygon Fill
static void cell_fill_polygon(meas_render_ctx_t *ctx,
                              const meas_point_t *points, uint16_t count) {
  if (!ctx || !ctx->buffer || !points || count < 3)
    return;

  // Global Y range for this Tile
  int16_t global_y_start = ctx->y_offset;
  int16_t global_y_end = ctx->y_offset + ctx->height;

  // Limits for polygon (Bounding Box check could optimize)
  int16_t poly_min_y = points[0].y;
  int16_t poly_max_y = points[0].y;
  for (uint16_t i = 1; i < count; i++) {
    if (points[i].y < poly_min_y)
      poly_min_y = points[i].y;
    if (points[i].y > poly_max_y)
      poly_max_y = points[i].y;
  }

  // Intersect Y Loop ranges
  int16_t start_y = (global_y_start > poly_min_y) ? global_y_start : poly_min_y;
  int16_t end_y =
      (global_y_end < poly_max_y + 1) ? global_y_end : poly_max_y + 1;

  if (start_y >= end_y)
    return;

// Node buffer (Stack allocated)
// Max nodes per scanline for simple UI polygons usually small.
// 16 is sufficient for simple convex/concave shapes.
#define MAX_POLY_NODES 16
  int16_t nodes[MAX_POLY_NODES];

  for (int16_t y = start_y; y < end_y; y++) {
    int node_cnt = 0;
    uint16_t j = count - 1;

    // Build Nodes
    for (uint16_t i = 0; i < count; i++) {
      int16_t y1 = points[i].y;
      int16_t y2 = points[j].y;
      int16_t x1 = points[i].x;
      int16_t x2 = points[j].x;

      if ((y1 < y && y2 >= y) || (y2 < y && y1 >= y)) {
        if (node_cnt < MAX_POLY_NODES) {
          // X intersection
          // x = x1 + (y - y1) * (x2 - x1) / (y2 - y1)
          // Use long for intermediate calc to avoid overflow
          nodes[node_cnt++] =
              (int16_t)(x1 + ((int32_t)(y - y1) * (x2 - x1)) / (y2 - y1));
        }
      }
      j = i;
    }

    // Sort Nodes
    qsort(nodes, node_cnt, sizeof(int16_t), compare_nodes);

    // Fill Pairs
    // We are drawing into a TILE. So we must map Global Y -> Tile Relative Y
    int16_t tile_y = y - ctx->y_offset;
    meas_pixel_t *row = &ctx->buffer[tile_y * ctx->width];
    meas_pixel_t color = ctx->fg_color;
    int16_t width = ctx->width;

    for (int k = 0; k < node_cnt; k += 2) {
      if (k + 1 >= node_cnt)
        break;
      int16_t x_start = nodes[k];
      int16_t x_end = nodes[k + 1];

      if (x_start >= width)
        continue;
      if (x_end <= 0)
        continue;

      if (x_start < 0)
        x_start = 0;
      if (x_end > width)
        x_end = width;

      for (int16_t x = x_start; x < x_end; x++) {
        row[x] = color;
      }
    }
  }
}

static void cell_get_dims(meas_render_ctx_t *ctx, int16_t *w, int16_t *h) {
  if (w)
    *w = ctx->width;
  if (h)
    *h = ctx->height;
}

// --- API Definition ---

const meas_render_api_t meas_render_cell_api = {
    .draw_pixel = cell_draw_pixel,
    .draw_line = cell_draw_line,
    .draw_polyline = cell_draw_polyline,
    .fill_rect = cell_fill_rect,
    .fill_polygon = cell_fill_polygon,
    .blit = cell_blit,
    .draw_text = NULL, // TODO: Font System
    .fill_gradient_v = cell_fill_gradient_v,
    .fill_gradient_h = cell_fill_gradient_h,
    .get_dims = cell_get_dims};

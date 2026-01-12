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

// --- Clipping Helpers ---

static bool is_inside_clip(meas_render_ctx_t *ctx, int16_t x, int16_t y) {
  if (x < ctx->clip_rect.x || x >= ctx->clip_rect.x + ctx->clip_rect.w)
    return false;
  if (y < ctx->clip_rect.y || y >= ctx->clip_rect.y + ctx->clip_rect.h)
    return false;
  return true;
}

// --- Clip Stack ---

static void cell_push_clip_rect(meas_render_ctx_t *ctx, meas_rect_t rect) {
  if (!ctx || ctx->clip_stack_ptr >= MEAS_MAX_CLIP_STACK)
    return;

  // Save current clip
  ctx->clip_stack[ctx->clip_stack_ptr++] = ctx->clip_rect;

  // Intersect new with current
  int16_t new_x = (rect.x > ctx->clip_rect.x) ? rect.x : ctx->clip_rect.x;
  int16_t new_y = (rect.y > ctx->clip_rect.y) ? rect.y : ctx->clip_rect.y;
  int16_t new_r = (rect.x + rect.w < ctx->clip_rect.x + ctx->clip_rect.w)
                      ? rect.x + rect.w
                      : ctx->clip_rect.x + ctx->clip_rect.w;
  int16_t new_b = (rect.y + rect.h < ctx->clip_rect.y + ctx->clip_rect.h)
                      ? rect.y + rect.h
                      : ctx->clip_rect.y + ctx->clip_rect.h;

  if (new_x < new_r && new_y < new_b) {
    ctx->clip_rect = (meas_rect_t){new_x, new_y, (int16_t)(new_r - new_x),
                                   (int16_t)(new_b - new_y)};
  } else {
    // Empty clip
    ctx->clip_rect = (meas_rect_t){0, 0, 0, 0};
  }
}

static meas_rect_t cell_pop_clip_rect(meas_render_ctx_t *ctx) {
  if (!ctx || ctx->clip_stack_ptr == 0) {
    // If stack empty, return current (or empty? Current is safer as no-op)
    // Assuming no-op if underflow.
    if (ctx)
      return ctx->clip_rect;
    return (meas_rect_t){0, 0, 0, 0};
  }

  ctx->clip_rect = ctx->clip_stack[--ctx->clip_stack_ptr];
  return ctx->clip_rect;
}

// --- Primitives (Cell/Tile Operations) ---

// --- Helpers ---

static inline meas_pixel_t alpha_blend(meas_pixel_t bg, meas_pixel_t fg,
                                       uint8_t alpha) {
  if (alpha == MEAS_ALPHA_TRANSPARENT)
    return bg;
  if (alpha == MEAS_ALPHA_OPAQUE)
    return fg;

  // Extract RGB (RGB565)
  uint32_t r_bg = (bg >> 11) & 0x1F;
  uint32_t g_bg = (bg >> 5) & 0x3F;
  uint32_t b_bg = bg & 0x1F;

  uint32_t r_fg = (fg >> 11) & 0x1F;
  uint32_t g_fg = (fg >> 5) & 0x3F;
  uint32_t b_fg = fg & 0x1F;

  // Mix: Result = (FG * Alpha + BG * (255 - Alpha)) / 255
  // Approx: >> 8 (divide by 256)
  uint32_t inv_alpha = 256 - alpha;

  uint32_t r = (r_fg * alpha + r_bg * inv_alpha) >> 8;
  uint32_t g = (g_fg * alpha + g_bg * inv_alpha) >> 8;
  uint32_t b = (b_fg * alpha + b_bg * inv_alpha) >> 8; // Typo fix: b_fg

  return (meas_pixel_t)((r << 11) | (g << 5) | b);
}

static void cell_draw_pixel(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                            uint8_t alpha) {
  if (!ctx || !ctx->buffer)
    return;
  if (alpha == MEAS_ALPHA_TRANSPARENT)
    return;

  // Clip (Global)
  if (!is_inside_clip(ctx, x, y))
    return;

  int16_t lx = x - ctx->x_offset;
  int16_t ly = y - ctx->y_offset;

  if (lx >= 0 && lx < ctx->width && ly >= 0 && ly < ctx->height) {
    if (alpha == MEAS_ALPHA_OPAQUE) {
      ctx->buffer[ly * ctx->width + lx] = ctx->fg_color;
    } else {
      meas_pixel_t bg = ctx->buffer[ly * ctx->width + lx];
      ctx->buffer[ly * ctx->width + lx] = alpha_blend(bg, ctx->fg_color, alpha);
    }
  }
}

static meas_pixel_t cell_get_pixel(meas_render_ctx_t *ctx, int16_t x,
                                   int16_t y) {
  if (!ctx || !ctx->buffer)
    return 0;

  // Clip (Global)
  if (!is_inside_clip(ctx, x, y))
    return 0;

  int16_t lx = x - ctx->x_offset;
  int16_t ly = y - ctx->y_offset;

  if (lx >= 0 && lx < ctx->width && ly >= 0 && ly < ctx->height) {
    return ctx->buffer[ly * ctx->width + lx];
  }
  return 0;
}

static void cell_draw_line(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1, uint8_t alpha) {
  if (!ctx || !ctx->buffer)
    return;

  // Optimization: Trivial Rejection against Clip Rect?
  // Using Cohen-Sutherland ideas or just pixel check for now.
  // Pixel check is safest for robust implementation in short time.

  // Bresenham
  int16_t dx = abs(x1 - x0);
  int16_t dy = abs(y1 - y0);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = (dx > dy ? dx : -dy) / 2;
  int16_t e2;

  while (1) {
    cell_draw_pixel(ctx, x0, y0, alpha);
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

static void cell_draw_rect(meas_render_ctx_t *ctx, meas_rect_t rect,
                           uint8_t alpha) {
  int16_t x = rect.x;
  int16_t y = rect.y;
  int16_t w = rect.w;
  int16_t h = rect.h;

  cell_draw_line(ctx, x, y, x + w - 1, y, alpha);                 // Top
  cell_draw_line(ctx, x, y + h - 1, x + w - 1, y + h - 1, alpha); // Bottom
  cell_draw_line(ctx, x, y, x, y + h - 1, alpha);                 // Left
  cell_draw_line(ctx, x + w - 1, y, x + w - 1, y + h - 1, alpha); // Right
}

static void cell_fill_rect(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                           int16_t w, int16_t h, uint8_t alpha) {
  if (!ctx || !ctx->buffer)
    return;
  if (alpha == MEAS_ALPHA_TRANSPARENT)
    return;

  // Clip to global clip_rect first
  int16_t cx = ctx->clip_rect.x;
  int16_t cy = ctx->clip_rect.y;
  int16_t cw = ctx->clip_rect.w;
  int16_t ch = ctx->clip_rect.h;

  // Intersect (x,y,w,h) with (cx,cy,cw,ch)
  if (x < cx) {
    w -= (cx - x);
    x = cx;
  }
  if (y < cy) {
    h -= (cy - y);
    y = cy;
  }
  if (x + w > cx + cw)
    w = cx + cw - x;
  if (y + h > cy + ch)
    h = cy + ch - y;

  if (w <= 0 || h <= 0)
    return;

  // Now clip to TILE bounds
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
    if (alpha == MEAS_ALPHA_OPAQUE) {
      // Fast Path
      for (int16_t j = 0; j < w; j++) {
        row[j] = color;
      }
    } else {
      // Blend Path
      for (int16_t j = 0; j < w; j++) {
        row[j] = alpha_blend(row[j], color, alpha);
      }
    }
    row += ctx->width;
  }
}

// Circle Helper
static void draw_circle_points(meas_render_ctx_t *ctx, int16_t xc, int16_t yc,
                               int16_t x, int16_t y, uint8_t alpha) {
  cell_draw_pixel(ctx, xc + x, yc + y, alpha);
  cell_draw_pixel(ctx, xc - x, yc + y, alpha);
  cell_draw_pixel(ctx, xc + x, yc - y, alpha);
  cell_draw_pixel(ctx, xc - x, yc - y, alpha);
  cell_draw_pixel(ctx, xc + y, yc + x, alpha);
  cell_draw_pixel(ctx, xc - y, yc + x, alpha);
  cell_draw_pixel(ctx, xc + y, yc - x, alpha);
  cell_draw_pixel(ctx, xc - y, yc - x, alpha);
}

static void cell_draw_circle(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                             int16_t r, uint8_t alpha) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  cell_draw_pixel(ctx, x0, y0 + r, alpha);
  cell_draw_pixel(ctx, x0, y0 - r, alpha);
  cell_draw_pixel(ctx, x0 + r, y0, alpha);
  cell_draw_pixel(ctx, x0 - r, y0, alpha);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    draw_circle_points(ctx, x0, y0, x, y, alpha);
  }
}

static void cell_fill_circle(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                             int16_t r, uint8_t alpha) {
  int16_t x = 0;
  int16_t y = r;
  int16_t d = 3 - 2 * r;

  while (y >= x) {
    // Scanlines
    cell_fill_rect(ctx, x0 - x, y0 - y, 2 * x + 1, 1, alpha);
    cell_fill_rect(ctx, x0 - x, y0 + y, 2 * x + 1, 1, alpha);
    cell_fill_rect(ctx, x0 - y, y0 - x, 2 * y + 1, 1, alpha);
    cell_fill_rect(ctx, x0 - y, y0 + x, 2 * y + 1, 1, alpha);

    x++;
    if (d > 0) {
      y--;
      d = d + 4 * (x - y) + 10;
    } else {
      d = d + 4 * x + 6;
    }
  }
}

static void cell_blit(meas_render_ctx_t *ctx, int16_t x, int16_t y, int16_t w,
                      int16_t h, const void *img, uint8_t alpha) {
  if (!ctx || !ctx->buffer || !img)
    return;
  if (alpha == MEAS_ALPHA_TRANSPARENT)
    return;

  const uint16_t *src = (const uint16_t *)img;

  // Simple unclipped blit to tile (optimization needed)
  int16_t ly_start = y - ctx->y_offset;
  int16_t ly_end = ly_start + h;

  // Intersect with Tile Y
  if (ly_start < 0) {
    src += (-ly_start) * w; // Skip source lines
    ly_start = 0;
  }
  if (ly_end > ctx->height)
    ly_end = ctx->height;

  int16_t lx_start = x - ctx->x_offset;
  int16_t copy_w = w;

  // Intersect with Tile X
  int16_t src_x_offset = 0;
  if (lx_start < 0) {
    src_x_offset = -lx_start;
    copy_w += lx_start;
    lx_start = 0;
  }
  if (lx_start + copy_w > ctx->width) {
    copy_w = ctx->width - lx_start;
  }

  if (copy_w <= 0 || ly_start >= ly_end)
    return;

  for (int16_t i = ly_start; i < ly_end; i++) {
    meas_pixel_t *dst = &ctx->buffer[i * ctx->width + lx_start];
    const uint16_t *src_line = src + src_x_offset;

    if (alpha == MEAS_ALPHA_OPAQUE) {
      memcpy(dst, src_line, copy_w * sizeof(meas_pixel_t));
    } else {
      for (int16_t j = 0; j < copy_w; j++) {
        dst[j] = alpha_blend(dst[j], src_line[j], alpha);
      }
    }
    src += w; // Next source line
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
                                 meas_pixel_t c2, uint8_t alpha) {
  if (!ctx || !ctx->buffer)
    return;

  // Clip to global clip_rect
  int16_t clip_x = ctx->clip_rect.x;
  int16_t clip_y = ctx->clip_rect.y;
  int16_t clip_w = ctx->clip_rect.w;
  int16_t clip_h = ctx->clip_rect.h;

  int16_t orig_h = h;
  int16_t orig_y = y;

  if (x < clip_x) {
    w -= (clip_x - x);
    x = clip_x;
  }
  if (y < clip_y) {
    h -= (clip_y - y);
    y = clip_y;
  }
  if (x + w > clip_x + clip_w)
    w = clip_x + clip_w - x;
  if (y + h > clip_y + clip_h)
    h = clip_y + clip_h - y;

  if (w <= 0 || h <= 0)
    return;

  // Tile intersection
  int16_t ly_start = y - ctx->y_offset;
  int16_t ly_end = ly_start + h;

  if (ly_start < 0)
    ly_start = 0;
  if (ly_end > ctx->height)
    ly_end = ctx->height;

  int16_t lx_start = x - ctx->x_offset;
  int16_t lx_end = lx_start + w;

  if (lx_start < 0)
    lx_start = 0;
  if (lx_end > ctx->width)
    lx_end = ctx->width;

  if (lx_start >= lx_end || ly_start >= ly_end)
    return;

  for (int16_t i = ly_start; i < ly_end; i++) {
    int16_t global_cur_y = ctx->y_offset + i;
    int16_t relative_y = global_cur_y - orig_y;

    uint8_t ratio = (relative_y * 255) / (orig_h - 1 > 0 ? orig_h - 1 : 1);
    meas_pixel_t grad_col = lerp_color(c1, c2, ratio);

    meas_pixel_t *row = &ctx->buffer[i * ctx->width];
    for (int16_t rx = lx_start; rx < lx_end; rx++) {
      if (alpha == MEAS_ALPHA_OPAQUE) {
        row[rx] = grad_col;
      } else {
        row[rx] = alpha_blend(row[rx], grad_col, alpha);
      }
    }
  }
}

static void cell_fill_gradient_h(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                                 int16_t w, int16_t h, meas_pixel_t c1,
                                 meas_pixel_t c2, uint8_t alpha) {
  if (!ctx || !ctx->buffer)
    return;

  // Clip to global clip_rect
  int16_t clip_x = ctx->clip_rect.x;
  int16_t clip_y = ctx->clip_rect.y;
  int16_t clip_w = ctx->clip_rect.w;
  int16_t clip_h = ctx->clip_rect.h;

  int16_t orig_w = w;
  int16_t orig_x = x;

  if (x < clip_x) {
    w -= (clip_x - x);
    x = clip_x;
  }
  if (y < clip_y) {
    h -= (clip_y - y);
    y = clip_y;
  }
  if (x + w > clip_x + clip_w)
    w = clip_x + clip_w - x;
  if (y + h > clip_y + clip_h)
    h = clip_y + clip_h - y;

  if (w <= 0 || h <= 0)
    return;

  // Tile intersection
  int16_t ly_start = y - ctx->y_offset;
  int16_t ly_end = ly_start + h;

  if (ly_start < 0)
    ly_start = 0;
  if (ly_end > ctx->height)
    ly_end = ctx->height;

  int16_t lx_start = x - ctx->x_offset;
  int16_t lx_end = lx_start + w;

  if (lx_start < 0)
    lx_start = 0;
  if (lx_end > ctx->width)
    lx_end = ctx->width;

  if (lx_start >= lx_end || ly_start >= ly_end)
    return;

  for (int16_t i = ly_start; i < ly_end; i++) {
    meas_pixel_t *row = &ctx->buffer[i * ctx->width];
    for (int16_t rx = lx_start; rx < lx_end; rx++) {
      int16_t global_cur_x = ctx->x_offset + rx;
      int16_t relative_x = global_cur_x - orig_x;
      uint8_t ratio = (relative_x * 255) / (orig_w - 1 > 0 ? orig_w - 1 : 1);
      meas_pixel_t grad_col = lerp_color(c1, c2, ratio);

      if (alpha == MEAS_ALPHA_OPAQUE) {
        row[rx] = grad_col;
      } else {
        row[rx] = alpha_blend(row[rx], grad_col, alpha);
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

// --- Poly Primitives ---

static void cell_draw_polyline(meas_render_ctx_t *ctx,
                               const meas_point_t *points, uint16_t count,
                               uint8_t alpha) {
  if (!ctx || !points || count < 2)
    return;

  for (uint16_t i = 0; i < count - 1; i++) {
    cell_draw_line(ctx, points[i].x, points[i].y, points[i + 1].x,
                   points[i + 1].y, alpha);
  }
}

// Comparison function for qsort
static int compare_nodes(const void *a, const void *b) {
  return (*(int16_t *)a - *(int16_t *)b);
}

// Scanline Polygon Fill
static void cell_fill_polygon(meas_render_ctx_t *ctx,
                              const meas_point_t *points, uint16_t count,
                              uint8_t alpha) {
  if (!ctx || !ctx->buffer || !points || count < 3)
    return;
  if (alpha == MEAS_ALPHA_TRANSPARENT)
    return;

  // Global Y range for this Tile AND Clip Rect
  int16_t global_y_start = ctx->y_offset;
  int16_t global_y_end = ctx->y_offset + ctx->height;

  if (global_y_start < ctx->clip_rect.y)
    global_y_start = ctx->clip_rect.y;
  if (global_y_end > ctx->clip_rect.y + ctx->clip_rect.h)
    global_y_end = ctx->clip_rect.y + ctx->clip_rect.h;

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

    // Fill Pairs with Clipping
    int16_t tile_y = y - ctx->y_offset;
    meas_pixel_t *row = &ctx->buffer[tile_y * ctx->width];
    meas_pixel_t color = ctx->fg_color;
    int16_t width = ctx->width;

    // Global Clip Range for X
    int16_t clip_x_min = ctx->clip_rect.x;
    int16_t clip_x_max = ctx->clip_rect.x + ctx->clip_rect.w; // Exclusive

    for (int k = 0; k < node_cnt; k += 2) {
      if (k + 1 >= node_cnt)
        break;
      int16_t x_start = nodes[k];
      int16_t x_end = nodes[k + 1];

      // Clip to logical clip rect
      if (x_start < clip_x_min)
        x_start = clip_x_min;
      if (x_end > clip_x_max)
        x_end = clip_x_max;
      if (x_start >= x_end)
        continue;

      // Clip to Tile relative X
      int16_t lx_start = x_start - ctx->x_offset;
      int16_t lx_end = x_end - ctx->x_offset;

      if (lx_start >= width)
        continue;
      if (lx_end <= 0)
        continue;

      if (lx_start < 0)
        lx_start = 0;
      if (lx_end > width)
        lx_end = width;

      for (int16_t x = lx_start; x < lx_end; x++) {
        if (alpha == MEAS_ALPHA_OPAQUE) {
          row[x] = color;
        } else {
          row[x] = alpha_blend(row[x], color, alpha);
        }
      }
    }
  }
}

// --- Triangle Primitives ---

static void cell_fill_triangle(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                               int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                               uint8_t alpha) {
  if (!ctx || !ctx->buffer)
    return;

  // Sort by Y: y0 <= y1 <= y2
  if (y0 > y1) {
    int16_t t;
    t = y0;
    y0 = y1;
    y1 = t;
    t = x0;
    x0 = x1;
    x1 = t;
  }
  if (y0 > y2) {
    int16_t t;
    t = y0;
    y0 = y2;
    y2 = t;
    t = x0;
    x0 = x2;
    x2 = t;
  }
  if (y1 > y2) {
    int16_t t;
    t = y1;
    y1 = y2;
    y2 = t;
    t = x1;
    x1 = x2;
    x2 = t;
  }

  // Check visibility (Y range)
  if (y2 < ctx->clip_rect.y || y0 >= ctx->clip_rect.y + ctx->clip_rect.h)
    return;

  int16_t total_height = y2 - y0;
  if (total_height == 0)
    return;

  int16_t y;
  for (y = y0; y <= y2; y++) {
    // Skip if off-screen Y
    if (y < ctx->clip_rect.y || y >= ctx->clip_rect.y + ctx->clip_rect.h)
      continue;

    // Standard scanline logic
    // Segment A: V0 -> V2 (Long side)
    // Segment B: V0 -> V1 (Top half short
    // side) OR V1 -> V2 (Bottom half short
    // side)

    int16_t x_start, x_end;

    // Interpolate Long Side (V0 -> V2)
    // x = x0 + (y - y0) * (x2 - x0) / (y2 -
    // y0)
    x_start = x0 + ((int32_t)(y - y0) * (x2 - x0)) / total_height;

    // Interpolate Short Side
    if (y < y1) {
      // V0 -> V1
      int16_t h = y1 - y0;
      if (h == 0)
        x_end = x0; // Should not happen if
                    // loop condition meets
      else
        x_end = x0 + ((int32_t)(y - y0) * (x1 - x0)) / h;
    } else {
      // V1 -> V2
      int16_t h = y2 - y1;
      if (h == 0)
        x_end = x1;
      else
        x_end = x1 + ((int32_t)(y - y1) * (x2 - x1)) / h;
    }

    // Sort X
    if (x_start > x_end) {
      int16_t t = x_start;
      x_start = x_end;
      x_end = t;
    }

    // Clip X
    if (x_start < ctx->clip_rect.x)
      x_start = ctx->clip_rect.x;
    if (x_end > ctx->clip_rect.x + ctx->clip_rect.w)
      x_end = ctx->clip_rect.x + ctx->clip_rect.w;

    if (x_start >= x_end)
      continue;

    // To Tile Coords
    int16_t lx_start = x_start - ctx->x_offset;
    int16_t lx_end = x_end - ctx->x_offset;
    int16_t ly = y - ctx->y_offset;

    if (ly < 0 || ly >= ctx->height)
      continue;

    if (lx_start < 0)
      lx_start = 0;
    if (lx_end > ctx->width)
      lx_end = ctx->width;

    if (lx_start >= lx_end)
      continue;

    // Draw Span
    meas_pixel_t *row = &ctx->buffer[ly * ctx->width + lx_start];
    int16_t w = lx_end - lx_start;
    meas_pixel_t color = ctx->fg_color;

    if (alpha == MEAS_ALPHA_OPAQUE) {
      for (int k = 0; k < w; k++)
        row[k] = color;
    } else {
      for (int k = 0; k < w; k++)
        row[k] = alpha_blend(row[k], color, alpha);
    }
  }
}

// --- Round Rect Primitives ---

static void cell_draw_round_rect(meas_render_ctx_t *ctx, meas_rect_t rect,
                                 int16_t r, uint8_t alpha) {
  int16_t x = rect.x;
  int16_t y = rect.y;
  int16_t w = rect.w;
  int16_t h = rect.h;

  // Clamp radius
  int16_t min_side = (w < h) ? w : h;
  if (r > min_side / 2)
    r = min_side / 2;
  if (r < 0)
    r = 0;

  if (r == 0) {
    cell_draw_rect(ctx, rect, alpha);
    return;
  }

  // Draw 4 Sides
  // Top
  cell_draw_line(ctx, x + r, y, x + w - r - 1, y, alpha);
  // Bottom
  cell_draw_line(ctx, x + r, y + h - 1, x + w - r - 1, y + h - 1, alpha);
  // Left
  cell_draw_line(ctx, x, y + r, x, y + h - r - 1, alpha);
  // Right
  cell_draw_line(ctx, x + w - 1, y + r, x + w - 1, y + h - r - 1, alpha);

  // Draw 4 Corners
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t cx = 0;
  int16_t cy = r;

  // Centers of corners
  int16_t xtl = x + r;
  int16_t ytl = y + r;
  int16_t xtr = x + w - 1 - r;
  int16_t ytr = y + r;
  int16_t xbl = x + r;
  int16_t ybl = y + h - 1 - r;
  int16_t xbr = x + w - 1 - r;
  int16_t ybr = y + h - 1 - r;

  while (cx < cy) {
    if (f >= 0) {
      cy--;
      ddF_y += 2;
      f += ddF_y;
    }
    cx++;
    ddF_x += 2;
    f += ddF_x;

    // Top Left (Q2)
    cell_draw_pixel(ctx, xtl - cx, ytl - cy, alpha);
    cell_draw_pixel(ctx, xtl - cy, ytl - cx, alpha);

    // Top Right (Q1)
    cell_draw_pixel(ctx, xtr + cx, ytr - cy, alpha);
    cell_draw_pixel(ctx, xtr + cy, ytr - cx, alpha);

    // Bottom Left (Q3)
    cell_draw_pixel(ctx, xbl - cx, ybl + cy, alpha);
    cell_draw_pixel(ctx, xbl - cy, ybl + cx, alpha);

    // Bottom Right (Q4)
    cell_draw_pixel(ctx, xbr + cx, ybr + cy, alpha);
    cell_draw_pixel(ctx, xbr + cy, ybr + cx, alpha);
  }

  // Handle Diagonals (floating point to int
  // artifact in Bresenham logic)
  if (cx == cy) {
    cell_draw_pixel(ctx, xtl - cx, ytl - cy, alpha);
    cell_draw_pixel(ctx, xtr + cx, ytr - cy, alpha);
    cell_draw_pixel(ctx, xbl - cx, ybl + cy, alpha);
    cell_draw_pixel(ctx, xbr + cx, ybr + cy, alpha);
  }
}

static void cell_fill_round_rect(meas_render_ctx_t *ctx, meas_rect_t rect,
                                 int16_t r, uint8_t alpha) {
  int16_t x = rect.x;
  int16_t y = rect.y;
  int16_t w = rect.w;
  int16_t h = rect.h;

  // Clamp radius
  int16_t min_side = (w < h) ? w : h;
  if (r > min_side / 2)
    r = min_side / 2;
  if (r < 0)
    r = 0;

  if (r == 0) {
    cell_fill_rect(ctx, x, y, w, h, alpha);
    return;
  }

  // 1. Fill Central Body
  // From y+r to y+h-r-1 (Inclusive height is h
  // - 2r)
  cell_fill_rect(ctx, x, y + r, w, h - 2 * r, alpha);

  // 2. Fill Top/Bottom Strips + Corners using
  // Bresenham

  // Centers
  int16_t xtl = x + r;
  int16_t ytl = y + r;
  int16_t xtr = x + w - 1 - r;
  int16_t ybl = y + h - 1 - r;

  // Draw helper for scanlines
  // We need to fill for each Y step.
  // The standard bresenham iterates `cx` (x
  // offset) and `cy` (y offset). We mirror for
  // top and bottom.

  // Initial strip at offset `r` (should be
  // covered by central body if exact, but
  // let's effectively do rows `ytl - cy` and
  // `ybl + cy` as cy goes from r down to 0...
  // wait bresenham goes 0 to r? cx goes
  // 0..~0.7r. cy goes r..~0.7r.

  // We need to cover all Y offsets from 1 to
  // `r`. Standard "Fill Circle" approach
  // iterates Y.

  int16_t current_y = r;
  int16_t current_x = 0;
  int16_t d = 3 - 2 * r;

  while (current_y >= current_x) {
    // Scanline 1: Offset Y = current_y
    // Rows: ytl - current_y AND ybl +
    // current_y Width span: from (xtl -
    // current_x) to (xtr + current_x) Note:
    // xtr is `x + w - 1 - r`. Span width =
    // (xtr + current_x) - (xtl - current_x) +
    // 1
    //            = (x + w - 1 - r + x) - (x +
    //            r - x) + 1 = w - 2r + 2x - 1
    //            + 1 = w - 2r + 2x
    // Wait, let's use calc:
    // Start X = xtl - current_x
    // Width = (xtr + current_x) - (xtl -
    // current_x) + 1

    int16_t span_w_narrow = (xtr + current_x) - (xtl - current_x) + 1;
    int16_t span_w_wide = (xtr + current_y) - (xtl - current_y) + 1;

    // Top Bounds
    // Row (ytl - current_y): Wide Span
    if (current_x > 0) { // Avoid overlapping central rect
                         // (offset 0)
      cell_fill_rect(ctx, xtl - current_y, ytl - current_x, span_w_wide, 1,
                     alpha);
      cell_fill_rect(ctx, xtl - current_y, ybl + current_x, span_w_wide, 1,
                     alpha);
    }

    if (current_x != current_y) {
      cell_fill_rect(ctx, xtl - current_x, ytl - current_y, span_w_narrow, 1,
                     alpha);
      cell_fill_rect(ctx, xtl - current_x, ybl + current_y, span_w_narrow, 1,
                     alpha);
    }

    current_x++;
    if (d > 0) {
      current_y--;
      d = d + 4 * (current_x - current_y) + 10;
    } else {
      d = d + 4 * current_x + 6;
    }
  }
}

// --- Text Primitives ---

void cell_set_font(meas_render_ctx_t *ctx, const meas_font_t *font) {
  if (ctx) {
    ctx->font = font;
  }
}

int16_t cell_get_text_width(meas_render_ctx_t *ctx, const char *text) {
  if (!ctx || !ctx->font || !text)
    return 0;

  int16_t width = 0;
  const meas_font_t *f = ctx->font;

  while (*text) {
    uint8_t w, h;
    f->get_glyph(f->bitmap, *text, &w, &h);
    width += w;
    text++;
  }
  return width;
}

int16_t cell_get_text_height(meas_render_ctx_t *ctx, const char *text) {
  (void)text;
  if (!ctx || !ctx->font)
    return 0;
  return ctx->font->height;
}

void cell_draw_text(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                    const char *text, uint8_t alpha) {
  if (!ctx || !ctx->font || !text)
    return;

  const meas_font_t *f = ctx->font;
  int16_t cur_x = x;
  int16_t cur_y = y;

  while (*text) {
    char c = *text;
    uint8_t gw, gh;
    const uint8_t *glyph_data = f->get_glyph(f->bitmap, c, &gw, &gh);

    if (glyph_data) {
      if (f->height > 8) {
        // Large Font (Native 16-bit)
        // Ensure aligned access if possible,
        // or use memcpy safe read if
        // architecture strict Since we
        // controls fonts, we assume 16-bit
        // alignment.
        const uint16_t *u16_data = (const uint16_t *)glyph_data;
        for (int r = 0; r < gh; r++) {
          uint16_t row_bits = u16_data[r];
          // Row width is typically 11-16
          // pixels.
          for (int cx = 0; cx < 16; cx++) {
            if (row_bits & (0x8000 >> cx)) {
              cell_draw_pixel(ctx, cur_x + cx, cur_y + r, alpha);
            }
          }
        }
      } else {
        // Small Font (Column Major)
        for (int col = 0; col < gw; col++) {
          uint8_t stripe = glyph_data[col + 1]; // Skip header
          for (int row = 0; row < 8; row++) {
            if (stripe & (1 << row)) {
              cell_draw_pixel(ctx, cur_x + col, cur_y + row, alpha);
            }
          }
        }
      }
    }
    cur_x += gw;
    text++;
  }
}

// --- API Definition ---

// --- Extended Primitives ---

static void cell_draw_text_aligned(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                                   const char *text, uint8_t align,
                                   uint8_t alpha) {
  if (!ctx || !ctx->font || !text)
    return;

  int16_t w = cell_get_text_width(ctx, text);
  int16_t h = cell_get_text_height(ctx, text);

  // Horizontal Alignment
  if (align & MEAS_ALIGN_CENTER) {
    x -= w / 2;
  } else if (align & MEAS_ALIGN_RIGHT) {
    x -= w;
  }

  // Vertical Alignment
  if (align & MEAS_ALIGN_VCENTER) {
    y -= h / 2;
  } else if (align & MEAS_ALIGN_BOTTOM) {
    y -= h;
  }

  cell_draw_text(ctx, x, y, text, alpha);
}

static meas_rect_t cell_get_clip_rect(meas_render_ctx_t *ctx) {
  if (ctx) {
    return ctx->clip_rect;
  }
  return (meas_rect_t){0, 0, 0, 0};
}

static void cell_invert_rect(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                             int16_t w, int16_t h) {
  if (!ctx || !ctx->buffer)
    return;

  // Clip to global clip_rect first
  int16_t cx = ctx->clip_rect.x;
  int16_t cy = ctx->clip_rect.y;
  int16_t cw = ctx->clip_rect.w;
  int16_t ch = ctx->clip_rect.h;

  // Intersect (x,y,w,h) with (cx,cy,cw,ch)
  if (x < cx) {
    w -= (cx - x);
    x = cx;
  }
  if (y < cy) {
    h -= (cy - y);
    y = cy;
  }
  if (x + w > cx + cw)
    w = cx + cw - x;
  if (y + h > cy + ch)
    h = cy + ch - y;

  if (w <= 0 || h <= 0)
    return;

  // Now clip to TILE bounds
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

  for (int16_t i = 0; i < h; i++) {
    for (int16_t j = 0; j < w; j++) {
      row[j] = ~row[j];
    }
    row += ctx->width;
  }
}

static void cell_draw_line_patt(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                                int16_t x1, int16_t y1, uint8_t pattern,
                                uint8_t alpha) {
  if (pattern == MEAS_PATTERN_SOLID) {
    cell_draw_line(ctx, x0, y0, x1, y1, alpha);
    return;
  }

  // Bresenham with Pattern
  // Logic duplicated to avoid function call
  // overhead in tight loop or complexity of
  // generic callback
  int16_t dx = abs(x1 - x0);
  int16_t dy = abs(y1 - y0);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = (dx > dy ? dx : -dy) / 2;
  int16_t e2;

  uint8_t bit_counter = 0;

  while (1) {
    // Check pattern: MSB first (0x80) rotated
    // by bit_counter
    if (pattern & (0x80 >> (bit_counter & 7))) {
      cell_draw_pixel(ctx, x0, y0, alpha);
    }
    bit_counter++;

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

// --- Arcs and Sectors ---
#include "measlib/utils/math.h" // For atan2, etc

static void cell_draw_arc(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                          int16_t r, int16_t start_angle, int16_t end_angle,
                          uint8_t alpha) {
  // Bresenham Circle with angle check
  // Optimization: Precompute octant visibility/angles?
  // Doing per-pixel atan2 is expensive.
  // BUT we have meas_math_atan2 which is somewhat fast.
  // Let's implement full circle iteration and check angles.
  // Angles in degrees 0..360.

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t cx = 0;
  int16_t cy = r;

  // Standard Bresenham octants
  // Function to check point and draw
  // Lambda-like helper macro or function

  // Normalize angles to 0-360
  while (start_angle < 0)
    start_angle += 360;
  while (start_angle >= 360)
    start_angle -= 360;
  while (end_angle < 0)
    end_angle += 360;
  while (end_angle >= 360)
    end_angle -= 360;

  bool cross_zero = (end_angle < start_angle);

#define CHECK_AND_DRAW(px, py)                                                 \
  do {                                                                         \
    float ang = meas_math_atan2((float)((py) - y), (float)((px) - x)) *        \
                180.0f / 3.14159f;                                             \
    if (ang < 0)                                                               \
      ang += 360.0f;                                                           \
    int16_t iang = (int16_t)ang;                                               \
    bool visible = false;                                                      \
    if (!cross_zero) {                                                         \
      if (iang >= start_angle && iang <= end_angle)                            \
        visible = true;                                                        \
    } else {                                                                   \
      if (iang >= start_angle || iang <= end_angle)                            \
        visible = true;                                                        \
    }                                                                          \
    if (visible)                                                               \
      cell_draw_pixel(ctx, px, py, alpha);                                     \
  } while (0)

  CHECK_AND_DRAW(x, y + r);
  CHECK_AND_DRAW(x, y - r);
  CHECK_AND_DRAW(x + r, y);
  CHECK_AND_DRAW(x - r, y);

  while (cx < cy) {
    if (f >= 0) {
      cy--;
      ddF_y += 2;
      f += ddF_y;
    }
    cx++;
    ddF_x += 2;
    f += ddF_x;

    CHECK_AND_DRAW(x + cx, y + cy);
    CHECK_AND_DRAW(x - cx, y + cy);
    CHECK_AND_DRAW(x + cx, y - cy);
    CHECK_AND_DRAW(x - cx, y - cy);
    CHECK_AND_DRAW(x + cy, y + cx);
    CHECK_AND_DRAW(x - cy, y + cx);
    CHECK_AND_DRAW(x + cy, y - cx);
    CHECK_AND_DRAW(x - cy, y - cx);
  }
#undef CHECK_AND_DRAW
}

static void cell_fill_pie(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                          int16_t r, int16_t start_angle, int16_t end_angle,
                          uint8_t alpha) {
  // Fill circle but restricted by angles.
  // Iterating BB and checking atan2 is 'safest' for correctness but slow.
  // Scanline approach is complex for pie slices.
  // Given the constraints and likely usage (small indicators),
  // we use a BB iterator with bounding box optimization.

  // Normalize angles
  while (start_angle < 0)
    start_angle += 360;
  while (start_angle >= 360)
    start_angle -= 360;
  while (end_angle < 0)
    end_angle += 360;
  while (end_angle >= 360)
    end_angle -= 360;
  bool cross_zero = (end_angle < start_angle);

  int16_t x0 = x - r;
  int16_t y0 = y - r;
  int16_t x1 = x + r;
  int16_t y1 = y + r;

  // Clip to global
  if (x0 < ctx->clip_rect.x)
    x0 = ctx->clip_rect.x;
  if (y0 < ctx->clip_rect.y)
    y0 = ctx->clip_rect.y;
  if (x1 >= ctx->clip_rect.x + ctx->clip_rect.w)
    x1 = ctx->clip_rect.x + ctx->clip_rect.w - 1;
  if (y1 >= ctx->clip_rect.y + ctx->clip_rect.h)
    y1 = ctx->clip_rect.y + ctx->clip_rect.h - 1;

  // Tile Clip
  int16_t tx0 = x0 - ctx->x_offset;
  int16_t ty0 = y0 - ctx->y_offset;
  int16_t tx1 = x1 - ctx->x_offset;
  int16_t ty1 = y1 - ctx->y_offset;

  if (tx0 < 0)
    tx0 = 0;
  if (ty0 < 0)
    ty0 = 0;
  if (tx1 >= ctx->width)
    tx1 = ctx->width - 1;
  if (ty1 >= ctx->height)
    ty1 = ctx->height - 1;

  if (tx0 > tx1 || ty0 > ty1)
    return;

  int32_t r2 = r * r;

  for (int16_t cy = ty0; cy <= ty1; cy++) {
    meas_pixel_t *row = &ctx->buffer[cy * ctx->width];
    int16_t gy = cy + ctx->y_offset;
    int32_t dy = gy - y;
    int32_t dy2 = dy * dy;

    for (int16_t cx = tx0; cx <= tx1; cx++) {
      int16_t gx = cx + ctx->x_offset;
      int32_t dx = gx - x;

      // Check Radius
      if (dx * dx + dy2 <= r2) {
        // Check Angle
        float ang = meas_math_atan2((float)dy, (float)dx) * 180.0f / 3.14159f;
        if (ang < 0)
          ang += 360.0f;
        int16_t iang = (int16_t)ang;

        bool visible = false;
        if (!cross_zero) {
          if (iang >= start_angle && iang <= end_angle)
            visible = true;
        } else {
          if (iang >= start_angle || iang <= end_angle)
            visible = true;
        }

        if (visible) {
          if (alpha == MEAS_ALPHA_OPAQUE) {
            row[cx] = ctx->fg_color;
          } else {
            row[cx] = alpha_blend(row[cx], ctx->fg_color, alpha);
          }
        }
      }
    }
  }
}

// --- Extended Shapes ---

static void cell_draw_line_thick(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                                 int16_t x1, int16_t y1, uint8_t width,
                                 uint8_t alpha) {
  if (width <= 1) {
    cell_draw_line(ctx, x0, y0, x1, y1, alpha);
    return;
  }

  // Calculate perpendicular vector
  float dx = (float)(x1 - x0);
  float dy = (float)(y1 - y0);
  float len = meas_math_sqrt(dx * dx + dy * dy);

  if (len == 0.0f)
    return; // Zero length line

  float ux = -dy / len;
  float uy = dx / len;

  float half_w = (float)width / 2.0f;
  float off_x = ux * half_w;
  float off_y = uy * half_w;

  // 4 corners of the thick line rectangle
  meas_point_t points[4];
  points[0] = (meas_point_t){(int16_t)(x0 + off_x), (int16_t)(y0 + off_y)};
  points[1] = (meas_point_t){(int16_t)(x1 + off_x), (int16_t)(y1 + off_y)};
  points[2] = (meas_point_t){(int16_t)(x1 - off_x), (int16_t)(y1 - off_y)};
  points[3] = (meas_point_t){(int16_t)(x0 - off_x), (int16_t)(y0 - off_y)};

  cell_fill_polygon(ctx, points, 4, alpha);
}

static void cell_draw_triangle(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                               int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                               uint8_t alpha) {
  cell_draw_line(ctx, x0, y0, x1, y1, alpha);
  cell_draw_line(ctx, x1, y1, x2, y2, alpha);
  cell_draw_line(ctx, x2, y2, x0, y0, alpha);
}

static void cell_draw_text_rect(meas_render_ctx_t *ctx, meas_rect_t rect,
                                const char *text, uint8_t align,
                                uint8_t alpha) {
  if (!ctx || !text || !ctx->font)
    return;

  // Simple Word Wrap implementation
  // Note: Does not handle VCENTER/BOTTOM alignment in this pass (requires
  // pre-calculation of lines) Implements TOP alignment, with LEFT/CENTER/RIGHT
  // support.

  int16_t cur_x = rect.x;
  int16_t cur_y = rect.y;
  int16_t line_height = ctx->font->height;
  int16_t space_width = cell_get_text_width(ctx, " ");

  const char *ptr = text;
  while (*ptr) {
    const char *word_start = ptr;
    int16_t word_len = 0;

    // Find next word
    while (*ptr && *ptr != ' ' && *ptr != '\n') {
      ptr++;
      word_len++;
    }

    // Measure word
    char word_buf[64];
    if (word_len >= 63)
      word_len = 63;
    memcpy(word_buf, word_start, word_len);
    word_buf[word_len] = '\0';

    int16_t word_w = cell_get_text_width(ctx, word_buf);

    // New Line check
    if (cur_x + word_w > rect.x + rect.w) {
      // Move to next line
      cur_x = rect.x;
      cur_y += line_height;
    }

    // Clip Y
    if (cur_y + line_height > rect.y + rect.h)
      break;

    // Draw Word
    // TODO: Handle Align (Requires per-line buffer or 2-pass)
    // For now, defaulting to LEFT alignment flow.
    // Ideally this function would fill a line buffer then draw it.

    cell_draw_text(ctx, cur_x, cur_y, word_buf, alpha);
    cur_x += word_w;

    // Handle space or newline
    if (*ptr == ' ') {
      cur_x += space_width;
      ptr++;
    } else if (*ptr == '\n') {
      cur_x = rect.x;
      cur_y += line_height;
      ptr++;
    }
  }
}

static void cell_measure_text(meas_render_ctx_t *ctx, const char *text,
                              meas_text_metrics_t *out_metrics) {
  if (!ctx || !out_metrics)
    return;
  out_metrics->width = cell_get_text_width(ctx, text);
  out_metrics->height = cell_get_text_height(ctx, text);
  out_metrics->ascent = out_metrics->height;
  out_metrics->descent = 0;
}

static void cell_draw_text_rotated(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                                   const char *text, int16_t angle,
                                   uint8_t alpha) {
  if (!ctx || !ctx->font || !text)
    return;

  meas_real_t rad = (meas_real_t)angle * (meas_real_t)MEAS_PI / 180.0f;
  meas_real_t s, c;
  meas_math_sincos(rad, &s, &c);

  const meas_font_t *f = ctx->font;
  meas_real_t cur_x = (meas_real_t)x;
  meas_real_t cur_y = (meas_real_t)y;

  while (*text) {
    char ch = *text;
    uint8_t gw, gh;
    const uint8_t *glyph_data = f->get_glyph(f->bitmap, ch, &gw, &gh);

    if (glyph_data) {
      if (f->height > 8) {
        const uint16_t *u16_data = (const uint16_t *)glyph_data;
        for (int r = 0; r < gh; r++) {
          uint16_t row_bits = u16_data[r];
          for (int k = 0; k < 16; k++) {
            if (row_bits & (0x8000 >> k)) {
              meas_real_t rx = (meas_real_t)k * c - (meas_real_t)r * s;
              meas_real_t ry = (meas_real_t)k * s + (meas_real_t)r * c;
              cell_draw_pixel(ctx, (int16_t)(cur_x + rx), (int16_t)(cur_y + ry),
                              alpha);
            }
          }
        }
      } else {
        for (int col = 0; col < gw; col++) {
          uint8_t stripe = glyph_data[col + 1];
          for (int row = 0; row < 8; row++) {
            if (stripe & (1 << row)) {
              meas_real_t rx = (meas_real_t)col * c - (meas_real_t)row * s;
              meas_real_t ry = (meas_real_t)col * s + (meas_real_t)row * c;
              cell_draw_pixel(ctx, (int16_t)(cur_x + rx), (int16_t)(cur_y + ry),
                              alpha);
            }
          }
        }
      }
    }
    cur_x += (meas_real_t)gw * c;
    cur_y += (meas_real_t)gw * s;
    text++;
  }
}

const meas_render_api_t meas_render_cell_api = {
    .draw_pixel = cell_draw_pixel,
    .get_pixel = cell_get_pixel,
    .draw_line = cell_draw_line,
    .draw_polyline = cell_draw_polyline,
    .fill_rect = cell_fill_rect,
    .fill_polygon = cell_fill_polygon,
    .blit = cell_blit,
    .draw_text = cell_draw_text,
    .fill_gradient_v = cell_fill_gradient_v,
    .fill_gradient_h = cell_fill_gradient_h,
    .get_dims = cell_get_dims,
    .draw_rect = cell_draw_rect,
    .draw_circle = cell_draw_circle,
    .fill_circle = cell_fill_circle,
    .draw_round_rect = cell_draw_round_rect,
    .fill_round_rect = cell_fill_round_rect,
    .set_font = cell_set_font,
    .get_text_width = cell_get_text_width,
    .get_text_height = cell_get_text_height,
    .draw_text_aligned = cell_draw_text_aligned,
    .draw_text_rect = cell_draw_text_rect,
    .invert_rect = cell_invert_rect,
    .get_clip_rect = cell_get_clip_rect,
    .draw_line_patt = cell_draw_line_patt,
    .draw_line_thick = cell_draw_line_thick,
    .push_clip_rect = cell_push_clip_rect,
    .pop_clip_rect = cell_pop_clip_rect,
    .fill_triangle = cell_fill_triangle,
    .draw_triangle = cell_draw_triangle,
    .draw_arc = cell_draw_arc,
    .fill_pie = cell_fill_pie,
    // Extended Text
    .measure_text = cell_measure_text,
    .draw_text_rotated = cell_draw_text_rotated};

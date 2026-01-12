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

// --- Primitives (Cell/Tile Operations) ---

static void cell_set_clip_rect(meas_render_ctx_t *ctx, meas_rect_t rect) {
  if (!ctx)
    return;
  ctx->clip_rect = rect;
}

static void cell_draw_pixel(meas_render_ctx_t *ctx, int16_t x, int16_t y) {
  if (!ctx || !ctx->buffer)
    return;

  // Clip (Global)
  if (!is_inside_clip(ctx, x, y))
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

static void cell_draw_rect(meas_render_ctx_t *ctx, meas_rect_t rect) {
  int16_t x = rect.x;
  int16_t y = rect.y;
  int16_t w = rect.w;
  int16_t h = rect.h;

  cell_draw_line(ctx, x, y, x + w - 1, y);                 // Top
  cell_draw_line(ctx, x, y + h - 1, x + w - 1, y + h - 1); // Bottom
  cell_draw_line(ctx, x, y, x, y + h - 1);                 // Left
  cell_draw_line(ctx, x + w - 1, y, x + w - 1, y + h - 1); // Right
}

static void cell_fill_rect(meas_render_ctx_t *ctx, int16_t x, int16_t y,
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
  meas_pixel_t color = ctx->fg_color;

  for (int16_t i = 0; i < h; i++) {
    for (int16_t j = 0; j < w; j++) {
      row[j] = color;
    }
    row += ctx->width;
  }
}

// Circle Helper
static void draw_circle_points(meas_render_ctx_t *ctx, int16_t xc, int16_t yc,
                               int16_t x, int16_t y) {
  cell_draw_pixel(ctx, xc + x, yc + y);
  cell_draw_pixel(ctx, xc - x, yc + y);
  cell_draw_pixel(ctx, xc + x, yc - y);
  cell_draw_pixel(ctx, xc - x, yc - y);
  cell_draw_pixel(ctx, xc + y, yc + x);
  cell_draw_pixel(ctx, xc - y, yc + x);
  cell_draw_pixel(ctx, xc + y, yc - x);
  cell_draw_pixel(ctx, xc - y, yc - x);
}

static void cell_draw_circle(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                             int16_t r) {
  int16_t x = 0, y = r;
  int16_t d = 3 - 2 * r;
  draw_circle_points(ctx, x0, y0, x, y);
  while (y >= x) {
    x++;
    if (d > 0) {
      y--;
      d = d + 4 * (x - y) + 10;
    } else {
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

                             static void cell_fill_circle(
                                 meas_render_ctx_t * ctx, int16_t x0,
                                 int16_t y0, int16_t r, uint8_t alpha) {
                               int16_t x = 0;
                               int16_t y = r;
                               int16_t d = 3 - 2 * r;

                               while (y >= x) {
                                 // Scanlines
                                 cell_fill_rect(ctx, x0 - x, y0 - y, 2 * x + 1,
                                                1, alpha);
                                 cell_fill_rect(ctx, x0 - x, y0 + y, 2 * x + 1,
                                                1, alpha);
                                 cell_fill_rect(ctx, x0 - y, y0 - x, 2 * y + 1,
                                                1, alpha);
                                 cell_fill_rect(ctx, x0 - y, y0 + x, 2 * y + 1,
                                                1, alpha);

                                 x++;
                                 if (d > 0) {
                                   y--;
                                   d = d + 4 * (x - y) + 10;
                                 } else {
                                   d = d + 4 * x + 6;
                                 }
                               }
                             }

                             static void cell_blit(
                                 meas_render_ctx_t * ctx, int16_t x, int16_t y,
                                 int16_t w, int16_t h, const void *img,
                                 uint8_t alpha) {
                               // TODO: Implement Alpha Blending for Blit
                               // For now using Opaque implementation
                               (void)alpha;

                               if (!ctx || !ctx->buffer || !img)
                                 return;

                               const uint16_t *src = (const uint16_t *)img;

                               // Simple unclipped blit to tile (optimization
                               // needed)
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
                                 meas_pixel_t *dst =
                                     &ctx->buffer[i * ctx->width + lx_start];
                                 const uint16_t *src_line = src + src_x_offset;
                                 memcpy(dst, src_line,
                                        copy_w * sizeof(meas_pixel_t));
                                 src += w; // Next source line
                               }
                             }

                             // --- Gradient Primitives ---

                             static inline meas_pixel_t lerp_color(
                                 meas_pixel_t c1, meas_pixel_t c2,
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
                               uint16_t r =
                                   r1 + ((int32_t)(r2 - r1) * ratio) / 255;
                               uint16_t g =
                                   g1 + ((int32_t)(g2 - g1) * ratio) / 255;
                               uint16_t b =
                                   b1 + ((int32_t)(b2 - b1) * ratio) / 255;

                               return (r << 11) | (g << 5) | b;
                             }

                             static void cell_fill_gradient_v(
                                 meas_render_ctx_t * ctx, int16_t x, int16_t y,
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

                                 uint8_t ratio =
                                     (relative_y * 255) /
                                     (orig_h - 1 > 0 ? orig_h - 1 : 1);
                                 meas_pixel_t grad_col =
                                     lerp_color(c1, c2, ratio);

                                 meas_pixel_t *row =
                                     &ctx->buffer[i * ctx->width];
                                 for (int16_t rx = lx_start; rx < lx_end;
                                      rx++) {
                                   if (alpha == MEAS_ALPHA_OPAQUE) {
                                     row[rx] = grad_col;
                                   } else {
                                     row[rx] =
                                         alpha_blend(row[rx], grad_col, alpha);
                                   }
                                 }
                               }
                             }

                             static void cell_fill_gradient_h(
                                 meas_render_ctx_t * ctx, int16_t x, int16_t y,
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
                                 meas_pixel_t *row =
                                     &ctx->buffer[i * ctx->width];
                                 for (int16_t rx = lx_start; rx < lx_end;
                                      rx++) {
                                   int16_t global_cur_x = ctx->x_offset + rx;
                                   int16_t relative_x = global_cur_x - orig_x;
                                   uint8_t ratio =
                                       (relative_x * 255) /
                                       (orig_w - 1 > 0 ? orig_w - 1 : 1);
                                   meas_pixel_t grad_col =
                                       lerp_color(c1, c2, ratio);

                                   if (alpha == MEAS_ALPHA_OPAQUE) {
                                     row[rx] = grad_col;
                                   } else {
                                     row[rx] =
                                         alpha_blend(row[rx], grad_col, alpha);
                                   }
                                 }
                               }
                             }

                             static void cell_get_dims(meas_render_ctx_t * ctx,
                                                       int16_t *w, int16_t *h) {
                               if (w)
                                 *w = ctx->width;
                               if (h)
                                 *h = ctx->height;
                             }

                             // --- Poly Primitives ---

                             static void cell_draw_polyline(
                                 meas_render_ctx_t * ctx,
                                 const meas_point_t *points, uint16_t count,
                                 uint8_t alpha) {
                               if (!ctx || !points || count < 2)
                                 return;

                               for (uint16_t i = 0; i < count - 1; i++) {
                                 cell_draw_line(ctx, points[i].x, points[i].y,
                                                points[i + 1].x,
                                                points[i + 1].y, alpha);
                               }
                             }

                             // Comparison function for qsort
                             static int compare_nodes(const void *a,
                                                      const void *b) {
                               return (*(int16_t *)a - *(int16_t *)b);
                             }

                             // Scanline Polygon Fill
                             static void cell_fill_polygon(
                                 meas_render_ctx_t * ctx,
                                 const meas_point_t *points, uint16_t count,
                                 uint8_t alpha) {
                               if (!ctx || !ctx->buffer || !points || count < 3)
                                 return;
                               if (alpha == MEAS_ALPHA_TRANSPARENT)
                                 return;

                               // Global Y range for this Tile AND Clip Rect
                               int16_t global_y_start = ctx->y_offset;
                               int16_t global_y_end =
                                   ctx->y_offset + ctx->height;

                               if (global_y_start < ctx->clip_rect.y)
                                 global_y_start = ctx->clip_rect.y;
                               if (global_y_end >
                                   ctx->clip_rect.y + ctx->clip_rect.h)
                                 global_y_end =
                                     ctx->clip_rect.y + ctx->clip_rect.h;

                               // Limits for polygon (Bounding Box check could
                               // optimize)
                               int16_t poly_min_y = points[0].y;
                               int16_t poly_max_y = points[0].y;
                               for (uint16_t i = 1; i < count; i++) {
                                 if (points[i].y < poly_min_y)
                                   poly_min_y = points[i].y;
                                 if (points[i].y > poly_max_y)
                                   poly_max_y = points[i].y;
                               }

                               // Intersect Y Loop ranges
                               int16_t start_y = (global_y_start > poly_min_y)
                                                     ? global_y_start
                                                     : poly_min_y;
                               int16_t end_y = (global_y_end < poly_max_y + 1)
                                                   ? global_y_end
                                                   : poly_max_y + 1;

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

      // --- API Definition ---

      const meas_render_api_t meas_render_cell_api = {
          .draw_pixel = cell_draw_pixel,
          .get_pixel = cell_get_pixel,
          .draw_line = cell_draw_line,
          .draw_polyline = cell_draw_polyline,
          .fill_rect = cell_fill_rect,
          .fill_polygon = cell_fill_polygon,
          .blit = cell_blit,
          .draw_text = NULL, // TODO: Font System
          .fill_gradient_v = cell_fill_gradient_v,
          .fill_gradient_h = cell_fill_gradient_h,
          .get_dims = cell_get_dims,
          .set_clip_rect = cell_set_clip_rect,
          .draw_rect = cell_draw_rect,
          .draw_circle = cell_draw_circle,
          .fill_circle = cell_fill_circle};

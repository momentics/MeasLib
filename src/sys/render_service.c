/**
 * @file render_service.c
 * @brief Render Service Implementation (HAL Bridge).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/sys/render_service.h"
#include "drv_lcd.h" // Target specific driver
#include "measlib/ui/core.h"
#include <stddef.h>

// UI Instance
static meas_ui_t main_ui;

// External Accessor from Board
extern void *meas_board_get_lcd_ctx(void);

// --- Shim Functions ---

static void shim_draw_pixel(meas_render_ctx_t *ctx, int16_t x, int16_t y) {
  (void)ctx;
  // Driver missing pixel primitive, use fill_rect
  void *lcd = meas_board_get_lcd_ctx();
  if (lcd)
    meas_drv_lcd_fill_rect(lcd, x, y, 1, 1, ctx->fg_color);
}

static void shim_fill_rect(meas_render_ctx_t *ctx, int16_t x, int16_t y,
                           int16_t w, int16_t h) {
  (void)ctx;
  void *lcd = meas_board_get_lcd_ctx();
  if (lcd)
    meas_drv_lcd_fill_rect(lcd, x, y, w, h, ctx->fg_color);
}

static void shim_blit(meas_render_ctx_t *ctx, int16_t x, int16_t y, int16_t w,
                      int16_t h, const void *img) {
  (void)ctx;
  void *lcd = meas_board_get_lcd_ctx();
  if (lcd)
    meas_drv_lcd_blit(lcd, x, y, w, h, (const uint16_t *)img);
}

static void shim_draw_line(meas_render_ctx_t *ctx, int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1) {
  void *lcd = meas_board_get_lcd_ctx();
  if (!lcd)
    return;

  // Basic optimizations
  if (y0 == y1) {
    int16_t start = (x0 < x1) ? x0 : x1;
    int16_t width = (x0 < x1) ? (x1 - x0 + 1) : (x0 - x1 + 1);
    meas_drv_lcd_fill_rect(lcd, start, y0, width, 1, ctx->fg_color);
  } else if (x0 == x1) {
    int16_t start = (y0 < y1) ? y0 : y1;
    int16_t height = (y0 < y1) ? (y1 - y0 + 1) : (y0 - y1 + 1);
    meas_drv_lcd_fill_rect(lcd, x0, start, 1, height, ctx->fg_color);
  } else {
    // Fallback to pixel drawing (shim_draw_pixel will fetch ctx again)
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = (dx > dy ? dx : -dy) / 2;
    int16_t e2;

    while (1) {
      meas_drv_lcd_fill_rect(lcd, x0, y0, 1, 1, ctx->fg_color);
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
}

static const meas_render_api_t render_api = {.draw_pixel = shim_draw_pixel,
                                             .draw_line = shim_draw_line,
                                             .fill_rect = shim_fill_rect,
                                             .blit = shim_blit,
                                             .draw_text = NULL};

meas_ui_t *meas_render_service_init(void *display_ctx) {
  (void)display_ctx; // Use internal accessor

  // Initialize standard values
  main_ui.base.api = NULL; // Fixed .vtable -> .api
  main_ui.zone_count = 0;

  return &main_ui;
}

void meas_render_service_update(void) {
  // Basic Test: Fill blue square occasionally?
  static int i = 0;
  if (i++ % 100 == 0) {
    meas_drv_lcd_fill_rect(meas_board_get_lcd_ctx(), 10, 10, 50, 50, 0x001F);
  }
}

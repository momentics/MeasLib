/**
 * @file core.c
 * @brief UI Core Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/ui/core.h"

// Define Tile Height locally or import?
// Ideally should be shared config. For now using static const or #define to
// match render_service.
#define UI_TILE_HEIGHT 8
#define UI_SCREEN_HEIGHT 240

void meas_ui_tick(meas_ui_t *ui) {
  if (!ui)
    return;
  const meas_ui_api_t *api = (const meas_ui_api_t *)ui->base.api;
  if (api && api->update) {
    api->update(ui);
  }
}

void meas_ui_invalidate_rect(meas_ui_t *ui, int16_t x, int16_t y, int16_t w,
                             int16_t h) {
  if (!ui)
    return;

  // 1. Clip to Screen
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (y >= UI_SCREEN_HEIGHT)
    return;
  if (y + h > UI_SCREEN_HEIGHT)
    h = UI_SCREEN_HEIGHT - y;
  if (h <= 0)
    return;

  // 2. Calculate affected tiles
  int16_t start_tile = y / UI_TILE_HEIGHT;
  int16_t end_tile = (y + h - 1) / UI_TILE_HEIGHT;

  // 3. Set Bits
  for (int16_t i = start_tile; i <= end_tile; i++) {
    if (i < 32) { // Safety for 32-bit mask
      ui->dirty_map |= (1U << i);
    }
  }
}

void meas_ui_force_redraw(meas_ui_t *ui) {
  if (ui) {
    ui->dirty_map = 0xFFFFFFFF;
  }
}

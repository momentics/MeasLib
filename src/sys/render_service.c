/**
 * @file render_service.c
 * @brief Render Service Implementation (Pipeline Controller).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements the Rendering Pipeline:
 * 1. Defines a static Tile Buffer (Zero Alloc).
 * 2. Iterates only over dirty tiles using the dirty bitmap.
 * 3. Sets up a Render Context pointing to the Tile.
 * 4. Calls the UI Engine to rasterize into the Tile (using Standard SW
 * Renderer), passing the tile area so steps can skip non-overlapping content.
 * 5. Flushes the Tile to the Hardware Driver via DMA (Zero Copy).
 */

#include "measlib/sys/render_service.h"
#include "drv_lcd.h"
#include "measlib/ui/render.h"
#include <string.h>

// Portable find-first-set-bit (ctz) — returns index of lowest set bit.
// MSVC uses _BitScanForward; GCC/Clang use __builtin_ctz.
#if defined(_MSC_VER)
#include <intrin.h>
static inline int ctz(uint32_t v) {
  unsigned long idx;
  _BitScanForward(&idx, v);
  return (int)idx;
}
#else
#define ctz(v) __builtin_ctz(v)
#endif

// UI Instance
static meas_ui_t main_ui;

// LCD HAL handle — initialized once at startup.
static void *lcd_handle = NULL;

// Tile Configuration
// 320 * 8 * 2 bytes = 5120 bytes
#define TILE_WIDTH 320
#define TILE_HEIGHT 8
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

static meas_pixel_t tile_buffer[TILE_WIDTH * TILE_HEIGHT];

// Import Standard Software Rasterizer (from ui/render_cell.c)
extern const meas_render_api_t meas_render_cell_api;

// Import Main Layout
extern const meas_ui_api_t layout_main_api;

// --- Service API ---

meas_ui_t *meas_render_service_init(void) {
  // Initialize LCD HAL once.
  lcd_handle = meas_drv_lcd_init();

  main_ui.base.api = (const meas_object_api_t *)&layout_main_api;

  return &main_ui;
}

void meas_render_service_update(void) {
  if (!lcd_handle)
    return;

  const meas_ui_api_t *ui_api = (const meas_ui_api_t *)main_ui.base.api;
  if (!ui_api || !ui_api->draw)
    return;

  // Skip entirely if nothing is dirty.
  if (main_ui.dirty_map == 0)
    return;

  // Force full redraw on first call.
  static bool first_run = true;
  if (first_run) {
    meas_ui_force_redraw(&main_ui);
    first_run = false;
  }

  // Iterate only dirty tiles via the bitmap.
  uint32_t dirty = main_ui.dirty_map;
  main_ui.dirty_map = 0; // clear upfront

  while (dirty) {
    int tile_idx = ctz(dirty);
    dirty &= ~(1U << tile_idx);

    int16_t y = tile_idx * TILE_HEIGHT;
    int16_t h = TILE_HEIGHT;
    if (y + h > SCREEN_HEIGHT)
      h = SCREEN_HEIGHT - y;

    // 1. Setup Context (Zero Alloc - Static Buffer)
    meas_render_ctx_t ctx = {.buffer = tile_buffer,
                             .width = TILE_WIDTH,
                             .height = h,
                             .x_offset = 0,
                             .y_offset = y,
                             .fg_color = 0xFFFF,
                             .bg_color = 0x0000,
                             .clip_rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}};

    // 2. Call UI Draw — pass tile area so each step can skip itself.
    ui_api->draw(&main_ui, &ctx, &meas_render_cell_api, y, h);

    // 3. Flush to Hardware (Zero Copy - DMA)
    meas_rect_t rect = {0, y, TILE_WIDTH, h};
    meas_drv_lcd_blit(lcd_handle, rect, tile_buffer);
  }
}

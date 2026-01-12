/**
 * @file render_service.c
 * @brief Render Service Implementation (Pipeline Controller).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements the Rendering Pipeline:
 * 1. Defines a static Tile Buffer (Zero Alloc).
 * 2. Iterates over the screen in tiles (e.g. 320x8).
 * 3. Sets up a Render Context pointing to the Tile.
 * 4. Calls the UI Engine to rasterize into the Tile (using Standard SW
 * Renderer).
 * 5. Flushes the Tile to the Hardware Driver via DMA (Zero Copy).
 */

#include "measlib/sys/render_service.h"
#include "drv_lcd.h" // Hardware Bridge
#include "measlib/ui/render.h"
#include <string.h>

// UI Instance
static meas_ui_t main_ui;

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

meas_ui_t *meas_render_service_init(void *display_ctx) {
  (void)display_ctx; // HAL context managed internally via drv_lcd.h

  // Initialize UI with Main Layout
  main_ui.base.api = (const meas_object_api_t *)&layout_main_api;

  return &main_ui;
}

void meas_render_service_update(void) {
  void *lcd = meas_drv_lcd_init(); // Get or Init HW
  if (!lcd)
    return;

  // Get UI API
  const meas_ui_api_t *ui_api = (const meas_ui_api_t *)main_ui.base.api;
  if (!ui_api || !ui_api->draw)
    return;

  // Force Redraw on Init (if not done elsewhere)
  // Or check if map is 0? For now, we assume layout triggers first redraw or we
  // do it here once.
  static bool first_run = true;
  if (first_run) {
    meas_ui_force_redraw(&main_ui);
    first_run = false;
  }

  // Tile Loop (Row Major)
  for (int16_t y = 0; y < SCREEN_HEIGHT; y += TILE_HEIGHT) {
    // Dirty Check
    int16_t tile_idx = y / TILE_HEIGHT;
    if (tile_idx < 32 && !(main_ui.dirty_map & (1U << tile_idx))) {
      continue; // Skip clean tile
    }

    int16_t h = TILE_HEIGHT;
    if (y + h > SCREEN_HEIGHT)
      h = SCREEN_HEIGHT - y;

    // 1. Setup Context (Zero Alloc - Static Buffer)
    // Note: User can optimize by checking dirty regions, but for now we redraw
    // all.
    meas_render_ctx_t ctx = {.buffer = tile_buffer,
                             .width = TILE_WIDTH,
                             .height = h,
                             .x_offset = 0,
                             .y_offset = y, // Global Y
                             .fg_color = 0xFFFF,
                             .bg_color = 0x0000};

    // 2. Call UI Draw (Software Rasterizer)
    // The UI Logic uses the provided API to write into ctx.buffer
    ui_api->draw(&main_ui, &ctx, &meas_render_cell_api);

    // 3. Flush to Hardware (Zero Copy - DMA)
    meas_drv_lcd_blit(lcd, 0, y, TILE_WIDTH, h, tile_buffer);

    // Clear Dirty Bit
    if (tile_idx < 32) {
      main_ui.dirty_map &= ~(1U << tile_idx);
    }
  }
}

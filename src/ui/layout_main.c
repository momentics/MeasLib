/**
 * @file layout_main.c
 * @brief Main UI Layout (High-Level Application Logic) with Pipeline.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/ui/core.h"
#include <stddef.h>

#define LAYOUT_SCREEN_W 320
#define LAYOUT_SCREEN_H 240

// --- Pipeline Steps ---

static void step_draw_bg(const meas_ui_t *ui, meas_render_ctx_t *ctx,
                         const meas_render_api_t *api, int16_t tile_y,
                         int16_t tile_h) {
  (void)ui;
  // Layer 0: Background — fill only the current tile area.
  // Gradient is computed from global Y so colors are correct per tile.
  api->fill_gradient_v(ctx, 0, tile_y, LAYOUT_SCREEN_W, tile_h, 0x0010,
                       0x0000, MEAS_ALPHA_OPAQUE);
}

static bool step_grid_condition(const meas_ui_t *ui, int16_t tile_y,
                                int16_t tile_h) {
  (void)ui;
  // Grid spans the full screen; draw if the tile overlaps the grid area.
  int16_t tile_bottom = tile_y + tile_h;
  return (tile_bottom > 0 && tile_y < LAYOUT_SCREEN_H);
}

static void step_draw_grid(const meas_ui_t *ui, meas_render_ctx_t *ctx,
                           const meas_render_api_t *api, int16_t tile_y,
                           int16_t tile_h) {
  (void)ui;
  (void)tile_h;
  // Layer 1: Grid — draw only the portions that intersect this tile.
  ctx->fg_color = 0x07E0; // Green
  // Vertical center line at x=160 — draw within tile Y range.
  api->draw_line(ctx, 160, tile_y, 160, tile_y + tile_h - 1,
                 MEAS_ALPHA_OPAQUE);
  // Horizontal center line at y=120 — draw only if tile overlaps it.
  if (tile_y <= 120 && (tile_y + tile_h) > 120) {
    api->draw_line(ctx, 0, 120, LAYOUT_SCREEN_W, 120, MEAS_ALPHA_OPAQUE);
  }
}

static bool step_traces_condition(const meas_ui_t *ui, int16_t tile_y,
                                  int16_t tile_h) {
  (void)ui;
  int16_t tile_bottom = tile_y + tile_h;
  return (tile_bottom > 0 && tile_y < LAYOUT_SCREEN_H);
}

static void step_draw_traces(const meas_ui_t *ui, meas_render_ctx_t *ctx,
                              const meas_render_api_t *api, int16_t tile_y,
                              int16_t tile_h) {
  (void)ui;
  // Layer 2: Traces — clip the test line to the tile.
  ctx->fg_color = 0xFFFF; // White
  // Diagonal line 0,0 -> 320,240 clipped to tile Y range.
  // y = (240/320) * x = 0.75 * x
  // For tile_y: x_start = tile_y / 0.75, for tile_bottom: x_end = tile_bottom / 0.75
  int16_t x_start = (int16_t)(tile_y / 0.75f);
  int16_t x_end = (int16_t)((tile_y + tile_h - 1) / 0.75f);
  if (x_start < 0)
    x_start = 0;
  if (x_end >= LAYOUT_SCREEN_W)
    x_end = LAYOUT_SCREEN_W - 1;
  if (x_start < x_end) {
    api->draw_line(ctx, x_start, tile_y, x_end, tile_y + tile_h - 1,
                   MEAS_ALPHA_OPAQUE);
  }
}

static bool step_overlay_condition(const meas_ui_t *ui, int16_t tile_y,
                                   int16_t tile_h) {
  (void)ui;
  int16_t tile_bottom = tile_y + tile_h;
  // Red box: y=100..140, Triangle: y=50..90, Zigzag: y=180..200,
  // Blue rect: y=110..160, Round rect: y=20..70
  // Any overlap with any overlay element.
  return (tile_bottom > 20 && tile_y < LAYOUT_SCREEN_H);
}

static void step_draw_overlay(const meas_ui_t *ui, meas_render_ctx_t *ctx,
                              const meas_render_api_t *api, int16_t tile_y,
                              int16_t tile_h) {
  (void)ui;
  (void)tile_h;
  // Layer 4: Overlay — draw elements only if they overlap the tile.
  // Render ctx already clips to tile, so global coords work.

  // Red Test Box: y=100..140
  if (tile_y < 140 && (tile_y + tile_h) > 100) {
    ctx->fg_color = 0xF800;
    api->fill_rect(ctx, 100, 100, 120, 40, MEAS_ALPHA_OPAQUE);
  }

  // Yellow Triangle: y=50..90
  if (tile_y < 90 && (tile_y + tile_h) > 50) {
    meas_point_t triangle[] = {{160, 50}, {200, 90}, {120, 90}};
    ctx->fg_color = 0xFFE0;
    api->fill_polygon(ctx, triangle, 3, MEAS_ALPHA_OPAQUE);
  }

  // Cyan Polyline: y=180..200
  if (tile_y < 200 && (tile_y + tile_h) > 180) {
    meas_point_t zigzag[] = {{10, 200}, {30, 180}, {50, 200}, {70, 180}};
    ctx->fg_color = 0x07FF;
    api->draw_polyline(ctx, zigzag, 4, MEAS_ALPHA_OPAQUE);
  }

  // Semi-Transparent Blue: y=110..160
  if (tile_y < 160 && (tile_y + tile_h) > 110) {
    ctx->fg_color = 0x001F;
    api->fill_rect(ctx, 110, 110, 50, 50, MEAS_ALPHA_50);
  }

  // Rounded Rect: y=20..70
  if (tile_y < 70 && (tile_y + tile_h) > 20) {
    meas_rect_t rr = {220, 20, 80, 50};
    ctx->fg_color = 0xF81F;
    api->fill_round_rect(ctx, rr, 10, MEAS_ALPHA_OPAQUE);
    ctx->fg_color = 0xFFFF;
    api->draw_round_rect(ctx, rr, 10, MEAS_ALPHA_OPAQUE);
  }
}

// --- Pipeline Definition ---

static const meas_render_step_t render_pipeline[] = {
    {RENDER_STAGE_BG, NULL, step_draw_bg},
    {RENDER_STAGE_GRID, step_grid_condition, step_draw_grid},
    {RENDER_STAGE_TRACE, step_traces_condition, step_draw_traces},
    {RENDER_STAGE_MARKER, NULL, NULL},
    {RENDER_STAGE_OVERLAY, step_overlay_condition, step_draw_overlay}};

static const size_t PIPELINE_STEPS =
    sizeof(render_pipeline) / sizeof(render_pipeline[0]);

// --- High-Level UI Logic ---

static meas_status_t layout_main_update(meas_ui_t *ui) {
  (void)ui;
  return MEAS_OK;
}

static meas_status_t layout_main_handle_input(meas_ui_t *ui,
                                             meas_variant_t input) {
  (void)ui;
  (void)input;
  return MEAS_OK;
}

/**
 * @brief Main Drawing Routine.
 * Iterates the Pipeline Layers for the current Tile.
 * Each step receives the tile Y offset and height so it can skip
 * content that doesn't overlap the tile.
 */
static meas_status_t layout_main_draw(meas_ui_t *ui, meas_render_ctx_t *ctx,
                                      const meas_render_api_t *api,
                                      int16_t tile_y, int16_t tile_h) {
  if (!api || !ctx)
    return MEAS_ERROR;

  for (size_t i = 0; i < PIPELINE_STEPS; i++) {
    const meas_render_step_t *step = &render_pipeline[i];

    if (step->condition && !step->condition(ui, tile_y, tile_h)) {
      continue;
    }

    if (step->execute) {
      step->execute(ui, ctx, api, tile_y, tile_h);
    }
  }

  return MEAS_OK;
}

const meas_ui_api_t layout_main_api = {.update = layout_main_update,
                                       .draw = layout_main_draw,
                                       .handle_input =
                                           layout_main_handle_input};

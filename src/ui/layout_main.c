/**
 * @file layout_main.c
 * @brief Main UI Layout (High-Level Application Logic) with Pipeline.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/ui/core.h"
#include <stddef.h>

// --- Pipeline Steps ---

static void step_draw_bg(const meas_ui_t *ui, meas_render_ctx_t *ctx,
                         const meas_render_api_t *api) {
  (void)ui;
  // Layer 0: Background
  // Gradient: Deep Blue (Top) -> Black (Bottom)
  api->fill_gradient_v(ctx, 0, 0, 320, 240, 0x0010, 0x0000, MEAS_ALPHA_OPAQUE);
}

static void step_draw_grid(const meas_ui_t *ui, meas_render_ctx_t *ctx,
                           const meas_render_api_t *api) {
  (void)ui;
  // Layer 1: Grid (Stub)
  // Example: Draw the Crosshair here as part of the grid
  ctx->fg_color = 0x07E0; // Green
  api->draw_line(ctx, 160, 0, 160, 240, MEAS_ALPHA_OPAQUE);
  api->draw_line(ctx, 0, 120, 320, 120, MEAS_ALPHA_OPAQUE);
}

static void step_draw_traces(const meas_ui_t *ui, meas_render_ctx_t *ctx,
                             const meas_render_api_t *api) {
  (void)ui;
  // Layer 2: Traces
  // Draw the Test White Line
  ctx->fg_color = 0xFFFF; // White
  api->draw_line(ctx, 0, 0, 320, 240, MEAS_ALPHA_OPAQUE);
}

static void step_draw_overlay(const meas_ui_t *ui, meas_render_ctx_t *ctx,
                              const meas_render_api_t *api) {
  (void)ui;
  // Layer 4: Overlay (Menu/Widgets)
  // Draw the Red Test Box
  ctx->fg_color = 0xF800; // Red
  api->fill_rect(ctx, 100, 100, 120, 40, MEAS_ALPHA_OPAQUE);

  // Demo: Yellow Triangle
  meas_point_t triangle[] = {{160, 50}, {200, 90}, {120, 90}};
  ctx->fg_color = 0xFFE0; // Yellow
  api->fill_polygon(ctx, triangle, 3, MEAS_ALPHA_OPAQUE);

  // Demo: Cyan Polyline
  meas_point_t zigzag[] = {{10, 200}, {30, 180}, {50, 200}, {70, 180}};
  ctx->fg_color = 0x07FF; // Cyan
  api->draw_polyline(ctx, zigzag, 4, MEAS_ALPHA_OPAQUE);

  // Demo: Semi-Transparent Blue overlap
  ctx->fg_color = 0x001F; // Blue
  // Draw over the Red Box to test blending (Purple-ish result expected)
  api->fill_rect(ctx, 110, 110, 50, 50, MEAS_ALPHA_50);

  // Demo: Rounded Rect
  meas_rect_t rr = {220, 20, 80, 50};
  ctx->fg_color = 0xF81F; // Magenta
  api->fill_round_rect(ctx, rr, 10, MEAS_ALPHA_OPAQUE);
  ctx->fg_color = 0xFFFF; // White
  api->draw_round_rect(ctx, rr, 10, MEAS_ALPHA_OPAQUE);
}

// --- Pipeline Definition ---

static const meas_render_step_t render_pipeline[] = {
    {RENDER_STAGE_BG, NULL, step_draw_bg},
    {RENDER_STAGE_GRID, NULL, step_draw_grid},
    {RENDER_STAGE_TRACE, NULL, step_draw_traces},
    {RENDER_STAGE_MARKER, NULL, NULL}, // No markers yet
    {RENDER_STAGE_OVERLAY, NULL, step_draw_overlay}};

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
 */
static meas_status_t layout_main_draw(meas_ui_t *ui, meas_render_ctx_t *ctx,
                                      const meas_render_api_t *api) {
  if (!api || !ctx)
    return MEAS_ERROR;

  for (size_t i = 0; i < PIPELINE_STEPS; i++) {
    const meas_render_step_t *step = &render_pipeline[i];

    // Check Condition
    if (step->condition && !step->condition(ui)) {
      continue;
    }

    // Execute Step
    if (step->execute) {
      step->execute(ui, ctx, api);
    }
  }

  return MEAS_OK;
}

const meas_ui_api_t layout_main_api = {.update = layout_main_update,
                                       .draw = layout_main_draw,
                                       .handle_input =
                                           layout_main_handle_input};

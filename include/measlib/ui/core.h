/**
 * @file core.h
 * @brief UI Controller & Main Loop.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the central `meas_ui_t` object that manages the screen composition,
 * focus, and interaction state.
 */

#ifndef MEASLIB_UI_CORE_H
#define MEASLIB_UI_CORE_H

#include "measlib/core/object.h"
#include "measlib/ui/render.h"

// Forward decl
typedef struct {
  int16_t x, y, w, h;
  meas_id_t widget_id; // ID for callback routing
  void (*on_touch)(void *ctx, int16_t x, int16_t y);
} meas_ui_zone_t;

/**
 * @brief UI Controller Object
 */
typedef struct {
  meas_object_t base;

  // State
  meas_id_t focused_id;        /**< Currently focused widget ID */
  meas_object_t *active_modal; /**< Active Modal Window (e.g. Keypad) */
  bool menu_open;              /**< Is main menu visible? */

  // Scene Descriptor
  meas_ui_zone_t
      hit_zones[16]; /**< Registered interactive zones for current frame */
  uint8_t zone_count;
} meas_ui_t;

/**
 * @brief UI API
 */
typedef struct {
  meas_object_api_t base;

  meas_status_t (*update)(meas_ui_t *ui);
  meas_status_t (*draw)(meas_ui_t *ui, const meas_render_api_t *draw_api);
  meas_status_t (*handle_input)(meas_ui_t *ui, meas_variant_t input_event);

} meas_ui_api_t;

#endif // MEASLIB_UI_CORE_H

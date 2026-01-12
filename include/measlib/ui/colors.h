/**
 * @file colors.h
 * @brief Pre-defined Color Maps for Heatmaps/Waterfalls.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEASLIB_UI_COLORS_H
#define MEASLIB_UI_COLORS_H

#include "measlib/types.h"
#include "measlib/ui/render.h"

/**
 * @brief Color Map Structure
 * Defines a LUT-based color palette (e.g. for heatmaps).
 */
typedef struct {
  const meas_pixel_t *lut; /**< Pointer to pixel array */
  uint16_t size;           /**< Number of elements */
} meas_color_map_t;

// Available Color Maps
extern const meas_color_map_t meas_cmap_hot;
extern const meas_color_map_t meas_cmap_jet;

/**
 * @brief UI Color Palette Indices
 * Corresponds to logical UI elements.
 */
typedef enum {
  MEAS_UI_COLOR_BG = 0,      // background
  MEAS_UI_COLOR_FG,          // foreground (in most cases text on background)
  MEAS_UI_COLOR_GRID,        // grid lines color
  MEAS_UI_COLOR_MENU,        // UI menu color
  MEAS_UI_COLOR_MENU_TEXT,   // UI menu text color
  MEAS_UI_COLOR_MENU_ACTIVE, // UI selected menu color
  MEAS_UI_COLOR_TRACE_1,     // Trace 1 color
  MEAS_UI_COLOR_TRACE_2,     // Trace 2 color
  MEAS_UI_COLOR_TRACE_3,     // Trace 3 color
  MEAS_UI_COLOR_TRACE_4,     // Trace 4 color
  MEAS_UI_COLOR_TRACE_5,     // Stored trace A color
  MEAS_UI_COLOR_TRACE_6,     // Stored trace B color
  MEAS_UI_COLOR_NORMAL_BAT,  // Normal battery icon color
  MEAS_UI_COLOR_LOW_BAT,     // Low battery icon color
  MEAS_UI_COLOR_SPEC_INPUT,  // Not used, for future
  MEAS_UI_COLOR_RISE_EDGE,   // UI menu button rise edge color
  MEAS_UI_COLOR_FALLEN_EDGE, // UI menu button fallen edge color
  MEAS_UI_COLOR_SWEEP_LINE,  // Sweep line color
  MEAS_UI_COLOR_BW_TEXT,     // Bandwidth text color
  MEAS_UI_COLOR_INPUT_TEXT,  // Keyboard Input text color
  MEAS_UI_COLOR_INPUT_BG,    // Keyboard Input text background color
  MEAS_UI_COLOR_MEASURE,     // Measure text color
  MEAS_UI_COLOR_GRID_VALUE,  // Not used, for future
  MEAS_UI_COLOR_INTERP_CAL,  // Calibration state on interpolation color
  MEAS_UI_COLOR_DISABLE_CAL, // Calibration state on disable color
  MEAS_UI_COLOR_LINK,        // UI menu button text for values color
  MEAS_UI_COLOR_TXT_SHADOW,  // Plot area text border color
  MEAS_UI_COLOR_MAX
} meas_ui_color_id_t;

/**
 * @brief Default UI Theme
 * Standard color palette similar to NanoVNA-X reference.
 */
extern const meas_pixel_t meas_ui_theme_default[MEAS_UI_COLOR_MAX];

#endif // MEASLIB_UI_COLORS_H

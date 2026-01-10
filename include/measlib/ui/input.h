/**
 * @file input.h
 * @brief UI Input System.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines input events (Touch, Key, Rotary) and calibration for touch screens.
 */

#ifndef MEASLIB_UI_INPUT_H
#define MEASLIB_UI_INPUT_H

#include "measlib/types.h"
#include <stdint.h>

/**
 * @brief Input Event Types
 */
typedef enum {
  INPUT_TYPE_TOUCH_PRESS,
  INPUT_TYPE_TOUCH_MOVE,
  INPUT_TYPE_TOUCH_RELEASE,
  INPUT_TYPE_KEY_PRESS,
  INPUT_TYPE_ROTARY_ENC
} meas_input_type_t;

/**
 * @brief Input Event Data
 */
typedef struct {
  meas_input_type_t type;
  int16_t x;          /**< Touch X coordinate OR Key Code */
  int16_t y;          /**< Touch Y coordinate OR Encoder Delta */
  uint32_t timestamp; /**< Event time (ms) */
} meas_input_event_t;

/**
 * @brief Touch Calibration Matrix
 * Used to transform raw ADC/Controller coords to Screen Space.
 */
typedef struct {
  int32_t a, b, c, d, e, f, div;
} meas_touch_cal_t;

/**
 * @brief Transform raw event using calibration data.
 */
meas_status_t meas_ui_calibrate_touch(meas_input_event_t *ev,
                                      const meas_touch_cal_t *cal);

#endif // MEASLIB_UI_INPUT_H

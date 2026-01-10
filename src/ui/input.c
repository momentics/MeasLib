/**
 * @file input.c
 * @brief Input & Calibration Logic.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/ui/input.h"

meas_status_t meas_ui_calibrate_touch(meas_input_event_t *ev,
                                      const meas_touch_cal_t *cal) {
  if (!ev || !cal)
    return MEAS_ERROR;

  if (ev->type >= INPUT_TYPE_KEY_PRESS)
    return MEAS_OK; // Only cal touch

  int32_t x_raw = ev->x;
  int32_t y_raw = ev->y;

  // Apply 3-point Matrix Transform:
  // X = (Ax + By + C) / Div
  // Y = (Dx + Ey + F) / Div

  if (cal->div == 0)
    return MEAS_ERROR;

  int32_t x_new = (cal->a * x_raw + cal->b * y_raw + cal->c) / cal->div;
  int32_t y_new = (cal->d * x_raw + cal->e * y_raw + cal->f) / cal->div;

  ev->x = (int16_t)x_new;
  ev->y = (int16_t)y_new;

  return MEAS_OK;
}

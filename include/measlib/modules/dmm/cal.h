/**
 * @file cal.h
 * @brief DMM Linear Calibration.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines simple Gain/Offset correction for Multimeter/ADC inputs.
 */

#ifndef MEASLIB_MODULES_DMM_CAL_H
#define MEASLIB_MODULES_DMM_CAL_H

#include "measlib/core/object.h"

/**
 * @brief Linear Calibration Coefficients
 * Y = (X * Gain) + Offset
 */
typedef struct {
  meas_real_t gain;
  meas_real_t offset;
} meas_dmm_cal_coefs_t;

// DMM Cal object definition would follow generic pattern

#endif // MEASLIB_MODULES_DMM_CAL_H

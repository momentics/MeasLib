/**
 * @file cal.h
 * @brief VNA Calibration (SOLT).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the Vector Error Correction definitions (12-term / 3-term models).
 */

#ifndef MEASLIB_MODULES_VNA_CAL_H
#define MEASLIB_MODULES_VNA_CAL_H

#include "measlib/core/data.h"
#include "measlib/core/object.h"

/**
 * @brief Calibration Standard Types
 */
typedef enum {
  CAL_STD_OPEN,
  CAL_STD_SHORT,
  CAL_STD_LOAD,
  CAL_STD_THRU,
  CAL_STD_ISOLATION
} meas_cal_std_t;

/**
 * @brief Error Terms Container (SOLT)
 * Pointers to arrays of error coefficients.
 */
typedef struct {
  meas_complex_t *ed; /**< Directivity Error */
  meas_complex_t *es; /**< Source Match Error */
  meas_complex_t *er; /**< Reflection Tracking Error */
  meas_complex_t *et; /**< Transmission Tracking Error */
  meas_complex_t *ex; /**< Isolation Error */
} meas_cal_coefs_t;

/**
 * @brief VNA Calibration Object
 */
typedef struct {
  meas_object_t base;
} meas_cal_t;

/**
 * @brief Calibration API
 */
typedef struct {
  meas_object_api_t base;

  /**
   * @brief Apply error correction to raw data.
   */
  meas_status_t (*apply)(meas_cal_t *cal, meas_data_block_t *data);

  /**
   * @brief Measure a standard for calibration.
   */
  meas_status_t (*measure_std)(meas_cal_t *cal, meas_cal_std_t std);

  /**
   * @brief Compute error coefficients from measurements.
   */
  meas_status_t (*compute)(meas_cal_t *cal, meas_cal_coefs_t *out_coefs);

} meas_cal_api_t;

/**
 * @brief Save/Load Helper functions.
 */
meas_status_t meas_cal_save(meas_cal_t *cal, const char *filename);
meas_status_t meas_cal_load(meas_cal_t *cal, const char *filename);

#endif // MEASLIB_MODULES_VNA_CAL_H

/**
 * @file analysis.h
 * @brief High-Level Signal Analysis Logic (Peaks, Regression, RF Math).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEASLIB_DSP_ANALYSIS_H
#define MEASLIB_DSP_ANALYSIS_H

#include "measlib/types.h"
#include <stddef.h>

// ============================================================================
// Peak Search
// ============================================================================

typedef struct {
  size_t index;
  float frequency;
  float amplitude;
} meas_dsp_peak_t;

/**
 * @brief Find global maximum peak in data.
 *
 * @param data Input magnitude array.
 * @param length Array length.
 * @param[out] peak Resulting peak info.
 * @return MEAS_OK if peak found.
 */
meas_status_t meas_dsp_peak_find_max(const float *data, size_t length,
                                     meas_dsp_peak_t *peak);

/**
 * @brief Find all local peaks above threshold.
 *
 * @param data Input magnitude array.
 * @param length Array length.
 * @param threshold Minimum amplitude.
 * @param[out] peaks Output buffer for peaks.
 * @param[in,out] max_count Input: Buffer size, Output: Number found.
 * @return MEAS_OK on success.
 */
meas_status_t meas_dsp_peak_find_all(const float *data, size_t length,
                                     float threshold, meas_dsp_peak_t *peaks,
                                     size_t *max_count);

// ============================================================================
// Regression
// ============================================================================

typedef struct {
  float a;
  float b;
} meas_dsp_lin_reg_t; // y = ax + b

/**
 * @brief Calculate Linear Regression (y = ax + b).
 *
 * @param x X coordinates.
 * @param y Y coordinates.
 * @param length Number of points.
 * @param[out] result Coefficients a and b.
 * @return MEAS_OK.
 */
meas_status_t meas_dsp_regression_linear(const float *x, const float *y,
                                         size_t length,
                                         meas_dsp_lin_reg_t *result);

// ============================================================================
// RF Analysis (Impedance Matching)
// ============================================================================

typedef struct {
  float xs;  // Series reactance
  float xps; // Parallel (Source side) reactance
  float xpl; // Parallel (Load side) reactance
} meas_dsp_match_result_t;

/**
 * @brief Calculate L/C Matching Network.
 * Match Load (Rl + jXl) to Source (R0).
 *
 * @param R0 Source Impedance (e.g. 50 Ohm).
 * @param RL Load Resistance.
 * @param XL Load Reactance.
 * @param[out] matches Matches array (up to 4 solutions).
 * @param[out] count Number of solutions found.
 * @return MEAS_OK.
 */
meas_status_t meas_dsp_lc_match(float R0, float RL, float XL,
                                meas_dsp_match_result_t *matches,
                                size_t *count);

#endif // MEASLIB_DSP_ANALYSIS_H

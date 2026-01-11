/**
 * @file analysis.c
 * @brief High-Level Signal Analysis Logic Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/dsp/analysis.h"
#include "measlib/utils/math.h"
#include <float.h>
#include <math.h>

#ifndef VNA_EPSILON
#define VNA_EPSILON 1e-6f
#endif

// ============================================================================
// Peak Search
// ============================================================================

meas_status_t meas_dsp_peak_find_max(const float *data, size_t length,
                                     meas_dsp_peak_t *peak) {
  if (!data || !peak || length == 0)
    return MEAS_ERROR;

  float max_val = -FLT_MAX;
  size_t max_idx = 0;

  for (size_t i = 0; i < length; i++) {
    if (data[i] > max_val) {
      max_val = data[i];
      max_idx = i;
    }
  }

  peak->index = max_idx;
  peak->amplitude = max_val;
  // Frequency needs external mapping (index -> freq), so we set freq = index
  // or caller handles it. Prototypes say peak has frequency field.
  // The function takes "data" (amplitude array). It doesn't know frequency map.
  // We will store index in frequency too or 0.0f?
  // Caller should populate frequency based on index.
  peak->frequency = 0.0f;

  return MEAS_OK;
}

static bool is_local_max(const float *data, size_t i, size_t len) {
  if (i == 0 || i == len - 1)
    return false;
  return (data[i] > data[i - 1]) && (data[i] > data[i + 1]);
}

meas_status_t meas_dsp_peak_find_all(const float *data, size_t length,
                                     float threshold, meas_dsp_peak_t *peaks,
                                     size_t *max_count) {
  if (!data || !peaks || !max_count || *max_count == 0 || length < 3)
    return MEAS_ERROR;

  size_t found = 0;
  size_t limit = *max_count;

  for (size_t i = 1; i < length - 1; i++) {
    if (data[i] > threshold && is_local_max(data, i, length)) {
      if (found < limit) {
        peaks[found].index = i;
        peaks[found].amplitude = data[i];
        peaks[found].frequency = 0.0f; // Caller responsibility
        found++;
      } else {
        break;
      }
    }
  }

  *max_count = found;
  return MEAS_OK;
}

// ============================================================================
// Regression
// ============================================================================

meas_status_t meas_dsp_regression_linear(const float *x, const float *y,
                                         size_t length,
                                         meas_dsp_lin_reg_t *result) {
  if (!x || !y || !result || length == 0)
    return MEAS_ERROR;

  float sum_x = 0, sum_y = 0, sum_xx = 0, sum_xy = 0;

  for (size_t i = 0; i < length; i++) {
    sum_x += x[i];
    sum_y += y[i];
    sum_xx += x[i] * x[i];
    sum_xy += x[i] * y[i];
  }

  float n = (float)length;
  float denom = (n * sum_xx - sum_x * sum_x);

  if (fabsf(denom) < VNA_EPSILON) {
    result->a = 0.0f;
    result->b = 0.0f;
    return MEAS_ERROR; // Vertical line
  }

  result->a = (n * sum_xy - sum_x * sum_y) / denom;
  result->b = (sum_y * sum_xx - sum_x * sum_xy) / denom;

  return MEAS_OK;
}

// ============================================================================
// RF Analysis (LC Match)
// ============================================================================

static void match_quad(float a, float b, float c, float *r) {
  if (fabsf(a) < VNA_EPSILON) {
    if (fabsf(b) > VNA_EPSILON) {
      r[0] = r[1] = -c / b;
    } else {
      r[0] = r[1] = 0.0f;
    }
    return;
  }
  float d =
      b * b - 4.0f * a * c; // NanoVNA used 2*a*c? No, standard is b^2 - 4ac
  // NanoVNA source: d = b^2 - 2*(2a)*c = b^2 - 4ac. Yes.

  if (d < 0) {
    r[0] = r[1] = 0;
    return;
  }
  float sd = meas_math_sqrt(d);
  r[0] = (-b + sd) / (2.0f * a);
  r[1] = (-b - sd) / (2.0f * a);
}

static void match_calc_hi(float R0, float RL, float XL,
                          meas_dsp_match_result_t *m) {
  float xp[2];
  // Equations from NanoVNA-X
  match_quad(R0 - RL, 2.0f * XL * R0, R0 * (XL * XL + RL * RL), xp);

  for (int i = 0; i < 2; i++) {
    float xl_new = XL + xp[i];
    m[i].xpl = xp[i];
    m[i].xps = 0.0f;
    m[i].xs = xp[i] * xp[i] * xl_new / (RL * RL + xl_new * xl_new) - xp[i];
  }
}

static void match_calc_lo(float R0, float RL, float XL,
                          meas_dsp_match_result_t *m) {
  float xs[2];
  match_quad(1.0f, 2.0f * XL, RL * RL + XL * XL - R0 * RL, xs);

  float rl_diff = RL - R0;

  for (int i = 0; i < 2; i++) {
    float xl_new = XL + xs[i];
    m[i].xs = xs[i];
    m[i].xpl = 0.0f;
    m[i].xps = -R0 * R0 * xl_new / (rl_diff * rl_diff + xl_new * xl_new);
  }
}

meas_status_t meas_dsp_lc_match(float R0, float RL, float XL,
                                meas_dsp_match_result_t *matches,
                                size_t *count) {
  if (!matches || !count)
    return MEAS_ERROR;

  if (RL <= 0.5f) {
    *count = 0;
    return MEAS_OK;
  } // Too low res

  // Check if matchable
  // If RL matches R0 (approx), only cancel XL
  if (RL > R0 * 0.9f && RL < R0 * 1.1f) {
    matches[0].xpl = 0;
    matches[0].xps = 0;
    matches[0].xs = -XL;
    *count = 1;
    return MEAS_OK;
  }

  *count = 0;
  // Hi Match
  if (RL >= R0 || (RL * RL + XL * XL) > R0 * RL) {
    match_calc_hi(R0, RL, XL, matches);
    *count += 2;
  }

  // Lo Match
  if (*count < 4) { // Only if space
    match_calc_lo(R0, RL, XL, &matches[*count]);
    *count += 2;
  }

  return MEAS_OK;
}

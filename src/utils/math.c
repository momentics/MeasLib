/**
 * @file math.c
 * @brief Generic Math Utilities.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/utils/math.h"
#include <math.h> // Generic C math for sqrt/fabs

meas_real_t meas_math_interp_linear(meas_real_t x, meas_real_t x0,
                                    meas_real_t y0, meas_real_t x1,
                                    meas_real_t y1) {
  if ((x1 - x0) == 0.0)
    return y0;
  return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

void meas_math_stats(const meas_real_t *data, size_t count, meas_real_t *mean,
                     meas_real_t *std_dev, meas_real_t *min_val,
                     meas_real_t *max_val) {
  if (!data || count == 0)
    return;

  meas_real_t sum = 0.0;
  meas_real_t sq_sum = 0.0;
  meas_real_t min_v = data[0];
  meas_real_t max_v = data[0];

  for (size_t i = 0; i < count; i++) {
    meas_real_t val = data[i];
    sum += val;
    sq_sum += val * val;
    if (val < min_v)
      min_v = val;
    if (val > max_v)
      max_v = val;
  }

  if (mean)
    *mean = sum / count;
  if (min_val)
    *min_val = min_v;
  if (max_val)
    *max_val = max_v;

  if (std_dev) {
    meas_real_t avg = sum / count;
    meas_real_t variance = (sq_sum / count) - (avg * avg);
    *std_dev = sqrt(variance > 0 ? variance : 0);
  }
}

meas_real_t meas_cabs(meas_complex_t z) {
  return sqrt(z.re * z.re + z.im * z.im);
}

meas_real_t meas_carg(meas_complex_t z) { return atan2(z.im, z.re); }

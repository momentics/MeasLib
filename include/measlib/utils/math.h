/**
 * @file math.h
 * @brief Generic Math Utilities.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Provides common algorithms for interpolation, statistics, and complex
 * arithmetic abstraction using the `meas_real_t` type.
 */

#ifndef MEASLIB_UTILS_MATH_H
#define MEASLIB_UTILS_MATH_H

#include "measlib/types.h"

/**
 * @brief Linear Interpolation.
 * Finds Y for a given X between (x0,y0) and (x1,y1).
 */
meas_real_t meas_math_interp_linear(meas_real_t x, meas_real_t x0,
                                    meas_real_t y0, meas_real_t x1,
                                    meas_real_t y1);

/**
 * @brief Compute Basic Statistics.
 * Calculates Mean, StdDev, Min, Max in a single pass.
 * @param data Input array.
 * @param count Number of elements.
 * @param mean Output Mean.
 * @param std_dev Output Standard Deviation.
 * @param min_val Output Min.
 * @param max_val Output Max.
 */
void meas_math_stats(const meas_real_t *data, size_t count, meas_real_t *mean,
                     meas_real_t *std_dev, meas_real_t *min_val,
                     meas_real_t *max_val);

/**
 * @brief Complex Absolute Value (Magnitude).
 */
meas_real_t meas_cabs(meas_complex_t z);

/**
 * @brief Complex Argument (Phase).
 */
meas_real_t meas_carg(meas_complex_t z);

#endif // MEASLIB_UTILS_MATH_H

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

// ============================================================================
// Constants
// ============================================================================

#define MEAS_PI 3.14159265358979323846
#define MEAS_EPSILON 1e-9

// ============================================================================
// Interpolation / Extrapolation
// ============================================================================

/**
 * @brief Linear Interpolation.
 * Finds Y for a given X between (x0,y0) and (x1,y1).
 */
meas_real_t meas_math_interp_linear(meas_real_t x, meas_real_t x0,
                                    meas_real_t y0, meas_real_t x1,
                                    meas_real_t y1);

/**
 * @brief Parabolic Interpolation.
 * Interpolates a value on a parabola defined by 3 equidistant points.
 * Useful for finding peaks or smooth curves through measured data.
 *
 * @param y1 Point at x = -1.
 * @param y2 Point at x = 0.
 * @param y3 Point at x = 1.
 * @param x Fractional position between -1 and 1 to interpolate at.
 * @return Interpolated value.
 */
meas_real_t meas_math_interp_parabolic(meas_real_t y1, meas_real_t y2,
                                       meas_real_t y3, meas_real_t x);

/**
 * @brief Cosine Interpolation.
 * Provides a smoother transition than linear interpolation using a cosine
 * curve.
 *
 * @param y1 Start value.
 * @param y2 End value.
 * @param x Position between 0.0 and 1.0.
 * @return Interpolated value.
 */
meas_real_t meas_math_interp_cosine(meas_real_t y1, meas_real_t y2,
                                    meas_real_t x);

/**
 * @brief Linear Extrapolation.
 * Estimate a value outside the known range based on two endpoints.
 *
 * @param x Target X position.
 * @param x0 First calibration X.
 * @param y0 First calibration Y.
 * @param x1 Second calibration X.
 * @param y1 Second calibration Y.
 * @return Extrapolated Y.
 */
meas_real_t meas_math_extrap_linear(meas_real_t x, meas_real_t x0,
                                    meas_real_t y0, meas_real_t x1,
                                    meas_real_t y1);

/**
 * @brief Check if two values are close (Approximation).
 * Returns true if the absolute difference is within epsilon.
 *
 * @param a First value.
 * @param b Second value.
 * @param epsilon Tolerance.
 * @return true if |a - b| <= epsilon.
 */
bool meas_math_is_close(meas_real_t a, meas_real_t b, meas_real_t epsilon);

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

// ============================================================================
// Fast Math (Platform Optimized)
// ============================================================================

/**
 * @brief Fast Sine and Cosine.
 * Computes sine and cosine of a given angle.
 * Uses optimized lookup tables and interpolation if fast math is enabled
 * and the target platform supports it (F303, AT32, F072).
 * Falls back to standard libm `sin` and `cos` for double precision.
 *
 * @param angle Input angle in radians.
 * @param[out] sin_val Pointer to store the sine result.
 * @param[out] cos_val Pointer to store the cosine result.
 */
void meas_math_sincos(meas_real_t angle, meas_real_t *sin_val,
                      meas_real_t *cos_val);

/**
 * @brief Fast Square Root.
 * Computes the square root of a number.
 * Uses fast inverse square root approximation for float types.
 *
 * @param x Input value.
 * @return Square root of x.
 */
meas_real_t meas_math_sqrt(meas_real_t x);

/**
 * @brief Fast Cube Root.
 * Computes the cube root of a number.
 * Uses iterative approximation for float types.
 *
 * @param x Input value.
 * @return Cube root of x.
 */
meas_real_t meas_math_cbrt(meas_real_t x);

/**
 * @brief Fast Natural Logarithm (ln).
 * Computes the natural logarithm of a number.
 * Uses approximation for float types.
 *
 * @param x Input value.
 * @return Natural logarithm of x.
 */
meas_real_t meas_math_log(meas_real_t x);

/**
 * @brief Fast Base-10 Logarithm.
 * Computes the base-10 logarithm of a number.
 * Derived from natural calibration for float types.
 *
 * @param x Input value.
 * @return Base-10 logarithm of x.
 */
meas_real_t meas_math_log10(meas_real_t x);

/**
 * @brief Fast Artangent.
 * Computes the arc tangent of a number.
 *
 * @param x Input value.
 * @return Arc tangent of x in radians.
 */
meas_real_t meas_math_atan(meas_real_t x);

/**
 * @brief Fast Arctangent2.
 * Computes the arc tangent of y/x using the signs of arguments to determine the
 * correct quadrant.
 *
 * @param y Coordinate y.
 * @param x Coordinate x.
 * @return Arc tangent of y/x in radians (-PI to PI).
 */
meas_real_t meas_math_atan2(meas_real_t y, meas_real_t x);

/**
 * @brief Modulo (fractional part).
 * Decomposes a number into integral and fractional parts.
 *
 * @param x Input value.
 * @param[out] iptr Pointer to store the integral part.
 * @return Fractional part of x.
 */
meas_real_t meas_math_modf(meas_real_t x, meas_real_t *iptr);

#endif // MEASLIB_UTILS_MATH_H

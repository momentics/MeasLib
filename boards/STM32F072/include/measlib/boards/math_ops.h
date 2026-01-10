/**
 * @file math_ops.h
 * @brief Board-specific Math Optimizations for STM32F072 (Cortex-M0).
 *
 * No hardware FPU available. Falls back to standard software implementations.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEASLIB_BOARDS_MATH_OPS_H
#define MEASLIB_BOARDS_MATH_OPS_H

#include "measlib/types.h"
#include <math.h>

/**
 * @brief Software Absolute Value.
 * @param x Input float.
 * @return Absolute value of x.
 */
static inline float meas_board_fabsf(float x) { return fabsf(x); }

/**
 * @brief Software Square Root.
 * @param x Input float.
 * @return Square root of x.
 */
static inline float meas_board_sqrtf(float x) { return sqrtf(x); }

/**
 * @brief Software Fused Multiply Add.
 * @param x Multiplicand.
 * @param y Multiplier.
 * @param z Addend.
 * @return Result of (x*y)+z.
 */
static inline float meas_board_fmaf(float x, float y, float z) {
  return (x * y) + z;
}

// Define Macros to override generic implementations
#define MEAS_BOARD_HAS_FPU 0
#define MEAS_MATH_SQRT_IMPL(x) meas_board_sqrtf(x)
#define MEAS_MATH_ABS_IMPL(x) meas_board_fabsf(x)

#endif // MEASLIB_BOARDS_MATH_OPS_H

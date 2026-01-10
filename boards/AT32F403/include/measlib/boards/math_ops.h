/**
 * @file math_ops.h
 * @brief Board-specific Math Optimizations for AT32F403 (Cortex-M4F).
 *
 * Provides inline assembly implementations for hardware FPU instructions.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEASLIB_BOARDS_MATH_OPS_H
#define MEASLIB_BOARDS_MATH_OPS_H

#include "measlib/types.h"
#include <math.h>

/**
 * @brief Hardware Absolute Value (FPU).
 * Uses vabs.f32 instruction.
 * @param x Input float.
 * @return Absolute value of x.
 */
static inline float meas_board_fabsf(float x) {
  __asm__("vabs.f32 %0, %1" : "=t"(x) : "t"(x));
  return x;
}

/**
 * @brief Hardware Square Root (FPU).
 * Uses vsqrt.f32 instruction.
 * @param x Input float.
 * @return Square root of x.
 */
static inline float meas_board_sqrtf(float x) {
  __asm__("vsqrt.f32 %0, %1" : "=t"(x) : "t"(x));
  return x;
}

/**
 * @brief Fused Multiply Add (FPU).
 * Uses vfma.f32 instruction.
 * Computes (x * y) + z.
 * @param x Multiplicand.
 * @param y Multiplier.
 * @param z Addend.
 * @return Result of (x*y)+z.
 */
static inline float meas_board_fmaf(float x, float y, float z) {
  __asm__("vfma.f32 %0, %1, %2" : "+t"(z) : "t"(x), "t"(y));
  return z;
}

// Define Macros to override generic implementations
#define MEAS_BOARD_HAS_FPU 1
#define MEAS_MATH_SQRT_IMPL(x) meas_board_sqrtf(x)
#define MEAS_MATH_ABS_IMPL(x) meas_board_fabsf(x)

#endif // MEASLIB_BOARDS_MATH_OPS_H

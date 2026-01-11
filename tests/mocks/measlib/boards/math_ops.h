/**
 * @file math_ops.h
 * @brief Board-specific Math Optimizations (Host Mock).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Mocks the hardware FPU instructions using standard C library functions
 * for host-based testing.
 */

#ifndef MEASLIB_BOARDS_MATH_OPS_H
#define MEASLIB_BOARDS_MATH_OPS_H

#include <math.h>

static inline float meas_board_fabsf(float x) { return fabsf(x); }

static inline float meas_board_sqrtf(float x) { return sqrtf(x); }

static inline float meas_board_fmaf(float x, float y, float z) {
  return fmaf(x, y, z);
}

#define MEAS_BOARD_HAS_FPU 0
#define MEAS_MATH_SQRT_IMPL(x) meas_board_sqrtf(x)
#define MEAS_MATH_ABS_IMPL(x) meas_board_fabsf(x)

#endif // MEASLIB_BOARDS_MATH_OPS_H

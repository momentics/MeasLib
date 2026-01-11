/**
 * @file dsp_ops.h
 * @brief Board-specific DSP operations for STM32F303 (Cortex-M4F).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Provides inline assembly wrappers for Cortex-M4 DSP instructions
 * (SMLABB, SMLABT, etc.) for optimized signal processing.
 */

#ifndef MEASLIB_BOARDS_DSP_OPS_H
#define MEASLIB_BOARDS_DSP_OPS_H

#include <stdint.h>

// Ensure we are compiling for Cortex-M4
#if !defined(ARM_MATH_CM4) && !defined(F303)
#warning "F303 DSP Header included but ARM_MATH_CM4 not defined"
#endif

// ============================================================================
// DSP Intrinsics (Cortex-M4)
// ============================================================================

/**
 * @brief SMLABB: Signed Multiply Accumulate Long (Bottom x Bottom).
 * res = acc + (x[15:0] * y[15:0])
 */
__attribute__((always_inline)) static inline int32_t
meas_dsp_ops_smlabb(int32_t x, int32_t y, int32_t acc) {
  int32_t r;
  __asm__("smlabb %[r], %[x], %[y], %[a]"
          : [r] "=r"(r)
          : [x] "r"(x), [y] "r"(y), [a] "r"(acc));
  return r;
}

/**
 * @brief SMLABT: Signed Multiply Accumulate Long (Bottom x Top).
 * res = acc + (x[15:0] * y[31:16])
 */
__attribute__((always_inline)) static inline int32_t
meas_dsp_ops_smlabt(int32_t x, int32_t y, int32_t acc) {
  int32_t r;
  __asm__("smlabt %[r], %[x], %[y], %[a]"
          : [r] "=r"(r)
          : [x] "r"(x), [y] "r"(y), [a] "r"(acc));
  return r;
}

/**
 * @brief SMLATB: Signed Multiply Accumulate Long (Top x Bottom).
 * res = acc + (x[31:16] * y[15:0])
 */
__attribute__((always_inline)) static inline int32_t
meas_dsp_ops_smlatb(int32_t x, int32_t y, int32_t acc) {
  int32_t r;
  __asm__("smlatb %[r], %[x], %[y], %[a]"
          : [r] "=r"(r)
          : [x] "r"(x), [y] "r"(y), [a] "r"(acc));
  return r;
}

/**
 * @brief SMLATT: Signed Multiply Accumulate Long (Top x Top).
 * res = acc + (x[31:16] * y[31:16])
 */
__attribute__((always_inline)) static inline int32_t
meas_dsp_ops_smlatt(int32_t x, int32_t y, int32_t acc) {
  int32_t r;
  __asm__("smlatt %[r], %[x], %[y], %[a]"
          : [r] "=r"(r)
          : [x] "r"(x), [y] "r"(y), [a] "r"(acc));
  return r;
}

// 64-bit Accumulation Variants (SMLALxx)

/**
 * @brief SMLALBB: Signed Multiply Accumulate Long Long (Bottom x Bottom).
 * acc += (int64_t)(x[15:0] * y[15:0])
 */
__attribute__((always_inline)) static inline int64_t
meas_dsp_ops_smlalbb(int64_t acc, int32_t x, int32_t y) {
  union {
    struct {
      uint32_t lo;
      uint32_t hi;
    } s;
    int64_t i;
  } r;
  r.i = acc;
  __asm__("smlalbb %[lo], %[hi], %[x], %[y]"
          : [lo] "+r"(r.s.lo), [hi] "+r"(r.s.hi)
          : [x] "r"(x), [y] "r"(y));
  return r.i;
}

__attribute__((always_inline)) static inline int64_t
meas_dsp_ops_smlalbt(int64_t acc, int32_t x, int32_t y) {
  union {
    struct {
      uint32_t lo;
      uint32_t hi;
    } s;
    int64_t i;
  } r;
  r.i = acc;
  __asm__("smlalbt %[lo], %[hi], %[x], %[y]"
          : [lo] "+r"(r.s.lo), [hi] "+r"(r.s.hi)
          : [x] "r"(x), [y] "r"(y));
  return r.i;
}

__attribute__((always_inline)) static inline int64_t
meas_dsp_ops_smlaltb(int64_t acc, int32_t x, int32_t y) {
  union {
    struct {
      uint32_t lo;
      uint32_t hi;
    } s;
    int64_t i;
  } r;
  r.i = acc;
  __asm__("smlaltb %[lo], %[hi], %[x], %[y]"
          : [lo] "+r"(r.s.lo), [hi] "+r"(r.s.hi)
          : [x] "r"(x), [y] "r"(y));
  return r.i;
}

__attribute__((always_inline)) static inline int64_t
meas_dsp_ops_smlaltt(int64_t acc, int32_t x, int32_t y) {
  union {
    struct {
      uint32_t lo;
      uint32_t hi;
    } s;
    int64_t i;
  } r;
  r.i = acc;
  __asm__("smlaltt %[lo], %[hi], %[x], %[y]"
          : [lo] "+r"(r.s.lo), [hi] "+r"(r.s.hi)
          : [x] "r"(x), [y] "r"(y));
  return r.i;
}

// Macro to indicate HW DSP presence
#define MEAS_DSP_HAS_HW_ACCEL 1

#endif // MEASLIB_BOARDS_DSP_OPS_H

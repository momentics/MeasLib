/**
 * @file dsp_ops.h
 * @brief Board-specific DSP operations for STM32F072 (Cortex-M0).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Provides software fallback for DSP instructions that are not present
 * on Cortex-M0.
 */

#ifndef MEASLIB_BOARDS_DSP_OPS_H
#define MEASLIB_BOARDS_DSP_OPS_H

#include <stdint.h>

// ============================================================================
// DSP Fallbacks (Software)
// ============================================================================

/**
 * @brief SMLABB: Signed Multiply Accumulate Long (Bottom x Bottom).
 * res = acc + (x[15:0] * y[15:0])
 */
static inline int32_t meas_dsp_ops_smlabb(int32_t x, int32_t y, int32_t acc) {
  int16_t x_lo = (int16_t)(x & 0xFFFF);
  int16_t y_lo = (int16_t)(y & 0xFFFF);
  return acc + ((int32_t)x_lo * y_lo);
}

/**
 * @brief SMLABT: Signed Multiply Accumulate Long (Bottom x Top).
 * res = acc + (x[15:0] * y[31:16])
 */
static inline int32_t meas_dsp_ops_smlabt(int32_t x, int32_t y, int32_t acc) {
  int16_t x_lo = (int16_t)(x & 0xFFFF);
  int16_t y_hi = (int16_t)((y >> 16) & 0xFFFF);
  return acc + ((int32_t)x_lo * y_hi);
}

/**
 * @brief SMLATB: Signed Multiply Accumulate Long (Top x Bottom).
 * res = acc + (x[31:16] * y[15:0])
 */
static inline int32_t meas_dsp_ops_smlatb(int32_t x, int32_t y, int32_t acc) {
  int16_t x_hi = (int16_t)((x >> 16) & 0xFFFF);
  int16_t y_lo = (int16_t)(y & 0xFFFF);
  return acc + ((int32_t)x_hi * y_lo);
}

/**
 * @brief SMLATT: Signed Multiply Accumulate Long (Top x Top).
 * res = acc + (x[31:16] * y[31:16])
 */
static inline int32_t meas_dsp_ops_smlatt(int32_t x, int32_t y, int32_t acc) {
  int16_t x_hi = (int16_t)((x >> 16) & 0xFFFF);
  int16_t y_hi = (int16_t)((y >> 16) & 0xFFFF);
  return acc + ((int32_t)x_hi * y_hi);
}

// 64-bit Accumulation Variants (SMLALxx)

static inline int64_t meas_dsp_ops_smlalbb(int64_t acc, int32_t x, int32_t y) {
  int16_t x_lo = (int16_t)(x & 0xFFFF);
  int16_t y_lo = (int16_t)(y & 0xFFFF);
  return acc + ((int64_t)x_lo * y_lo);
}

static inline int64_t meas_dsp_ops_smlalbt(int64_t acc, int32_t x, int32_t y) {
  int16_t x_lo = (int16_t)(x & 0xFFFF);
  int16_t y_hi = (int16_t)((y >> 16) & 0xFFFF);
  return acc + ((int64_t)x_lo * y_hi);
}

static inline int64_t meas_dsp_ops_smlaltb(int64_t acc, int32_t x, int32_t y) {
  int16_t x_hi = (int16_t)((x >> 16) & 0xFFFF);
  int16_t y_lo = (int16_t)(y & 0xFFFF);
  return acc + ((int64_t)x_hi * y_lo);
}

static inline int64_t meas_dsp_ops_smlaltt(int64_t acc, int32_t x, int32_t y) {
  int16_t x_hi = (int16_t)((x >> 16) & 0xFFFF);
  int16_t y_hi = (int16_t)((y >> 16) & 0xFFFF);
  return acc + ((int64_t)x_hi * y_hi);
}

// Macro to indicate NO HW DSP
#define MEAS_DSP_HAS_HW_ACCEL 0

#endif // MEASLIB_BOARDS_DSP_OPS_H

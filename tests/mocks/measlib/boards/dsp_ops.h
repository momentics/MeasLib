/**
 * @file dsp_ops.h
 * @brief Board-specific DSP operations (Host Mock).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Provides generic C implementations of Cortex-M4 DSP intrinsics
 * for host-based testing.
 */

#ifndef MEASLIB_BOARDS_DSP_OPS_H
#define MEASLIB_BOARDS_DSP_OPS_H

#include <stdint.h>

// Helper macros to extract 16-bit halves
#define DSP_LO(val) ((int16_t)((val) & 0xFFFF))
#define DSP_HI(val) ((int16_t)((val) >> 16))

// ============================================================================
// DSP Intrinsics (Generic C)
// ============================================================================

/**
 * @brief SMLABB: Signed Multiply Accumulate Long (Bottom x Bottom).
 * res = acc + (x[15:0] * y[15:0])
 */
static inline int32_t meas_dsp_ops_smlabb(int32_t x, int32_t y, int32_t acc) {
  return acc + (int32_t)(DSP_LO(x) * DSP_LO(y));
}

/**
 * @brief SMLABT: Signed Multiply Accumulate Long (Bottom x Top).
 * res = acc + (x[15:0] * y[31:16])
 */
static inline int32_t meas_dsp_ops_smlabt(int32_t x, int32_t y, int32_t acc) {
  return acc + (int32_t)(DSP_LO(x) * DSP_HI(y));
}

/**
 * @brief SMLATB: Signed Multiply Accumulate Long (Top x Bottom).
 * res = acc + (x[31:16] * y[15:0])
 */
static inline int32_t meas_dsp_ops_smlatb(int32_t x, int32_t y, int32_t acc) {
  return acc + (int32_t)(DSP_HI(x) * DSP_LO(y));
}

/**
 * @brief SMLATT: Signed Multiply Accumulate Long (Top x Top).
 * res = acc + (x[31:16] * y[31:16])
 */
static inline int32_t meas_dsp_ops_smlatt(int32_t x, int32_t y, int32_t acc) {
  return acc + (int32_t)(DSP_HI(x) * DSP_HI(y));
}

// 64-bit Accumulation Variants (SMLALxx)

static inline int64_t meas_dsp_ops_smlalbb(int64_t acc, int32_t x, int32_t y) {
  return acc + (int64_t)(DSP_LO(x) * DSP_LO(y));
}

static inline int64_t meas_dsp_ops_smlalbt(int64_t acc, int32_t x, int32_t y) {
  return acc + (int64_t)(DSP_LO(x) * DSP_HI(y));
}

static inline int64_t meas_dsp_ops_smlaltb(int64_t acc, int32_t x, int32_t y) {
  return acc + (int64_t)(DSP_HI(x) * DSP_LO(y));
}

static inline int64_t meas_dsp_ops_smlaltt(int64_t acc, int32_t x, int32_t y) {
  return acc + (int64_t)(DSP_HI(x) * DSP_HI(y));
}

// Macro to indicate NO HW DSP (Use generic paths if available, though these
// intrinsics emulate it)
#define MEAS_DSP_HAS_HW_ACCEL 0

#endif // MEASLIB_BOARDS_DSP_OPS_H

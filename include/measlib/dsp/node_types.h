/**
 * @file node_types.h
 * @brief Shared Context Definitions for DSP Nodes.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * This header defines the context structures for all DSP nodes.
 * Explicit definition allows static allocation in Channel structures
 * without code duplication or layout mismatches.
 */

#ifndef MEASLIB_DSP_NODE_TYPES_H
#define MEASLIB_DSP_NODE_TYPES_H

#include "measlib/core/trace.h"
#include "measlib/dsp/dsp.h"
#include "measlib/types.h"

// ============================================================================
// Basic Nodes
// ============================================================================

typedef struct {
  meas_real_t gain;
} meas_node_gain_ctx_t;

typedef struct {
  meas_real_t slope;
  meas_real_t intercept;
} meas_node_linear_ctx_t;

// ============================================================================
// Spectral Nodes
// ============================================================================

typedef struct {
  meas_dsp_window_t type;
} meas_node_window_ctx_t;

typedef struct {
  meas_dsp_fft_t fft_ctx;
} meas_node_fft_ctx_t;

// ============================================================================
// Source / Sink Nodes
// ============================================================================

typedef struct {
  float freq;
  float sample_rate;
  meas_dsp_wave_t type;
  uint32_t phase_acc;
} meas_node_wavegen_ctx_t;

typedef struct {
  meas_trace_t *trace;
} meas_node_sink_ctx_t;

// ============================================================================
// Radio / VNA Nodes
// ============================================================================

// DDC Result (Accumulators)
typedef struct {
  int64_t acc_i;
  int64_t acc_q;
  int64_t ref_i;
  int64_t ref_q;
} meas_ddc_result_t;

typedef struct {
  meas_ddc_result_t result;
  const int16_t *sin_table_ptr;
} meas_node_ddc_ctx_t;

typedef struct {
  meas_complex_t gamma;
} meas_node_sparam_ctx_t;

typedef struct {
  meas_cal_t *cal_obj; // Reference to calibration object
} meas_node_cal_ctx_t;

typedef struct {
  meas_real_t prev_phase;
  meas_real_t freq_step_rad; // d_omega = 2*PI*step
  bool first_point;
} meas_node_group_delay_ctx_t;

typedef struct {
  meas_real_t alpha;       // EMA factor
  meas_real_t current_avg; // State
} meas_node_avg_ctx_t;

#endif // MEASLIB_DSP_NODE_TYPES_H

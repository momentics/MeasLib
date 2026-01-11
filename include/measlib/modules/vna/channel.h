/**
 * @file channel.h
 * @brief VNA-Specific Channel Extensions.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the Vector Network Analyzer channel logic, which typically manages
 * frequency sweeps, port switching, and S-parameter calculation.
 */

#ifndef MEASLIB_MODULES_VNA_CHANNEL_H
#define MEASLIB_MODULES_VNA_CHANNEL_H

#include "measlib/core/channel.h"
#include "measlib/dsp/chain.h"
#include "measlib/dsp/dsp.h"
#include "measlib/dsp/node_types.h"
#include "measlib/modules/vna/cal.h"
#include <stdbool.h>

#define MEAS_PROP_VNA_START_FREQ 0x1001
#define MEAS_PROP_VNA_STOP_FREQ 0x1002
#define MEAS_PROP_VNA_POINTS 0x1003
#define MEAS_PROP_VNA_BUFFER_PTR 0x1004
#define MEAS_PROP_VNA_BUFFER_CAP 0x1005

#define VNA_MAX_POINTS 1024
#define VNA_MIN_FREQ 10000      // 10 kHz
#define VNA_MAX_FREQ 6000000000 // 6 GHz

typedef enum {
  VNA_CH_STATE_IDLE,
  VNA_CH_STATE_SETUP,    // Configure Synth/Switch
  VNA_CH_STATE_ACQUIRE,  // Trigger DMA
  VNA_CH_STATE_WAIT_DMA, // Wait for Event
  VNA_CH_STATE_PROCESS,  // Math/Solt
  VNA_CH_STATE_NEXT      // Iterate Frequency
} meas_vna_state_t;

typedef struct {
  meas_channel_t base;

  // FSM State
  meas_vna_state_t state;
  volatile bool is_data_ready; // Inter-task Flag
  uint64_t current_freq_hz;
  uint64_t start_freq_hz;
  uint64_t stop_freq_hz;
  uint32_t points;
  uint32_t current_point;

  // Data Buffers
  // Caller-Owned Buffer (or Driver Provided via Event)
  meas_complex_t *user_buffer;
  size_t user_buffer_cap; // Capacity (in elements)
  void *active_buffer;    // Pointer to current data (user_buffer or external)

  // Processing Pipeline
  meas_chain_t pipeline;

  // Pipeline Nodes (Statically allocated)
  meas_node_t node_ddc;
  meas_node_t node_sparam;
  meas_node_t node_cal;
  meas_node_t node_sink;

  // Contexts
  meas_node_ddc_ctx_t ctx_ddc;
  meas_node_sparam_ctx_t ctx_sparam;
  meas_node_cal_ctx_t ctx_cal;

  // Sink is simple pass-through or copying
  meas_node_sink_ctx_t ctx_sink;

  // External Dependencies
  meas_trace_t *output_trace; // Target for the sink node
  meas_cal_t *active_cal;     // Active Calibration (NULL if none)

  // Dependencies (Should be abstract HAL)
  void *hal_synth;
  void *hal_rx;

} meas_vna_channel_t;

#endif // MEASLIB_MODULES_VNA_CHANNEL_H

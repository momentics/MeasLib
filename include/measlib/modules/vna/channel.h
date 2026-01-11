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
  uint32_t current_freq_hz;
  uint32_t start_freq_hz;
  uint32_t stop_freq_hz;
  uint32_t points;
  uint32_t current_point;

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

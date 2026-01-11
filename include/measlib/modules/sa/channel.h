/**
 * @file channel.h
 * @brief Spectrum Analyzer (SA) Channel.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEASLIB_MODULES_SA_CHANNEL_H
#define MEASLIB_MODULES_SA_CHANNEL_H

#include "measlib/core/channel.h"
#include "measlib/dsp/chain.h"
#include "measlib/dsp/dsp.h"
#include "measlib/dsp/node_types.h"

// SA Specific Integration
typedef struct {
  meas_channel_t base;

  // Pipeline
  meas_chain_t pipeline;

  // Nodes
  meas_node_t node_window;
  meas_node_t node_fft;
  meas_node_t node_mag;
  meas_node_t node_logmag;
  meas_node_t node_sink;

  // Contexts
  meas_node_window_ctx_t ctx_window;
  meas_node_fft_ctx_t ctx_fft;
  // Mag/LogMag are stateless (void* ctx)
  meas_node_sink_ctx_t ctx_sink;

  // Config
  size_t fft_size;
  meas_trace_t *output_trace;

} meas_sa_channel_t;

meas_status_t meas_sa_channel_init(meas_sa_channel_t *ch, meas_trace_t *trace);

#endif // MEASLIB_MODULES_SA_CHANNEL_H

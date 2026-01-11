/**
 * @file channel.h
 * @brief Digital Multimeter (DMM) Channel.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEASLIB_MODULES_DMM_CHANNEL_H
#define MEASLIB_MODULES_DMM_CHANNEL_H

#include "measlib/core/channel.h"
#include "measlib/dsp/chain.h"
#include "measlib/dsp/dsp.h"
#include "measlib/dsp/node_types.h"

typedef struct {
  meas_channel_t base;

  meas_chain_t pipeline;

  // Nodes
  meas_node_t node_linear;
  meas_node_t node_sink;

  // Contexts
  meas_node_linear_ctx_t ctx_linear;
  meas_node_sink_ctx_t ctx_sink;

  // Config
  meas_trace_t *output_trace;

} meas_dmm_channel_t;

meas_status_t meas_dmm_channel_init(meas_dmm_channel_t *ch);

#endif // MEASLIB_MODULES_DMM_CHANNEL_H

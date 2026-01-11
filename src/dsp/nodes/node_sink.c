/**
 * @file node_sink.c
 * @brief Trace Sink Node.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/core/trace.h"
#include "measlib/dsp/chain.h"
#include <stddef.h>

// ============================================================================
// Trace Sink Node
// ============================================================================

#include "measlib/dsp/node_types.h"

static const char *node_sink_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_Sink_Trace";
}

static meas_status_t node_sink_process(meas_node_t *node,
                                       const meas_data_block_t *input,
                                       meas_data_block_t *output) {
  meas_node_sink_ctx_t *ctx = (meas_node_sink_ctx_t *)node->impl;
  if (!ctx || !input) {
    return MEAS_ERROR;
  }

  // 1. Push data to the trace
  if (ctx->trace) {
    meas_trace_copy_data(ctx->trace, input->data, input->size);
  }

  // 2. Pass-through (Sink is transparent)
  if (output) {
    output->source_id = input->source_id;
    output->sequence = input->sequence;
    output->size = input->size;
    output->data = input->data;
  }

  return MEAS_OK;
}

static meas_status_t node_sink_reset(meas_node_t *node) {
  (void)node;
  return MEAS_OK;
}

static const meas_node_api_t node_sink_api = {
    .base =
        {
            .get_name = node_sink_get_name,
            .set_prop = NULL,
            .get_prop = NULL,
            .destroy = NULL,
        },
    .process = node_sink_process,
    .reset = node_sink_reset,
};

meas_status_t meas_node_sink_trace_init(meas_node_t *node, void *ctx_mem,
                                        meas_trace_t *target_trace) {
  if (!node || !ctx_mem || !target_trace) {
    return MEAS_ERROR;
  }
  meas_node_sink_ctx_t *ctx = (meas_node_sink_ctx_t *)ctx_mem;
  ctx->trace = target_trace;

  node->api = &node_sink_api;
  node->impl = ctx;
  node->next = NULL;
  node->ref_count = 1;
  return MEAS_OK;
}

/**
 * @file node_basic.c
 * @brief Basic Processing Nodes (Gain, Offset, Passthrough).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/dsp/chain.h"
#include "measlib/types.h"
#include <stddef.h>

// ============================================================================
// Gain Node (Y = X * Gain)
// ============================================================================

#include "measlib/dsp/node_types.h"

static const char *node_gain_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_Gain";
}

static meas_status_t node_gain_process(meas_node_t *node,
                                       const meas_data_block_t *input,
                                       meas_data_block_t *output) {
  meas_node_gain_ctx_t *ctx = (meas_node_gain_ctx_t *)node->impl;
  if (!ctx || !input || !output) {
    return MEAS_ERROR;
  }

  // Zero-Copy Pass-through of metadata
  output->source_id = input->source_id;
  output->sequence = input->sequence;
  output->size = input->size;
  output->data = input->data; // Modifying in-place (if buffer ownership allows)
                              // OR assume upstream provided a writable buffer.

  // NOTE: In a true zero-copy system, we modify data in place if we are the
  // exclusive owner, or we need a new buffer if we are branching.
  // For this simple example, we assume IN-PLACE modification is allowed
  // by the buffer policy.

  meas_real_t *samples = (meas_real_t *)output->data;
  size_t count = output->size / sizeof(meas_real_t);

  meas_real_t k = ctx->gain;

  for (size_t i = 0; i < count; i++) {
    samples[i] *= k;
  }

  return MEAS_OK;
}

static meas_status_t node_gain_reset(meas_node_t *node) {
  (void)node;
  return MEAS_OK;
}

static const meas_node_api_t node_gain_api = {
    .base =
        {
            .get_name = node_gain_get_name,
            // Properties could be implemented here to change gain at runtime
            .set_prop = NULL,
            .get_prop = NULL,
            .destroy = NULL,
        },
    .process = node_gain_process,
    .reset = node_gain_reset,
};

// ============================================================================
// Public Constructor
// ============================================================================

/**
 * @brief Initialize a Gain Node.
 * @param node Pointer to node struct.
 * @param ctx Pointer to context struct (must persist).
 * @param gain Gain factor.
 */
meas_status_t meas_node_gain_init(meas_node_t *node, void *ctx_mem,
                                  meas_real_t gain) {
  if (!node || !ctx_mem) {
    return MEAS_ERROR;
  }

  meas_node_gain_ctx_t *ctx = (meas_node_gain_ctx_t *)ctx_mem;
  ctx->gain = gain;

  node->api = &node_gain_api;
  node->impl = ctx;
  node->next = NULL;
  node->ref_count = 1;

  return MEAS_OK;
}

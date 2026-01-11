/**
 * @file node_radio.c
 * @brief Radio Processing Nodes (DDC, S-Param).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/dsp/chain.h"
#include "measlib/dsp/dsp.h"
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// Internal Types
// ============================================================================

/**
 * @brief Intermediate result from DDC to SParam node.
 */
#include "measlib/dsp/node_types.h"

static const char *node_ddc_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_DDC";
}

static meas_status_t node_ddc_process(meas_node_t *node,
                                      const meas_data_block_t *input,
                                      meas_data_block_t *output) {
  meas_node_ddc_ctx_t *ctx = (meas_node_ddc_ctx_t *)node->impl;
  if (!ctx || !input || !output) {
    return MEAS_ERROR;
  }

  // Expecting int16_t samples
  const int16_t *samples = (const int16_t *)input->data;
  size_t count = input->size / sizeof(int16_t);

  // Execute Hardware Accelerated Mix Down
  // Output goes into ctx->result accumulators
  meas_dsp_mix_down(samples, count, ctx->sin_table_ptr, &ctx->result.acc_i,
                    &ctx->result.acc_q, &ctx->result.ref_i, &ctx->result.ref_q);

  // Prepare Output
  // Point to our internal result struct.
  output->source_id = input->source_id;
  output->sequence = input->sequence;
  output->size = sizeof(meas_ddc_result_t);
  output->data = &ctx->result;

  // NOTE: This changes the "shape" of the data from a stream to a single
  // struct. Downstream nodes must expect this.

  return MEAS_OK;
}

static meas_status_t node_ddc_reset(meas_node_t *node) {
  (void)node;
  // Clear accumulators logic could go here
  return MEAS_OK;
}

static const meas_node_api_t node_ddc_api = {
    .base =
        {
            .get_name = node_ddc_get_name,
            .set_prop = NULL,
            .get_prop = NULL,
            .destroy = NULL,
        },
    .process = node_ddc_process,
    .reset = node_ddc_reset,
};

meas_status_t meas_node_ddc_init(meas_node_t *node, void *ctx_mem) {
  if (!node || !ctx_mem) {
    return MEAS_ERROR;
  }
  // Default to shared table
  meas_node_ddc_ctx_t *ctx = (meas_node_ddc_ctx_t *)ctx_mem;
  ctx->sin_table_ptr = meas_dsp_sin_table_1024;

  node->api = &node_ddc_api;
  node->impl = ctx_mem;
  node->next = NULL;
  node->ref_count = 1;
  return MEAS_OK;
}

// ============================================================================
// S-Parameter Node
// ============================================================================

// ============================================================================
// S-Parameter Node
// ============================================================================

static const char *node_sparam_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_SParam";
}

static meas_status_t node_sparam_process(meas_node_t *node,
                                         const meas_data_block_t *input,
                                         meas_data_block_t *output) {
  meas_node_sparam_ctx_t *ctx = (meas_node_sparam_ctx_t *)node->impl;
  if (!ctx || !input || !output) {
    return MEAS_ERROR;
  }

  // Input MUST be meas_ddc_result_t
  if (input->size != sizeof(meas_ddc_result_t)) {
    return MEAS_ERROR;
  }

  const meas_ddc_result_t *ddc = (const meas_ddc_result_t *)input->data;

  // Calculate Gamma
  // Calculate Gamma
  meas_dsp_gamma_calc(ddc->acc_i, ddc->acc_q, ddc->ref_i, ddc->ref_q,
                      &ctx->gamma);

  // Output is a single complex number
  output->source_id = input->source_id;
  output->sequence = input->sequence;
  output->size = sizeof(meas_complex_t);
  output->data = &ctx->gamma;

  return MEAS_OK;
}

static meas_status_t node_sparam_reset(meas_node_t *node) {
  (void)node;
  return MEAS_OK;
}

static const meas_node_api_t node_sparam_api = {
    .base =
        {
            .get_name = node_sparam_get_name,
            .set_prop = NULL,
            .get_prop = NULL,
            .destroy = NULL,
        },
    .process = node_sparam_process,
    .reset = node_sparam_reset,
};

meas_status_t meas_node_sparam_init(meas_node_t *node, void *ctx_mem) {
  if (!node || !ctx_mem) {
    return MEAS_ERROR;
  }
  node->api = &node_sparam_api;
  node->impl = ctx_mem;
  node->next = NULL;
  node->ref_count = 1;
  return MEAS_OK;
}

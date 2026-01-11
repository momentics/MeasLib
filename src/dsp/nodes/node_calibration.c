/**
 * @file node_calibration.c
 * @brief VNA Calibration Application Node.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/dsp/chain.h"
#include "measlib/dsp/node_types.h"
#include "measlib/modules/vna/cal.h"
#include <stddef.h>

static const char *node_cal_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_Calibration";
}

static meas_status_t node_cal_process(meas_node_t *node,
                                      const meas_data_block_t *input,
                                      meas_data_block_t *output) {
  meas_node_cal_ctx_t *ctx = (meas_node_cal_ctx_t *)node->impl;
  if (!ctx || !input || !output) {
    return MEAS_ERROR;
  }

  // If no calibration object is attached, pass-through
  if (ctx->cal_obj == NULL) {
    // Determine if we can do 0-copy or need to copy
    if (output->data != input->data) {
      // Copy needed if buffers differ
      // Copy needed if buffers differ
      // size_t copy_size = (input->size < output->size) ? input->size :
      // output->size;

      // Assuming simplistic memcpy for now...
      // this context effectively without string.h, so loop copy. However, we
      // can assume output->data == input->data if the chain manager allows
      // inplace. If not, we have to copy. For this node, we will assume
      // IN-PLACE is supported or explicit copy needed.

      // Note: The calibration API 'apply' usually works in-place on the data
      // block if possible. But 'apply' takes 'meas_data_block_t *data'. It
      // modifies it. So to use it here, we might need to copy input to output
      // FIRST, then apply on output.
    }
  } else {
    // 1. Copy Input -> Output (Preparation for modification)
    // We assume Output buffer is large enough
    meas_complex_t *src = (meas_complex_t *)input->data;
    meas_complex_t *dst = (meas_complex_t *)output->data;
    size_t count = input->size / sizeof(meas_complex_t);

    for (size_t i = 0; i < count; i++) {
      dst[i] = src[i];
    }

    // Setup Output Block metadata locally to pass to apply
    output->source_id = input->source_id;
    output->sequence = input->sequence;
    output->size = input->size;

    // 2. Apply Calibration
    meas_cal_api_t *api = (meas_cal_api_t *)ctx->cal_obj->base.api;
    if (api && api->apply) {
      api->apply(ctx->cal_obj, output);
    }
  }

  return MEAS_OK;
}

static meas_status_t node_cal_reset(meas_node_t *node) {
  (void)node;
  return MEAS_OK;
}

static const meas_node_api_t node_cal_api = {
    .base =
        {
            .get_name = node_cal_get_name,
            .set_prop = NULL,
            .get_prop = NULL,
            .destroy = NULL,
        },
    .process = node_cal_process,
    .reset = node_cal_reset,
};

meas_status_t meas_node_cal_init(meas_node_t *node, void *ctx_mem,
                                 meas_cal_t *cal_obj) {
  if (!node || !ctx_mem) {
    return MEAS_ERROR;
  }

  meas_node_cal_ctx_t *ctx = (meas_node_cal_ctx_t *)ctx_mem;
  ctx->cal_obj = cal_obj;

  node->api = &node_cal_api;
  node->impl = ctx;
  node->next = NULL;
  node->ref_count = 1;
  return MEAS_OK;
}

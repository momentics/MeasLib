/**
 * @file node_math.c
 * @brief Math Processing Nodes (Magnitude, LogMag, Phase, GroupDelay, Average).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/dsp/chain.h"
#include "measlib/dsp/node_types.h"
#include "measlib/utils/math.h"
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// Magnitude Node (Complex -> Real)
// ============================================================================

static const char *node_mag_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_Magnitude";
}

static meas_status_t node_mag_process(meas_node_t *node,
                                      const meas_data_block_t *input,
                                      meas_data_block_t *output) {
  (void)node; // Stateless
  if (!input || !output) {
    return MEAS_ERROR;
  }

  // Input: Complex [Re, Im, Re, Im...]
  // Output: Real [Mag, Mag...]
  meas_complex_t *in_buf = (meas_complex_t *)input->data;
  meas_real_t *out_buf = (meas_real_t *)output->data;
  size_t count = input->size / sizeof(meas_complex_t);

  for (size_t i = 0; i < count; i++) {
    out_buf[i] = meas_cabs(in_buf[i]);
  }

  output->source_id = input->source_id;
  output->sequence = input->sequence;
  output->size = count * sizeof(meas_real_t);
  output->data = input->data; // Reuse buffer

  return MEAS_OK;
}

static meas_status_t node_mag_reset(meas_node_t *node) {
  (void)node;
  return MEAS_OK;
}

static const meas_node_api_t node_mag_api = {
    .base = {.get_name = node_mag_get_name},
    .process = node_mag_process,
    .reset = node_mag_reset,
};

meas_status_t meas_node_mag_init(meas_node_t *node, void *ctx_mem) {
  if (!node)
    return MEAS_ERROR;
  (void)ctx_mem;
  node->api = &node_mag_api;
  node->impl = NULL;
  node->next = NULL;
  node->ref_count = 1;
  return MEAS_OK;
}

// ============================================================================
// LogMag Node (Linear -> dB)
// ============================================================================

static const char *node_logmag_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_LogMag";
}

static meas_status_t node_logmag_process(meas_node_t *node,
                                         const meas_data_block_t *input,
                                         meas_data_block_t *output) {
  (void)node;
  if (!input || !output)
    return MEAS_ERROR;

  meas_real_t *buf = (meas_real_t *)output->data;
  size_t count = input->size / sizeof(meas_real_t);

  for (size_t i = 0; i < count; i++) {
    if (buf[i] > 0.0f) {
      buf[i] = 20.0f * meas_math_log10(buf[i]);
    } else {
      buf[i] = -140.0f;
    }
  }

  output->source_id = input->source_id;
  output->sequence = input->sequence;
  output->size = input->size;
  output->data = input->data;

  return MEAS_OK;
}

static meas_status_t node_logmag_reset(meas_node_t *node) {
  (void)node;
  return MEAS_OK;
}

static const meas_node_api_t node_logmag_api = {
    .base = {.get_name = node_logmag_get_name},
    .process = node_logmag_process,
    .reset = node_logmag_reset,
};

meas_status_t meas_node_logmag_init(meas_node_t *node, void *ctx_mem) {
  if (!node)
    return MEAS_ERROR;
  (void)ctx_mem;
  node->api = &node_logmag_api;
  node->impl = NULL;
  node->next = NULL;
  node->ref_count = 1;
  return MEAS_OK;
}

// ============================================================================
// Phase Node (Complex -> Real (Radians))
// ============================================================================

static const char *node_phase_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_Phase";
}

static meas_status_t node_phase_process(meas_node_t *node,
                                        const meas_data_block_t *input,
                                        meas_data_block_t *output) {
  (void)node;
  if (!input || !output)
    return MEAS_ERROR;

  const meas_complex_t *in_buf = (const meas_complex_t *)input->data;
  meas_real_t *out_buf = (meas_real_t *)output->data;
  size_t count = input->size / sizeof(meas_complex_t);

  for (size_t i = 0; i < count; i++) {
    out_buf[i] = meas_carg(in_buf[i]);
  }

  output->source_id = input->source_id;
  output->sequence = input->sequence;
  output->size = count * sizeof(meas_real_t);

  return MEAS_OK;
}

static meas_status_t node_phase_reset(meas_node_t *node) {
  (void)node;
  return MEAS_OK;
}

static const meas_node_api_t node_phase_api = {
    .base = {.get_name = node_phase_get_name},
    .process = node_phase_process,
    .reset = node_phase_reset,
};

meas_status_t meas_node_phase_init(meas_node_t *node, void *ctx_mem) {
  if (!node)
    return MEAS_ERROR;
  (void)ctx_mem;
  node->api = &node_phase_api;
  node->impl = NULL;
  node->next = NULL;
  node->ref_count = 1;
  return MEAS_OK;
}

// ============================================================================
// Group Delay Node (dPhi/dOmega)
// ============================================================================

static const char *node_gd_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_GroupDelay";
}

static meas_status_t node_gd_process(meas_node_t *node,
                                     const meas_data_block_t *input,
                                     meas_data_block_t *output) {
  meas_node_group_delay_ctx_t *ctx = (meas_node_group_delay_ctx_t *)node->impl;
  if (!ctx || !input || !output)
    return MEAS_ERROR;

  const meas_real_t *in_buf = (const meas_real_t *)input->data;
  meas_real_t *out_buf = (meas_real_t *)output->data;
  size_t count = input->size / sizeof(meas_real_t);

  for (size_t i = 0; i < count; i++) {
    meas_real_t phi = in_buf[i];
    meas_real_t delta_phi;

    if (ctx->first_point) {
      delta_phi = 0;
      ctx->first_point = false;
    } else {
      delta_phi = phi - ctx->prev_phase;
      // Unwrap
      while (delta_phi > MEAS_PI)
        delta_phi -= 2 * MEAS_PI;
      while (delta_phi < -MEAS_PI)
        delta_phi += 2 * MEAS_PI;
    }

    if (ctx->freq_step_rad != 0.0f) {
      out_buf[i] = -delta_phi / ctx->freq_step_rad;
    } else {
      out_buf[i] = 0.0f;
    }

    ctx->prev_phase = phi;
  }

  output->source_id = input->source_id;
  output->sequence = input->sequence;
  output->size = input->size;
  output->data = input->data;

  return MEAS_OK;
}

static meas_status_t node_gd_reset(meas_node_t *node) {
  meas_node_group_delay_ctx_t *ctx = (meas_node_group_delay_ctx_t *)node->impl;
  if (ctx) {
    ctx->first_point = true;
    ctx->prev_phase = 0.0f;
  }
  return MEAS_OK;
}

static const meas_node_api_t node_gd_api = {
    .base = {.get_name = node_gd_get_name},
    .process = node_gd_process,
    .reset = node_gd_reset,
};

meas_status_t meas_node_group_delay_init(meas_node_t *node, void *ctx_mem,
                                         meas_real_t freq_step) {
  if (!node || !ctx_mem)
    return MEAS_ERROR;

  meas_node_group_delay_ctx_t *ctx = (meas_node_group_delay_ctx_t *)ctx_mem;
  ctx->freq_step_rad = freq_step * 2.0f * MEAS_PI;
  ctx->first_point = true;
  ctx->prev_phase = 0.0f;

  node->api = &node_gd_api;
  node->impl = ctx;
  node->next = NULL;
  node->ref_count = 1;
  return MEAS_OK;
}

// ============================================================================
// Average Node (EMA)
// ============================================================================

static const char *node_avg_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_Average";
}

static meas_status_t node_avg_process(meas_node_t *node,
                                      const meas_data_block_t *input,
                                      meas_data_block_t *output) {
  meas_node_avg_ctx_t *ctx = (meas_node_avg_ctx_t *)node->impl;
  if (!ctx || !input || !output)
    return MEAS_ERROR;

  const meas_real_t *in_buf = (const meas_real_t *)input->data;
  meas_real_t *out_buf = (meas_real_t *)output->data;
  size_t count = input->size / sizeof(meas_real_t);

  for (size_t i = 0; i < count; i++) {
    meas_real_t val = in_buf[i];
    ctx->current_avg = meas_math_ema(ctx->current_avg, val, ctx->alpha);
    out_buf[i] = ctx->current_avg;
  }

  output->source_id = input->source_id;
  output->sequence = input->sequence;
  output->size = input->size;
  output->data = input->data;

  return MEAS_OK;
}

static meas_status_t node_avg_reset(meas_node_t *node) {
  meas_node_avg_ctx_t *ctx = (meas_node_avg_ctx_t *)node->impl;
  if (ctx) {
    ctx->current_avg = 0.0f;
  }
  return MEAS_OK;
}

static const meas_node_api_t node_avg_api = {
    .base = {.get_name = node_avg_get_name},
    .process = node_avg_process,
    .reset = node_avg_reset,
};

meas_status_t meas_node_avg_init(meas_node_t *node, void *ctx_mem,
                                 meas_real_t alpha) {
  if (!node || !ctx_mem)
    return MEAS_ERROR;
  meas_node_avg_ctx_t *ctx = (meas_node_avg_ctx_t *)ctx_mem;
  ctx->alpha = alpha;
  ctx->current_avg = 0.0f;

  node->api = &node_avg_api;
  node->impl = ctx;
  node->next = NULL;
  node->ref_count = 1;
  return MEAS_OK;
}

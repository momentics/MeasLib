/**
 * @file node_source.c
 * @brief Source Nodes (Waveform Generator).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/dsp/chain.h"
#include "measlib/dsp/dsp.h"
#include <math.h>
#include <stddef.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ============================================================================
// WaveGen Node
// ============================================================================

#include "measlib/dsp/node_types.h"

static const char *node_wavegen_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_WaveGen";
}

static meas_status_t node_wavegen_process(meas_node_t *node,
                                          const meas_data_block_t *input,
                                          meas_data_block_t *output) {
  meas_node_wavegen_ctx_t *ctx = (meas_node_wavegen_ctx_t *)node->impl;
  if (!ctx || !output) {
    return MEAS_ERROR;
  }

  // Source Node: Input might be NULL or ignored.
  // We need to fill output->data.
  // NOTE: In this architecture, 'process' usually implies transformation.
  // But for a source, we utilize the buffer provided in 'output' or 'input' if
  // it's meant to be reused. If 'input' is provided, we overwrite it.

  meas_real_t *buf;
  size_t count;

  if (input && input->data) {
    buf = (meas_real_t *)input->data;
    count = input->size / sizeof(meas_real_t);
    // Pass through metadata
    output->source_id = input->source_id;
    output->sequence = input->sequence;
    output->size = input->size;
    output->data = input->data;
  } else {
    // If no input buffer provided, we can't do anything without allocation.
    // The Pipeline.run() contract usually passes a block.
    return MEAS_ERROR;
  }

  // Unused for simple impl
  // float phase = (float)ctx->phase_acc * phase_step;

  // Simple floating point generation
  // For robustness we should use the accumulated phase properly wrapping 0..2PI

  // float current_phase = 0.0f; // Stateless for this block relative to start
  // (TODO: Maintain phase continuity across blocks using ctx->phase_acc)

  for (size_t i = 0; i < count; i++) {
    float t = (float)i / ctx->sample_rate;
    float arg = 2.0f * (float)M_PI * ctx->freq * t; // Basic non-continuous

    // Switch Type
    switch (ctx->type) {
    case DSP_WAVE_SINE:
      buf[i] = sinf(arg);
      break;
    case DSP_WAVE_SQUARE:
      buf[i] = (sinf(arg) >= 0.0f) ? 1.0f : -1.0f;
      break;
    default:
      buf[i] = 0.0f;
    }
  }

  return MEAS_OK;
}

static meas_status_t node_wavegen_reset(meas_node_t *node) {
  meas_node_wavegen_ctx_t *ctx = (meas_node_wavegen_ctx_t *)node->impl;
  if (ctx) {
    ctx->phase_acc = 0;
  }
  return MEAS_OK;
}

static const meas_node_api_t node_wavegen_api = {
    .base =
        {
            .get_name = node_wavegen_get_name,
            .set_prop = NULL,
            .get_prop = NULL,
            .destroy = NULL,
        },
    .process = node_wavegen_process,
    .reset = node_wavegen_reset,
};

meas_status_t meas_node_wavegen_init(meas_node_t *node, void *ctx_mem,
                                     float freq, float sample_rate,
                                     meas_dsp_wave_t type) {
  if (!node || !ctx_mem) {
    return MEAS_ERROR;
  }
  meas_node_wavegen_ctx_t *ctx = (meas_node_wavegen_ctx_t *)ctx_mem;
  ctx->freq = freq;
  ctx->sample_rate = sample_rate;
  ctx->type = type;
  ctx->phase_acc = 0;

  node->api = &node_wavegen_api;
  node->impl = ctx;
  node->next = NULL;
  node->ref_count = 1;
  return MEAS_OK;
}

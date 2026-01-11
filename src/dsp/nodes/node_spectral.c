/**
 * @file node_spectral.c
 * @brief Spectral Processing Nodes (Window, FFT).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/dsp/chain.h"
#include "measlib/dsp/dsp.h"
#include <stddef.h>

// ============================================================================
// Window Node
// ============================================================================

#include "measlib/dsp/node_types.h"

static const char *node_window_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_Window";
}

static meas_status_t node_window_process(meas_node_t *node,
                                         const meas_data_block_t *input,
                                         meas_data_block_t *output) {
  meas_node_window_ctx_t *ctx = (meas_node_window_ctx_t *)node->impl;
  if (!ctx || !input || !output) {
    return MEAS_ERROR;
  }

  // Pass-through metadata
  output->source_id = input->source_id;
  output->sequence = input->sequence;
  output->size = input->size;
  output->data = input->data; // In-place modification

  // Apply Window
  // Assuming input is REAL or COMPLEX.
  // meas_dsp_apply_window takes meas_real_t*.
  // If data is Complex, we need to handle it carefully.
  // Usually windowing is done on Time Domain signal which is often Real for us
  // (if coming from ADC buffer treat as Real).
  // However, meas_dsp_apply_window signature: (meas_real_t *buffer, size_t
  // size, ...)

  meas_real_t *buffer = (meas_real_t *)output->data;
  size_t count = output->size / sizeof(meas_real_t);

  return meas_dsp_apply_window(buffer, count, ctx->type);
}

static meas_status_t node_window_reset(meas_node_t *node) {
  (void)node;
  return MEAS_OK;
}

static const meas_node_api_t node_window_api = {
    .base =
        {
            .get_name = node_window_get_name,
            .set_prop = NULL,
            .get_prop = NULL,
            .destroy = NULL,
        },
    .process = node_window_process,
    .reset = node_window_reset,
};

meas_status_t meas_node_window_init(meas_node_t *node, void *ctx_mem,
                                    meas_dsp_window_t type) {
  if (!node || !ctx_mem) {
    return MEAS_ERROR;
  }
  meas_node_window_ctx_t *ctx = (meas_node_window_ctx_t *)ctx_mem;
  ctx->type = type;

  node->api = &node_window_api;
  node->impl = ctx;
  node->next = NULL;
  node->ref_count = 1;
  return MEAS_OK;
}

// ============================================================================
// FFT Node
// ============================================================================

static const char *node_fft_get_name(meas_object_t *obj) {
  (void)obj;
  return "Node_FFT";
}

static meas_status_t node_fft_process(meas_node_t *node,
                                      const meas_data_block_t *input,
                                      meas_data_block_t *output) {
  meas_node_fft_ctx_t *ctx = (meas_node_fft_ctx_t *)node->impl;
  if (!ctx || !input || !output) {
    return MEAS_ERROR;
  }

  // FFT technically requires Complex Input -> Complex Output.
  // We assume the input buffer is already castable to meas_complex_t*.
  // AND we perform IN-PLACE FFT if possible, or we need a destination buffer.

  // For this generic node, we assume In-Place for now, as meas_dsp_fft_exec
  // supports it if input==output pointers.

  output->source_id = input->source_id;
  output->sequence = input->sequence;
  output->size = input->size;
  output->data = input->data;

  meas_complex_t *buffer = (meas_complex_t *)output->data;

  // Note: FFT execution
  return meas_dsp_fft_exec(&ctx->fft_ctx, buffer, buffer);
}

static meas_status_t node_fft_reset(meas_node_t *node) {
  (void)node;
  return MEAS_OK;
}

static const meas_node_api_t node_fft_api = {
    .base =
        {
            .get_name = node_fft_get_name,
            .set_prop = NULL,
            .get_prop = NULL,
            .destroy = NULL,
        },
    .process = node_fft_process,
    .reset = node_fft_reset,
};

meas_status_t meas_node_fft_init(meas_node_t *node, void *ctx_mem,
                                 size_t length, bool inverse) {
  if (!node || !ctx_mem) {
    return MEAS_ERROR;
  }
  meas_node_fft_ctx_t *ctx = (meas_node_fft_ctx_t *)ctx_mem;

  // Initialize the underlying DSP FFT context
  meas_status_t status = meas_dsp_fft_init(&ctx->fft_ctx, length, inverse);
  if (status != MEAS_OK) {
    return status;
  }

  node->api = &node_fft_api;
  node->impl = ctx;
  node->next = NULL;
  node->ref_count = 1;
  return MEAS_OK;
}

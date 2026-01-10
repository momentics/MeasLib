/**
 * @file dsp.c
 * @brief DSP Engine Implementation (Stub/C-Reference).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/dsp/dsp.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Placeholder for real FFT impl or CMSIS-DSP wrapper
meas_status_t meas_dsp_fft_init(meas_dsp_fft_t *ctx, size_t length,
                                bool inverse) {
  if (!ctx)
    return MEAS_ERROR;
  ctx->length = length;
  ctx->inverse = inverse;
  ctx->backend_handle = NULL;
  return MEAS_OK;
}

meas_status_t meas_dsp_fft_exec(meas_dsp_fft_t *ctx,
                                const meas_complex_t *input,
                                meas_complex_t *output) {
  // Stub: Copy input to output (No-Op)
  // Real implementation requires bit-reversal and butterfly Ops
  if (!ctx || !input || !output)
    return MEAS_ERROR;
  memcpy(output, input, ctx->length * sizeof(meas_complex_t));
  return MEAS_OK;
}

meas_status_t meas_dsp_apply_window(meas_real_t *buffer, size_t size,
                                    meas_dsp_window_t win_type) {
  if (!buffer)
    return MEAS_ERROR;

  for (size_t i = 0; i < size; i++) {
    meas_real_t w = 1.0;
    meas_real_t ratio = (meas_real_t)i / (size - 1);

    switch (win_type) {
    case DSP_WINDOW_RECT:
      w = 1.0;
      break;
    case DSP_WINDOW_HANN:
      w = 0.5 * (1.0 - cos(2.0 * M_PI * ratio));
      break;
    case DSP_WINDOW_HAMMING:
      w = 0.54 - 0.46 * cos(2.0 * M_PI * ratio);
      break;
    case DSP_WINDOW_BLACKMAN:
      w = 0.42 - 0.5 * cos(2.0 * M_PI * ratio) + 0.08 * cos(4.0 * M_PI * ratio);
      break;
    }
    buffer[i] *= w;
  }
  return MEAS_OK;
}

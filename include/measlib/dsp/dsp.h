/**
 * @file dsp.h
 * @brief DSP Engine Abstraction.
 *  
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Wraps hardware-accelerated DSP functions (FFT, Windowing) or provides
 * software fallbacks.
 */

#ifndef MEASLIB_DSP_DSP_H
#define MEASLIB_DSP_DSP_H

#include "measlib/types.h"

/**
 * @brief Window Functions
 */
typedef enum {
  DSP_WINDOW_RECT,
  DSP_WINDOW_HANN,
  DSP_WINDOW_HAMMING,
  DSP_WINDOW_BLACKMAN
} meas_dsp_window_t;

/**
 * @brief FFT Context
 * Holds state for the FFT backend (tables, temporary buffers).
 */
typedef struct {
  size_t length;        /**< FFT Size */
  bool inverse;         /**< True for iFFT */
  void *backend_handle; /**< Opaque handle for underlying lib (e.g.
                           arm_rfft_instance) */
} meas_dsp_fft_t;

/**
 * @brief Initialize FFT Context.
 */
meas_status_t meas_dsp_fft_init(meas_dsp_fft_t *ctx, size_t length,
                                bool inverse);

/**
 * @brief Execute FFT/iFFT.
 * @param ctx Configured context.
 * @param input Input array (Complex).
 * @param output Output array (Complex).
 */
meas_status_t meas_dsp_fft_exec(meas_dsp_fft_t *ctx,
                                const meas_complex_t *input,
                                meas_complex_t *output);

/**
 * @brief Apply Window Function.
 * Multiplies data buffer by the generated window coefficients.
 */
meas_status_t meas_dsp_apply_window(meas_real_t *buffer, size_t size,
                                    meas_dsp_window_t win_type);

#endif // MEASLIB_DSP_DSP_H

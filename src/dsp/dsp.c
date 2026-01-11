/**
 * @file dsp.c
 * @brief DSP Engine Implementation (Stub/C-Reference).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/dsp/dsp.h"
#include "measlib/boards/dsp_ops.h"
#include "measlib/utils/math.h"
#include <math.h>
#include <string.h>

// ============================================================================
// Mixing (DDC)
// ============================================================================

/**
 * @brief Down-converts (mixes) input samples with a local oscillator (LO) sine
 * table.
 *
 * Performs digital down-conversion (DDC) by multiplying input samples with sine
 * and cosine components from a lookup table. Supports accumulation of results
 * for VNA processing.
 *
 * @param input Pointer to the input signal buffer (int16_t).
 * @param length Number of samples to process.
 * @param sin_table Pointer to the sine lookup table (int16_t).
 * @param acc_i [in/out] Accumulator for In-phase component signal.
 * @param acc_q [in/out] Accumulator for Quadrature component signal.
 * @param ref_i [in/out] Accumulator for In-phase reference signal.
 * @param ref_q [in/out] Accumulator for Quadrature reference signal.
 * @return MEAS_OK on success, MEAS_ERROR if inputs are invalid.
 */
meas_status_t meas_dsp_mix_down(const int16_t *input, size_t length,
                                const int16_t *sin_table, int64_t *acc_i,
                                int64_t *acc_q, int64_t *ref_i,
                                int64_t *ref_q) {
  if (!input || !sin_table || !acc_i || !acc_q || !ref_i || !ref_q)
    return MEAS_ERROR;

  // Initialize accumulators
  int64_t si = 0, sq = 0, ri = 0, rq = 0;
  size_t i = 0;

  // Process pairs (Ref+Samp)
  size_t limit = length / 2; // Processing pairs

  for (i = 0; i < limit; i++) {
    int32_t sc = ((int32_t *)sin_table)[i];
    int32_t sr = ((int32_t *)input)[i];

    // Using DSP Ops
    si = meas_dsp_ops_smlaltb(si, sr, sc); // Samp * Sin
    sq = meas_dsp_ops_smlaltt(sq, sr, sc); // Samp * Cos
    ri = meas_dsp_ops_smlalbb(ri, sr, sc); // Ref * Sin
    rq = meas_dsp_ops_smlalbt(rq, sr, sc); // Ref * Cos
  }

  // Commit to output
  *acc_i += si;
  *acc_q += sq;
  *ref_i += ri;
  *ref_q += rq;

  return MEAS_OK;
}

// ============================================================================
// VNA Specifics
// ============================================================================

/**
 * @brief Calculates the complex reflection coefficient (Gamma) from raw
 * accumulators.
 *
 * Computed as Gamma = (Sample / Reference).
 *
 * @param acc_samp_i Accumulated In-phase sample value.
 * @param acc_samp_q Accumulated Quadrature sample value.
 * @param acc_ref_i Accumulated In-phase reference value.
 * @param acc_ref_q Accumulated Quadrature reference value.
 * @param gamma [out] Pointer to store the calculated complex Gamma.
 */
void meas_dsp_gamma_calc(int64_t acc_samp_i, int64_t acc_samp_q,
                         int64_t acc_ref_i, int64_t acc_ref_q,
                         meas_complex_t *gamma) {
  if (!gamma)
    return;

  meas_real_t rs = (meas_real_t)acc_ref_i;
  meas_real_t rc = (meas_real_t)acc_ref_q;
  meas_real_t ss = (meas_real_t)acc_samp_i;
  meas_real_t sc = (meas_real_t)acc_samp_q;

  meas_real_t mag_sq = rc * rc + rs * rs;
  if (mag_sq < MEAS_EPSILON) {
    gamma->re = 0.0;
    gamma->im = 0.0;
    return;
  }

  meas_real_t inv = 1.0 / mag_sq;
  gamma->re = (sc * rc + ss * rs) * inv;
  gamma->im = (ss * rc - sc * rs) * inv;
}

/**
 * @brief Rotates the phase of a complex number (Gamma) to compensate for
 * electrical delay.
 *
 * @param gamma [in/out] Complex number to rotate.
 * @param frequency_hz Measurement frequency in Hz.
 * @param delay_s Electrical delay in seconds.
 */
void meas_dsp_phase_rotate(meas_complex_t *gamma, double frequency_hz,
                           double delay_s) {
  if (!gamma)
    return;

  double theta = -2.0 * MEAS_PI * frequency_hz * delay_s;
  meas_real_t c, s;
  meas_math_sincos((meas_real_t)theta, &s, &c);

  meas_real_t x = gamma->re;
  meas_real_t y = gamma->im;

  gamma->re = x * c - y * s;
  gamma->im = x * s + y * c;
}

/**
 * @brief Decimates a signal by averaging (Boxcar filter).
 *
 * Reduces the sample rate by summing 'factor' samples and dividing by 'factor'.
 *
 * @param input Input signal buffer.
 * @param in_len Number of input samples.
 * @param output Output signal buffer.
 * @param out_len [in/out] Input: Max capacity of output. Output: Actual samples
 * written.
 * @param factor Decimation factor (must be >= 1).
 * @return MEAS_OK on success, MEAS_ERROR on invalid arguments.
 */
meas_status_t meas_dsp_decimate(const meas_real_t *input, size_t in_len,
                                meas_real_t *output, size_t *out_len,
                                size_t factor) {
  if (!input || !output || !out_len || factor == 0)
    return MEAS_ERROR;

  size_t count = 0;
  // Simple integer decimation with averaging (Boxcar filter characteristic)
  // Logic: Sum 'factor' samples, divide by 'factor', output 1 sample.
  meas_real_t sum = 0.0f;
  size_t p = 0;

  for (size_t i = 0; i < in_len; i++) {
    sum += input[i];
    p++;
    if (p >= factor) {
      if (count < *out_len) {
        output[count++] = sum / (meas_real_t)factor;
      }
      sum = 0.0f;
      p = 0;
    }
  }

  *out_len = count;
  return MEAS_OK;
}

// ============================================================================
// DMM / SA / Gen
// ============================================================================

/**
 * @brief Performs the Goertzel algorithm to compute magnitude and phase at a
 * specific frequency.
 *
 * More efficient than FFT for a single frequency bin analysis.
 *
 * @param input Input signal buffer (int16_t).
 * @param length Number of samples.
 * @param target_freq Target frequency to analyze in Hz.
 * @param sample_rate Sampling rate in Hz.
 * @param magnitude [out] Pointer to store calculated magnitude.
 * @param phase [out] Pointer to store calculated phase (optional, can be NULL).
 * @return MEAS_OK on success.
 */
meas_status_t meas_dsp_goertzel(const int16_t *input, size_t length,
                                float target_freq, float sample_rate,
                                float *magnitude, float *phase) {
  if (!input || !magnitude)
    return MEAS_ERROR;

  // k = (N * f) / fs  <- Unused

  meas_real_t omega = (2.0 * MEAS_PI * target_freq) / sample_rate;
  meas_real_t cosine, sine;
  meas_math_sincos(omega, &sine, &cosine); // Expects meas_real_t*

  meas_real_t coeff = 2.0 * cosine;
  meas_real_t q0 = 0.0, q1 = 0.0, q2 = 0.0;

  for (size_t i = 0; i < length; i++) {
    q2 = q1;
    q1 = q0;
    q0 = (meas_real_t)input[i] + coeff * q1 - q2;
  }

  meas_real_t real = q1 - q2 * cosine;
  meas_real_t imag = q2 * sine;

  meas_complex_t z = {.re = real, .im = imag};
  *magnitude = (float)meas_cabs(z);
  if (phase) {
    *phase = (float)meas_math_atan2(imag, real);
  }

  return MEAS_OK;
}

/**
 * @brief Calculates optimal FFT length and decimation factor for a desired
 * Resolution Bandwidth (RBW).
 *
 * @param rbw Desired Resolution Bandwidth in Hz.
 * @param sample_rate Input sampling rate in Hz.
 * @param fft_len [out] Calculated FFT length (e.g., 512).
 * @param decimation [out] Calculated decimation factor.
 * @param effective_rbw [out] Actual achieved RBW (optional, can be NULL).
 * @return MEAS_OK on success.
 */
meas_status_t meas_dsp_rbw_calc(float rbw, float sample_rate, size_t *fft_len,
                                size_t *decimation, float *effective_rbw) {
  if (!fft_len || !decimation)
    return MEAS_ERROR;

  // Logic:
  // Resolution = Fs / N
  // If we want specific RBW, we need N = Fs / RBW.
  // But N is limited (e.g. 256, 512, 1024).
  // So we might need to decimate Fs first. Fs_new = Fs / D.
  // Then Resolution = Fs_new / N = Fs / (D * N).
  // We want Fs / (D * N) <= RBW.
  // So D * N >= Fs / RBW.

  // Constraints: N usually 256, 512, 1024.
  // Max decimation?
  // Let's assume standard sizes for MeasLib.

  size_t preferred_N = 512;
  float required_product = sample_rate / rbw;

  size_t d = (size_t)ceilf(required_product / preferred_N);
  if (d < 1)
    d = 1;

  // Round D to likely integer or power of 2 if needed?
  // For simple averaging decimation, any int is fine.

  *fft_len = preferred_N;
  *decimation = d;

  if (effective_rbw) {
    *effective_rbw = sample_rate / (float)(d * preferred_N);
  }

  return MEAS_OK;
}

/**
 * @brief Generates a digital waveform (DDS).
 *
 * Supports Sine, Square, Triangle, and Sawtooth waveforms.
 *
 * @param buffer Output buffer for generated samples (int16_t).
 * @param length Number of samples to generate.
 * @param freq Output frequency in Hz.
 * @param sample_rate Sampling rate in Hz.
 * @param type Waveform type.
 * @param phase_acc [in/out] Phase accumulator state (optional, for continuous
 * generation).
 * @return MEAS_OK on success.
 */
meas_status_t meas_dsp_dds_gen(int16_t *buffer, size_t length, float freq,
                               float sample_rate, meas_dsp_wave_t type,
                               uint32_t *phase_acc) {
  if (!buffer)
    return MEAS_ERROR;

  uint32_t phase = phase_acc ? *phase_acc : 0;
  // Phase accumulator is 32-bit.
  // Increment = (freq / fs) * 2^32
  uint32_t increment = (uint32_t)((freq / sample_rate) * 4294967296.0f);

  for (size_t i = 0; i < length; i++) {
    meas_real_t sample = 0.0f;

    // Normalized phase 0..1
    meas_real_t norm_phase = (meas_real_t)phase / 4294967296.0f;

    switch (type) {
    case DSP_WAVE_SINE: {
      meas_real_t s, c;
      // angle = 0..2PI
      meas_math_sincos(norm_phase * 2.0f * (float)MEAS_PI, &s, &c);
      sample = s;
      break;
    }
    case DSP_WAVE_SQUARE:
      sample = (norm_phase < 0.5f) ? 1.0f : -1.0f;
      break;
    case DSP_WAVE_TRIANGLE:
      // 0..0.5 -> 0..1, 0.5..1 -> 1..-1..0 ??
      // Standard triangle: 4*x - 1 (0..0.5), ...
      // 0 -> -1
      // 0.5 -> 1
      // 1 -> -1
      if (norm_phase < 0.5f) {
        sample = -1.0f + 4.0f * norm_phase;
      } else {
        sample = 1.0f - 4.0f * (norm_phase - 0.5f);
      }
      break;
    case DSP_WAVE_SAWTOOTH:
      sample = 2.0f * norm_phase - 1.0f;
      break;
    }

    // Convert to int16 (scale by 32000 for headroom)
    buffer[i] = (int16_t)(sample * 32000.0f);

    phase += increment;
  }

  if (phase_acc)
    *phase_acc = phase;

  return MEAS_OK;
}

// ============================================================================
// FFT / Windowing
// ============================================================================

/**
 * @brief Initializes the FFT context.
 *
 * @param ctx Pointer to FFT context structure.
 * @param length FFT length (must be a power of 2).
 * @param inverse Set to true for Inverse FFT (IFFT), false for forward FFT.
 * @return MEAS_OK on success.
 */
meas_status_t meas_dsp_fft_init(meas_dsp_fft_t *ctx, size_t length,
                                bool inverse) {
  if (!ctx)
    return MEAS_ERROR;
  ctx->length = length;
  ctx->inverse = inverse;
  ctx->backend_handle = NULL;
  return MEAS_OK;
}

static uint32_t reverse_bits(uint32_t x, uint32_t bits) {
  uint32_t result = 0;
  for (uint32_t i = 0; i < bits; i++) {
    result = (result << 1) | (x & 1);
    x >>= 1;
  }
  return result;
}

/**
 * @brief Executes the FFT or IFFT algorithm.
 *
 * Uses the Cooley-Tukey algorithm with in-place bit-reversal capabilities.
 *
 * @param ctx Pointer to initialized FFT context.
 * @param input Input complex buffer.
 * @param output Output complex buffer (can be same as input for in-place).
 * @return MEAS_OK on success, MEAS_ERROR if length is not power of 2.
 */
meas_status_t meas_dsp_fft_exec(meas_dsp_fft_t *ctx,
                                const meas_complex_t *input,
                                meas_complex_t *output) {
  if (!ctx || !input || !output)
    return MEAS_ERROR;

  size_t n = ctx->length;
  // Check if n is power of 2
  if ((n & (n - 1)) != 0)
    return MEAS_ERROR; // FFT requires power of 2

  // Determine number of bits for reversal
  uint32_t levels = 0;
  size_t temp = n;
  while (temp >>= 1)
    levels++;

  // 1. Bit Reversal Permutation
  if (input != output) {
    for (size_t i = 0; i < n; i++) {
      size_t j = reverse_bits(i, levels);
      output[j] = input[i];
    }
  } else {
    // In-place bit-reversal swap
    for (size_t i = 0; i < n; i++) {
      size_t j = reverse_bits(i, levels);
      if (j > i) {
        meas_complex_t temp_val = output[i];
        output[i] = output[j];
        output[j] = temp_val;
      }
    }
  }

  // 2. Cooley-Tukey Butterfly
  for (size_t size = 2; size <= n; size *= 2) {
    size_t halfsize = size / 2;

    // ctx->inverse: True=IFFT (+j), False=FFT (-j)
    meas_real_t angle_step = (ctx->inverse ? 2.0 : -2.0) * MEAS_PI / size;

    meas_real_t wn_re, wn_im;
    meas_math_sincos(angle_step, &wn_im, &wn_re); // sin, cos

    for (size_t i = 0; i < n; i += size) {
      meas_real_t w_re = 1.0;
      meas_real_t w_im = 0.0;

      for (size_t j = 0; j < halfsize; j++) {
        size_t k = i + j;
        size_t l = k + halfsize;

        meas_complex_t u = output[k];
        meas_complex_t v = output[l];

        // t = w * v
        meas_real_t t_re = w_re * v.re - w_im * v.im;
        meas_real_t t_im = w_re * v.im + w_im * v.re;

        // Butterfly
        output[k].re = u.re + t_re;
        output[k].im = u.im + t_im;
        output[l].re = u.re - t_re;
        output[l].im = u.im - t_im;

        // Update W
        meas_real_t next_w_re = w_re * wn_re - w_im * wn_im;
        meas_real_t next_w_im = w_re * wn_im + w_im * wn_re;
        w_re = next_w_re;
        w_im = next_w_im;
      }
    }
  }

  // 3. Normalization for IFFT
  if (ctx->inverse) {
    meas_real_t inv_n = 1.0 / n;
    for (size_t i = 0; i < n; i++) {
      output[i].re *= inv_n;
      output[i].im *= inv_n;
    }
  }

  return MEAS_OK;
}

/**
 * @brief Applies a window function to a real-valued signal buffer.
 *
 * Supports various window types (Hann, Hamming, Blackman, Rectangular).
 *
 * @param buffer Signal buffer to window (in-place modification).
 * @param size Number of samples in the buffer.
 * @param win_type Type of window function to apply.
 * @return MEAS_OK on success.
 */
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
    case DSP_WINDOW_HANN: {
      meas_real_t c;
      meas_math_sincos(2.0 * MEAS_PI * ratio, NULL, &c);
      w = 0.5 * (1.0 - c);
      break;
    }
    case DSP_WINDOW_HAMMING: {
      meas_real_t c;
      meas_math_sincos(2.0 * MEAS_PI * ratio, NULL, &c);
      w = 0.54 - 0.46 * c;
      break;
    }
    case DSP_WINDOW_BLACKMAN: {
      meas_real_t c1, c2;
      meas_math_sincos(2.0 * MEAS_PI * ratio, NULL, &c1);
      meas_math_sincos(4.0 * MEAS_PI * ratio, NULL, &c2);
      w = 0.42 - 0.5 * c1 + 0.08 * c2;
      break;
    }
    }
    buffer[i] *= w;
  }
  return MEAS_OK;
}

// ============================================================================
// Shared Tables
// ============================================================================

int16_t meas_dsp_sin_table_1024[1024];

/**
 * @brief Initializes global DSP lookup tables.
 *
 * Typically generates a sine wave table used for mixing and signal generation.
 *
 * @return MEAS_OK on success.
 */
meas_status_t meas_dsp_tables_init(void) {
  // Generate one cycle of sine wave
  // Freq = 1 Hz, Sample Rate = 1024 Hz -> 1024 samples/cycle
  return meas_dsp_dds_gen(meas_dsp_sin_table_1024, 1024, 1.0f, 1024.0f,
                          DSP_WAVE_SINE, NULL);
}

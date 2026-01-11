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

// ============================================================================
// Signal Processing (VNA/SA/GEN/DMM)
// ============================================================================

/**
 * @brief Digital Mix Down (DDC).
 * Mixes input samples with a reference
 * oscillator (sine table) to downconvert.
 * Uses hardware acceleration if available.
 *
 * @param input Input samples (I16: Q15
 * format typically).
 * @param length Number of samples.
 * @param sin_table Pointer to sine/cosine
 * table.
 * @param[out] acc_i Accumulator for I
 * component (mix * cos).
 * @param[out] acc_q Accumulator for Q
 * component (mix * sin).
 * @param[out] ref_i Accumulator for Ref I.
 * @param[out] ref_q Accumulator for Ref Q.
 */
meas_status_t meas_dsp_mix_down(const int16_t *input, size_t length,
                                const int16_t *sin_table, int64_t *acc_i,
                                int64_t *acc_q, int64_t *ref_i, int64_t *ref_q);

/**
 * @brief Calculate Gamma (Reflection
 * Coefficient). Gamma = (Sample /
 * Reference).
 *
 * @param acc_samp_i Sample I accumulator.
 * @param acc_samp_q Sample Q accumulator.
 * @param acc_ref_i Reference I accumulator.
 * @param acc_ref_q Reference Q accumulator.
 * @param[out] gamma Resulting complex
 * gamma.
 */
void meas_dsp_gamma_calc(int64_t acc_samp_i, int64_t acc_samp_q,
                         int64_t acc_ref_i, int64_t acc_ref_q,
                         meas_complex_t *gamma);

/**
 * @brief Phase Rotation (Electrical Delay).
 * Applies a phase shift corresponding to a
 * time delay at a given frequency.
 * Gamma_new = Gamma_old * exp(j * 2 * pi *
 * f * delay)
 *
 * @param gamma Input/Output Gamma.
 * @param frequency_hz Frequency in Hz.
 * @param delay_s Delay in seconds.
 */
void meas_dsp_phase_rotate(meas_complex_t *gamma, double frequency_hz,
                           double delay_s);

/**
 * @brief Decimate and Filter (FIR).
 * Detailed implementation pending.
 */
meas_status_t meas_dsp_decimate(const meas_real_t *input, size_t in_len,
                                meas_real_t *output, size_t *out_len,
                                size_t factor);

/**
 * @brief Goertzel Algorithm (Single Tone Detection).
 * efficiently calculates the magnitude and phase of a specific frequency
 * component.
 *
 * @param input Input samples (int16_t Q15).
 * @param length Number of samples.
 * @param target_freq Frequency to detect (Hz).
 * @param sample_rate Sampling rate (Hz).
 * @param[out] magnitude Detected magnitude.
 * @param[out] phase Detected phase (radians).
 */
meas_status_t meas_dsp_goertzel(const int16_t *input, size_t length,
                                float target_freq, float sample_rate,
                                float *magnitude, float *phase);

/**
 * @brief Spectrum Analyzer Parameter Calculation.
 * Determines optimal FFT size and Decimation factor for a desired RBW.
 *
 * @param rbw Desired Resolution Bandwidth (Hz).
 * @param sample_rate ADC Sampling Rate (Hz).
 * @param[out] fft_len Recommended FFT length (power of 2).
 * @param[out] decimation Recommended decimation factor.
 * @param[out] effective_rbw Actual RBW achievable.
 */
meas_status_t meas_dsp_rbw_calc(float rbw, float sample_rate, size_t *fft_len,
                                size_t *decimation, float *effective_rbw);

/**
 * @brief Waveform Types for DDS.
 */
typedef enum {
  DSP_WAVE_SINE,
  DSP_WAVE_SQUARE,
  DSP_WAVE_TRIANGLE,
  DSP_WAVE_SAWTOOTH
} meas_dsp_wave_t;

/**
 * @brief Direct Digital Synthesis (DDS) Generator.
 * Generates a waveform into the buffer.
 *
 * @param buffer Output buffer.
 * @param length Buffer length.
 * @param freq Output frequency (Hz).
 * @param sample_rate Sampling rate (Hz).
 * @param type Waveform type.
 * @param[in,out] phase_acc Pointer to phase accumulator (state).
 */
meas_status_t meas_dsp_dds_gen(int16_t *buffer, size_t length, float freq,
                               float sample_rate, meas_dsp_wave_t type,
                               uint32_t *phase_acc);

/**
 * @brief Shared Sine Table (1024 points, Int16).
 * Used by DDC nodes to avoid duplication.
 */
extern int16_t meas_dsp_sin_table_1024[1024];

/**
 * @brief Initialize Shared DSP Tables.
 * Must be called at startup.
 */
meas_status_t meas_dsp_tables_init(void);

#endif // MEASLIB_DSP_DSP_H

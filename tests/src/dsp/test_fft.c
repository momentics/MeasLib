/**
 * @file test_fft.c
 * @brief Unit Tests for FFT DSP.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Verifies the in-place FFT execution using simple signals (Impulse, DC).
 */

#include "measlib/dsp/dsp.h"
#include "measlib/utils/math.h"
#include "test_framework.h"
#include <math.h>

#define FFT_TEST_LEN 1024

static meas_complex_t fft_buf[FFT_TEST_LEN];

static meas_dsp_fft_t fft_ctx;

void test_fft_impulse(void) {
  // 1. Setup Impulse (Delta function) at t=0
  memset(fft_buf, 0, sizeof(fft_buf));
  fft_buf[0].re = 1.0f;
  fft_buf[0].im = 0.0f;

  // 2. Initialize and Run FFT (In-place)
  meas_dsp_fft_init(&fft_ctx, FFT_TEST_LEN, false);
  meas_dsp_fft_exec(&fft_ctx, fft_buf, fft_buf);

  // 3. Verify
  float max_err = 0.0f;
  for (int i = 0; i < FFT_TEST_LEN; i++) {
    float err_re = fabs(fft_buf[i].re - 1.0f);
    float err_im = fabs(fft_buf[i].im - 0.0f);
    if (err_re > max_err)
      max_err = err_re;
    if (err_im > max_err)
      max_err = err_im;
  }
  printf("[FFT] Impulse: Max Err=%.6f\n", max_err);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0f, max_err);
}

void test_fft_dcV(void) {
  // 1. Constant DC signal [1, 1, 1...]
  for (int i = 0; i < FFT_TEST_LEN; i++) {
    fft_buf[i].re = 1.0f;
    fft_buf[i].im = 0.0f;
  }

  // 2. Run FFT
  meas_dsp_fft_init(&fft_ctx, FFT_TEST_LEN, false);
  meas_dsp_fft_exec(&fft_ctx, fft_buf, fft_buf);

  // 3. Verify
  float max_err_dc = fabs(fft_buf[0].re - (float)FFT_TEST_LEN);
  float max_err_other = 0.0f;

  for (int i = 1; i < FFT_TEST_LEN; i++) {
    float mag =
        sqrtf(fft_buf[i].re * fft_buf[i].re + fft_buf[i].im * fft_buf[i].im);
    if (mag > max_err_other)
      max_err_other = mag;
  }
  printf("[FFT] DC: DC Err=%.6f, Max Leakage=%.6f\n", max_err_dc,
         max_err_other);

  TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0f, max_err_dc);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0f, max_err_other);
}

void test_fft_sine(void) {
  // 1. Generate Sine Wave at bin 8 (k=8)
  for (int i = 0; i < FFT_TEST_LEN; i++) {
    meas_real_t angle =
        (meas_real_t)(2.0 * MEAS_PI * 8.0 * (double)i / (double)FFT_TEST_LEN);
    meas_real_t s, c;
    meas_math_sincos(angle, &s, &c);
    fft_buf[i].re = c;
    fft_buf[i].im = 0.0f;
  }

  // 2. Run FFT
  meas_dsp_fft_init(&fft_ctx, FFT_TEST_LEN, false);
  meas_dsp_fft_exec(&fft_ctx, fft_buf, fft_buf);

  // 3. Verify
  int peak_bin = 0;
  float peak_mag = 0.0f;
  float max_side_lobe = 0.0f;

  for (int i = 0; i < FFT_TEST_LEN; i++) { // Check full spectrum
    float m =
        sqrtf(fft_buf[i].re * fft_buf[i].re + fft_buf[i].im * fft_buf[i].im);
    if (m > peak_mag) {
      peak_mag = m;
      peak_bin = i;
    } else {
      // Check side lobes (excluding symmetric peak at N-k if real input,
      // but here complex input with 0 im is effectively real, so spectrum has
      // conjugate symmetry) For k=8, we expect peak at 8 and 1016 (N-8).
      if (i != 8 && i != (FFT_TEST_LEN - 8)) {
        if (m > max_side_lobe)
          max_side_lobe = m;
      }
    }
  }

  float peak_err = fabsf(peak_mag - 512.0f);
  printf(
      "[FFT] Sine: Peak Bin=%d, Peak Mag=%.2f (Err: %.2f), Max Leakage=%.6f\n",
      peak_bin, peak_mag, peak_err, max_side_lobe);

  TEST_ASSERT_EQUAL(8, peak_bin);
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, peak_err);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0f, max_side_lobe);
}

#include <time.h>

void test_fft_perf(void) {
  const int iterations = 20000;
  meas_dsp_fft_init(&fft_ctx, FFT_TEST_LEN, false);

  // Prepare buffer
  for (int i = 0; i < FFT_TEST_LEN; i++) {
    fft_buf[i].re = (float)i;
    fft_buf[i].im = 0.0f;
  }

  clock_t start = clock();
  for (int i = 0; i < iterations; i++) {
    meas_dsp_fft_exec(&fft_ctx, fft_buf, fft_buf);
  }
  clock_t end = clock();

  double time_s = (double)(end - start) / CLOCKS_PER_SEC;
  double ops_per_sec = (double)iterations / time_s;

  printf("[PERF] FFT (%d pts): %d ops in %.4f s (%.1f FFTs/s)\n", FFT_TEST_LEN,
         iterations, time_s, ops_per_sec);
}

void run_fft_tests(void) {
  printf("--- Running FFT Tests ---\n");
  RUN_TEST(test_fft_impulse);
  RUN_TEST(test_fft_dcV);
  RUN_TEST(test_fft_sine);
  RUN_TEST(test_fft_perf);
}

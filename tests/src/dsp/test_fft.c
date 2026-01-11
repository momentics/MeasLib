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
  // FFT of delta should be constant magnitude across all bins (DC).
  // Actually, FFT of [1, 0, 0...] is [1, 1, 1...].

  memset(fft_buf, 0, sizeof(fft_buf));
  fft_buf[0].re = 1.0f;
  fft_buf[0].im = 0.0f;

  // 2. Initialize and Run FFT (In-place)
  meas_dsp_fft_init(&fft_ctx, FFT_TEST_LEN, false);
  meas_dsp_fft_exec(&fft_ctx, fft_buf, fft_buf);

  // 3. Verify
  // All bins should have Re=1.0, Im=0.0
  for (int i = 0; i < FFT_TEST_LEN; i++) {
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 1.0f, fft_buf[i].re);
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0f, fft_buf[i].im);
  }
}

void test_fft_dcV(void) {
  // 1. Constant DC signal [1, 1, 1...]
  // FFT shoud be [N, 0, 0...] (Impulse at f=0)
  for (int i = 0; i < FFT_TEST_LEN; i++) {
    fft_buf[i].re = 1.0f;
    fft_buf[i].im = 0.0f;
  }

  // 2. Run FFT
  meas_dsp_fft_init(&fft_ctx, FFT_TEST_LEN, false);
  meas_dsp_fft_exec(&fft_ctx, fft_buf, fft_buf);

  // 3. Verify
  // DC bin (0) should be N (1024)
  TEST_ASSERT_FLOAT_WITHIN(1e-3, (float)FFT_TEST_LEN, fft_buf[0].re);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0f, fft_buf[0].im);

  // Other bins should be 0
  for (int i = 1; i < FFT_TEST_LEN; i++) {
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0f, fft_buf[i].re);
    TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0f, fft_buf[i].im);
  }
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

  // 3. Verify Peak at bin 8
  int peak_bin = 0;
  float peak_mag = 0.0f;

  for (int i = 0; i < 32; i++) {
    float m =
        sqrtf(fft_buf[i].re * fft_buf[i].re + fft_buf[i].im * fft_buf[i].im);
    if (m > peak_mag) {
      peak_mag = m;
      peak_bin = i;
    }
  }

  // Verification:
  // 1. Peak must be at Bin 8
  TEST_ASSERT_EQUAL(8, peak_bin);

  // 2. Magnitude should be ~512 (N/2) for amplitude 1.0 cosine
  // Allow small margin for floating point errors
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 512.0f, peak_mag);
}

#include <time.h>

void test_fft_perf(void) {
  const int iterations = 1000;
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

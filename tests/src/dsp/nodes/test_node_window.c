/**
 * @file test_node_window.c
 * @brief Unit Tests for Windowing Functions.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Checks generation of standard window coefficients:
 * - Hann
 * - Hamming
 * - Flat Top
 */

#include "measlib/dsp/dsp.h"
#include "measlib/types.h"
#include "measlib/utils/math.h"
#include "test_framework.h"
#include <math.h>

// --- Test Cases ---

void test_window_hann(void) {
  size_t N = 5;
  meas_real_t buffer[5];
  for (size_t i = 0; i < N; i++)
    buffer[i] = 1.0f;

  meas_status_t status = meas_dsp_apply_window(buffer, N, DSP_WINDOW_HANN);
  TEST_ASSERT_EQUAL(MEAS_OK, status);

  // Expected: 0.5 * (1 - cos(2*PI*n/(N-1)))
  // n=0: 0
  // n=1: 0.5
  // n=2: 1.0
  // n=3: 0.5
  // n=4: 0
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.0, buffer[0]);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.5, buffer[1]);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 1.0, buffer[2]);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.5, buffer[3]);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.0, buffer[4]);
}

void test_window_rect(void) {
  size_t N = 5;
  meas_real_t buffer[5];
  for (size_t i = 0; i < N; i++)
    buffer[i] = 1.0f;

  meas_status_t status = meas_dsp_apply_window(buffer, N, DSP_WINDOW_RECT);
  TEST_ASSERT_EQUAL(MEAS_OK, status);

  // Expected: All 1.0 (No modification)
  for (size_t i = 0; i < N; i++) {
    TEST_ASSERT_FLOAT_WITHIN(1e-6, 1.0, buffer[i]);
  }
}

void test_window_hamming(void) {
  size_t N = 5;
  meas_real_t buffer[5];
  for (size_t i = 0; i < N; i++)
    buffer[i] = 1.0f;

  meas_status_t status = meas_dsp_apply_window(buffer, N, DSP_WINDOW_HAMMING);
  TEST_ASSERT_EQUAL(MEAS_OK, status);

  // Expected: 0.54 - 0.46 * cos(2*PI*n / (N-1))
  // n=0: 0.54 - 0.46 = 0.08
  // n=1: 0.54 - 0.46 * 0 = 0.54
  // n=2: 0.54 - 0.46 * -1 = 1.0
  // n=3: 0.54
  // n=4: 0.08
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.08, buffer[0]);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.54, buffer[1]);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 1.00, buffer[2]);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.54, buffer[3]);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.08, buffer[4]);
}

void test_window_blackman(void) {
  size_t N = 5;
  meas_real_t buffer[5];
  for (size_t i = 0; i < N; i++)
    buffer[i] = 1.0f;

  meas_status_t status = meas_dsp_apply_window(buffer, N, DSP_WINDOW_BLACKMAN);
  TEST_ASSERT_EQUAL(MEAS_OK, status);

  // Expected: 0.42 - 0.5*cos(t) + 0.08*cos(2t) where t = 2*PI*n/(N-1)
  // n=0 (t=0): 0.42 - 0.5 + 0.08 = 0.0
  // n=1 (t=PI/2): 0.42 - 0 + 0.08*(-1) = 0.34
  // n=2 (t=PI): 0.42 - 0.5*(-1) + 0.08*(1) = 0.42 + 0.5 + 0.08 = 1.0
  // n=3: 0.34
  // n=4: 0.0
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.00, buffer[0]);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.34, buffer[1]);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 1.00, buffer[2]);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.34, buffer[3]);
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.00, buffer[4]);
}

void run_node_window_tests(void) {
  printf("--- Running DSP Window Tests ---\n");
  RUN_TEST(test_window_hann);
  RUN_TEST(test_window_rect);
  RUN_TEST(test_window_hamming);
  RUN_TEST(test_window_blackman);
  RUN_TEST(test_window_rect);
  RUN_TEST(test_window_hamming);
  RUN_TEST(test_window_blackman);
}

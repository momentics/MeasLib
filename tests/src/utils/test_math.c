/**
 * @file test_math.c
 * @brief Unit Tests for Math Utils.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Verifies the precision of math approximations against standard C library
 * functions.
 */

#include "measlib/utils/math.h"
#include "test_framework.h"
#include <math.h>

void test_math_trig(void) {
  // Test Sin/Cos at various angles
  // 0
  meas_real_t s, c; // Use meas_real_t
  meas_math_sincos(0.0, &s, &c);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0, s);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 1.0, c);

  // PI/2 (1.57079...)
  meas_math_sincos(MEAS_PI / 2.0, &s, &c);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 1.0, s);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0, c);

  // PI (3.14159...)
  meas_math_sincos(MEAS_PI, &s, &c);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0, s);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, -1.0, c);

  // Random angle: 45 deg (PI/4)
  meas_math_sincos(MEAS_PI / 4.0, &s, &c);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.7071, s);
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.7071, c);
}

void test_math_atan2_simple(void) {
  // 0, 1 -> 0 deg
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.0, meas_math_atan2(0.0, 1.0));

  // 1, 0 -> 90 deg (PI/2 = 1.5708)
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 1.5708, meas_math_atan2(1.0, 0.0));

  // 1, 1 -> 45 deg (PI/4 = 0.7854)
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 0.7854, meas_math_atan2(1.0, 1.0));

  // -1, -1 -> -135 deg (-3PI/4 = -2.3562)
  TEST_ASSERT_FLOAT_WITHIN(1e-3, -2.3562, meas_math_atan2(-1.0, -1.0));
}

void test_math_sqrt(void) {
  // 1. Basic Squares
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 4.0, meas_math_sqrt(16.0));
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 10.0, meas_math_sqrt(100.0));

  // 2. Non-perfect squares
  TEST_ASSERT_FLOAT_WITHIN(1e-3, 1.414213, meas_math_sqrt(2.0));

  // 3. Zero
  TEST_ASSERT_FLOAT_WITHIN(1e-5, 0.0, meas_math_sqrt(0.0));
}

#include <time.h>

void test_math_accuracy(void) {
  // Sweep Atan2 across full circle
  const int steps = 3600; // 0.1 degree steps
  double max_err = 0.0;
  double sum_sq_err = 0.0;

  for (int i = 0; i < steps; i++) {
    double angle = (double)i * 2.0 * MEAS_PI / (double)steps - MEAS_PI;
    double x = cos(angle);
    double y = sin(angle);

    double expect = atan2(y, x);
    double actual = (double)meas_math_atan2((meas_real_t)y, (meas_real_t)x);
    double err = fabs(expect - actual);

    // Handle PI wrap-around diff
    if (err > MEAS_PI)
      err = 2.0 * MEAS_PI - err;

    if (err > max_err)
      max_err = err;
    sum_sq_err += err * err;
  }

  double rms = sqrt(sum_sq_err / steps);
  printf("[ACCURACY] Atan2: Max Err=%.6f, RMS=%.6f rad\n", max_err, rms);

  TEST_ASSERT_FLOAT_WITHIN(1e-2, 0.0, max_err); // Expect decent precision
}

void test_math_perf(void) {
  const int iterations = 1000000;
  clock_t start, end;
  volatile meas_real_t res;
  meas_real_t arg = 0.5f;

  // Sincos
  start = clock();
  for (int i = 0; i < iterations; i++) {
    meas_real_t s, c;
    meas_math_sincos(arg, &s, &c);
    res = s + c; // Prevent optimize out
    (void)res;
    arg += 0.0001f;
  }
  end = clock();
  double time_s = (double)(end - start) / CLOCKS_PER_SEC;
  printf("[PERF] Sincos: %d ops in %.4f s (%.1f Mops/s)\n", iterations, time_s,
         (iterations / time_s) / 1e6);

  // Atan2
  start = clock();
  for (int i = 0; i < iterations; i++) {
    res = meas_math_atan2(arg, 0.5f);
    arg += 0.0001f;
  }
  end = clock();
  time_s = (double)(end - start) / CLOCKS_PER_SEC;
  printf("[PERF] Atan2:  %d ops in %.4f s (%.1f Mops/s)\n", iterations, time_s,
         (iterations / time_s) / 1e6);

  // Log
  start = clock();
  for (int i = 0; i < iterations; i++) {
    res = meas_math_log(arg);
    arg += 0.0001f;
  }
  end = clock();
  time_s = (double)(end - start) / CLOCKS_PER_SEC;
  printf("[PERF] Log:    %d ops in %.4f s (%.1f Mops/s)\n", iterations, time_s,
         (iterations / time_s) / 1e6);

  // Cbrt
  start = clock();
  for (int i = 0; i < iterations; i++) {
    res = meas_math_cbrt(arg);
    arg += 0.0001f;
  }
  end = clock();
  time_s = (double)(end - start) / CLOCKS_PER_SEC;
  printf("[PERF] Cbrt:   %d ops in %.4f s (%.1f Mops/s)\n", iterations, time_s,
         (iterations / time_s) / 1e6);

  // Modf
  meas_real_t iptr;
  start = clock();
  for (int i = 0; i < iterations; i++) {
    res = meas_math_modf(arg, &iptr);
    arg += 0.0001f;
  }
  end = clock();
  time_s = (double)(end - start) / CLOCKS_PER_SEC;
  printf("[PERF] Modf:   %d ops in %.4f s (%.1f Mops/s)\n", iterations, time_s,
         (iterations / time_s) / 1e6);
}

void run_math_tests(void) {
  printf("--- Running Math Tests (meas_real_t size: %zu) ---\n",
         sizeof(meas_real_t));
  RUN_TEST(test_math_sqrt);
  RUN_TEST(test_math_trig);
  RUN_TEST(test_math_atan2_simple);
  RUN_TEST(test_math_accuracy);
  RUN_TEST(test_math_perf);
}

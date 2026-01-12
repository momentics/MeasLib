/**
 * @file test_fast_math.c
 * @brief Unit Tests for Internal Fast Math Approximations.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * This test file directly includes the source code of math.c to access
 * static internal functions (white-box testing). It defines F303 to
 * force the use of LUT-based implementations instead of libc fallbacks.
 */

#include "test_framework.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ----------------------------------------------------------------------------
// 1. Preparation: Rename Public API to avoid Linker Collisions
// ----------------------------------------------------------------------------
#define meas_math_interp_linear impl_meas_math_interp_linear
#define meas_math_interp_parabolic impl_meas_math_interp_parabolic
#define meas_math_interp_cosine impl_meas_math_interp_cosine
#define meas_math_extrap_linear impl_meas_math_extrap_linear
#define meas_math_is_close impl_meas_math_is_close
#define meas_math_sqrt impl_meas_math_sqrt
#define meas_math_cbrt impl_meas_math_cbrt
#define meas_math_log impl_meas_math_log
#define meas_math_log10 impl_meas_math_log10
#define meas_math_exp impl_meas_math_exp
#define meas_math_atan impl_meas_math_atan
#define meas_math_atan2 impl_meas_math_atan2
#define meas_math_modf impl_meas_math_modf
#define meas_math_sincos impl_meas_math_sincos
#define meas_math_stats impl_meas_math_stats
#define meas_math_rms impl_meas_math_rms
#define meas_math_sma impl_meas_math_sma
#define meas_math_ema impl_meas_math_ema
#define meas_cabs impl_meas_cabs
#define meas_carg impl_meas_carg
#define meas_math_spline_catmull_rom impl_meas_math_spline_catmull_rom

// ----------------------------------------------------------------------------
// 2. Configuration: Enable Embedded Logic
// ----------------------------------------------------------------------------
// Force F303 target logic to test the LUT implementation of fast_sincosf
// instead of the host libc fallback.
#ifndef F303
#define F303
#endif

// ----------------------------------------------------------------------------
// 3. Include Source
// ----------------------------------------------------------------------------
// Include the implementation directly to access static 'fast_*' functions.
// We use a relative path assuming we are in tests/src/utils
#include "../../../src/utils/math.c"

// Undefine to clean up (optional, but polite)
#undef meas_math_interp_linear
// ... (omitting exhaustive undef for brevity in test file)

// ----------------------------------------------------------------------------
// 4. Reference Functions (Standard Lib)
// ----------------------------------------------------------------------------
// We compare against standard float functions
#define REF_SQRT(x) sqrtf(x)
#define REF_CBRT(x) cbrtf(x)
#define REF_LOG(x) logf(x)
#define REF_ATAN2(y, x) atan2f(y, x)
#define REF_SIN(x) sinf(x)
#define REF_COS(x) cosf(x)

// ----------------------------------------------------------------------------
// 5. Test Cases
// ----------------------------------------------------------------------------

void test_fast_atan2_accuracy(void) {
  float max_err = 0.0f;
  float rms_err = 0.0f;
  int steps = 3600;

  for (int i = 0; i < steps; i++) {
    float angle = (float)i * (2.0f * MEAS_PI / steps);
    float y = sinf(angle);
    float x = cosf(angle);

    float ref = REF_ATAN2(y, x);
    float dut = fast_atan2f(y, x);

    // Handle wrap-around at PI/-PI
    float err = fabsf(dut - ref);
    if (err > MEAS_PI)
      err = 2.0f * MEAS_PI - err;

    if (err > max_err)
      max_err = err;
    rms_err += err * err;
  }
  rms_err = sqrtf(rms_err / steps);

  printf("[FAST] Atan2: Max Err=%.6f rad, RMS=%.6f rad\n", max_err, rms_err);
  // Approximation is polynomial, expect some error (~0.005 deg = 0.00008 rad)
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, max_err);
}

void test_fast_sincos_accuracy(void) {
  float max_err_sin = 0.0f;
  float max_err_cos = 0.0f;
  int steps = 3600;

  for (int i = 0; i < steps; i++) {
    float angle = (float)i * (2.0f * MEAS_PI / steps) - MEAS_PI; // -PI to PI
    float ref_s = REF_SIN(angle);
    float ref_c = REF_COS(angle);

    float dut_s, dut_c;
    fast_sincosf(angle, &dut_s, &dut_c);

    float err_s = fabsf(dut_s - ref_s);
    float err_c = fabsf(dut_c - ref_c);

    if (err_s > max_err_sin)
      max_err_sin = err_s;
    if (err_c > max_err_cos)
      max_err_cos = err_c;
  }

  printf("[FAST] SinCos: Max Err Sin=%.6f, Cos=%.6f\n", max_err_sin,
         max_err_cos);
  // LUT interpolation error
  TEST_ASSERT_FLOAT_WITHIN(0.002f, 0.0f, max_err_sin);
  TEST_ASSERT_FLOAT_WITHIN(0.002f, 0.0f, max_err_cos);
}

void test_fast_log_accuracy(void) {
  float max_err = 0.0f;

  // Sweep from 0.1 to 1000
  for (float x = 0.1f; x < 1000.0f; x *= 1.01f) {
    float ref = REF_LOG(x);
    float dut = fast_logf(x);
    float err = fabsf(dut - ref);

    // Relative error might be more appropriate for log
    if (fabsf(ref) > 1e-6) {
      // float rel = err / fabsf(ref);
    }

    if (err > max_err)
      max_err = err;
  }
  printf("[FAST] Log: Max Err=%.6f\n", max_err);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, max_err); // Pretty rough approx
}

void test_fast_cbrt_accuracy(void) {
  float max_err = 0.0f;
  for (float x = 0.0f; x < 1000.0f; x += 0.5f) {
    float ref = REF_CBRT(x);
    float dut = fast_cbrtf(x);
    float err = fabsf(dut - ref);
    if (err > max_err)
      max_err = err;
  }
  printf("[FAST] Cbrt: Max Err=%.6f\n", max_err);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, max_err);
}

#define TEST_ASSERT_INT_WITHIN(delta, expected, actual)                        \
  do {                                                                         \
    int diff = (int)(expected) - (int)(actual);                                \
    if (diff < 0)                                                              \
      diff = -diff;                                                            \
    if (diff > (delta)) {                                                      \
      printf("FAIL: %s:%d: Expected %d, got %d (delta %d > tol %d)\n",         \
             __FILE__, __LINE__, (int)(expected), (int)(actual), diff,         \
             (int)(delta));                                                    \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

void test_spline_catmull_rom(void) {
  // Define 5 control points
  meas_point_t points[] = {
      {0, 0},   // P0
      {20, 20}, // P1 (Start of curve)
      {40, 10}, // P2
      {60, 40}, // P3
      {80, 0}   // P4 (End of curve for P1..P3 segment)
  };

  // We want to interpolate between P1..P3 (and P2..P? depending on segment
  // count logic) count = 5. segments = 5 - 3 = 2. Segment 1: P0, P1, P2, P3 ->
  // Interpolates P1 to P2 Segment 2: P1, P2, P3, P4 -> Interpolates P2 to P3

  // Output buffer for 20 points (10 per segment)
  meas_point_t output[21];
  meas_math_spline_catmull_rom(points, 5, output, 20);

  // Check critical points
  // Index 0 should be P1 approx (t=0 of seg 1)
  TEST_ASSERT_INT_WITHIN(1, 20, output[0].x);
  TEST_ASSERT_INT_WITHIN(1, 20, output[0].y);

  // Index 10 should be P2 approx (t=0 of seg 2 OR t=1 of seg 1)
  TEST_ASSERT_INT_WITHIN(1, 40, output[10].x);
  TEST_ASSERT_INT_WITHIN(1, 10, output[10].y);

  // Index 19 roughly P3 (end of seg 2)
  // The last point logic might clamp to P(N-2) -> P3 {60, 40}
  TEST_ASSERT_INT_WITHIN(1, 60, output[19].x);
  TEST_ASSERT_INT_WITHIN(1, 40, output[19].y);

  printf("[FAST] Spline: Verified points P1(20,20)->P2(40,10)->P3(60,40)\n");
}

// ----------------------------------------------------------------------------
// 6. Performance Benchmarks
// ----------------------------------------------------------------------------

void test_fast_modff_accuracy(void) {
  float max_err_int = 0.0f;
  float max_err_frac = 0.0f;

  for (float x = -100.0f; x < 100.0f; x += 0.123f) {
    float ref_i, dut_i;
    float ref_f = modff(x, &ref_i);
    float dut_f = fast_modff(x, &dut_i);

    float err_i = fabsf(dut_i - ref_i);
    float err_f = fabsf(dut_f - ref_f);

    if (err_i > max_err_int)
      max_err_int = err_i;
    if (err_f > max_err_frac)
      max_err_frac = err_f;
  }
  printf("[FAST] Modf: Max Err Int=%.0f, Frac=%.6f\n", max_err_int,
         max_err_frac);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.0f, max_err_int);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.0f, max_err_frac);
}

// ----------------------------------------------------------------------------
// 6. Performance Benchmarks
// ----------------------------------------------------------------------------

void test_fast_math_perf(void) {
  const int iterations = 1000000;
  volatile float sink; // Prevent optimization
  volatile float iptr;
  clock_t start, end;

  // 1. SINCOS
  start = clock();
  float s, c;
  for (int i = 0; i < iterations; i++) {
    fast_sincosf((float)i * 0.01f, &s, &c);
    sink = s + c;
    (void)sink;
  }
  end = clock();
  double t_sincos = (double)(end - start) / CLOCKS_PER_SEC;

  // 2. ATAN2
  start = clock();
  for (int i = 0; i < iterations; i++) {
    sink = fast_atan2f((float)i, 1000.0f);
    (void)sink;
  }
  end = clock();
  double t_atan2 = (double)(end - start) / CLOCKS_PER_SEC;

  // 3. LOG
  start = clock();
  for (int i = 0; i < iterations; i++) {
    sink = fast_logf((float)i + 1.0f);
    (void)sink;
  }
  end = clock();
  double t_log = (double)(end - start) / CLOCKS_PER_SEC;

  // 4. CBRT
  start = clock();
  for (int i = 0; i < iterations; i++) {
    sink = fast_cbrtf((float)i);
    (void)sink;
  }
  end = clock();
  double t_cbrt = (double)(end - start) / CLOCKS_PER_SEC;

  // 5. MODF
  start = clock();
  for (int i = 0; i < iterations; i++) {
    sink = fast_modff((float)i * 1.5f, (float *)&iptr);
    (void)sink;
  }
  end = clock();
  double t_modf = (double)(end - start) / CLOCKS_PER_SEC;

  printf("[PERF] Fast Sincos: %.2f Mops/s\n", iterations / t_sincos / 1e6);
  printf("[PERF] Fast Atan2:  %.2f Mops/s\n", iterations / t_atan2 / 1e6);
  printf("[PERF] Fast Log:    %.2f Mops/s\n", iterations / t_log / 1e6);
  printf("[PERF] Fast Cbrt:   %.2f Mops/s\n", iterations / t_cbrt / 1e6);
  printf("[PERF] Fast Modf:   %.2f Mops/s\n", iterations / t_modf / 1e6);
}

void run_fast_math_tests(void) {
  printf("--- Running Fast Math Internal Tests (F303 Emulation) ---\n");
  RUN_TEST(test_fast_atan2_accuracy);
  RUN_TEST(test_fast_sincos_accuracy);
  RUN_TEST(test_fast_log_accuracy);
  RUN_TEST(test_fast_modff_accuracy);
  RUN_TEST(test_fast_cbrt_accuracy);
  RUN_TEST(test_spline_catmull_rom);
  RUN_TEST(test_fast_math_perf);
}

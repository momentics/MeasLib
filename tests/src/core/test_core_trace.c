/**
 * @file test_core_trace.c
 * @brief Unit Tests for Core Trace Logic (White-Box).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 */

// White-Box: Include the source directly
#include "../../../src/core/trace.c"
// object.c needed for base object implementation (API wrapper logic)
#include "../../../src/core/object.c"

#include "test_framework.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// --- Mocks ---

static bool get_data_called = false;
static bool copy_data_called = false;

static meas_status_t mock_get_data(meas_trace_t *t, const meas_real_t **x,
                                   const meas_real_t **y, size_t *count) {
  (void)t;
  (void)x;
  (void)y;
  (void)count;
  get_data_called = true;
  return MEAS_OK; // Correct status code
}

static meas_status_t mock_copy_data(meas_trace_t *t, const void *data,
                                    size_t size) {
  (void)t;
  (void)data;
  (void)size;
  copy_data_called = true;
  return MEAS_OK;
}

static const meas_trace_api_t mock_trace_api = {.base = {.get_name = NULL,
                                                         .set_prop = NULL,
                                                         .get_prop = NULL,
                                                         .destroy = NULL},
                                                .get_data = mock_get_data,
                                                .copy_data = mock_copy_data};

// --- Tests ---

void test_trace_delegation(void) {
  get_data_called = false;
  copy_data_called = false;

  meas_trace_t trace;
  trace.base.api = (const meas_object_api_t *)&mock_trace_api;
  trace.base.ref_count = 1; // Basic init

  // 2. Test get_data delegation
  const meas_real_t *x = NULL;
  const meas_real_t *y = NULL;
  size_t count = 0;

  meas_status_t status = meas_trace_get_data(&trace, &x, &y, &count);
  TEST_ASSERT_EQUAL(MEAS_OK, status);
  TEST_ASSERT_EQUAL(1, get_data_called);

  // 3. Test copy_data delegation
  status = meas_trace_copy_data(&trace, NULL, 0);
  TEST_ASSERT_EQUAL(MEAS_OK, status);
  TEST_ASSERT_EQUAL(1, copy_data_called);
}

void run_core_trace_tests(void) {
  printf("--- Running Core Trace Tests ---\n");
  RUN_TEST(test_trace_delegation);
}

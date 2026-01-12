/**
 * @file test_core_object.c
 * @brief Unit Tests for Core Object Model (White-Box).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/core/object.h"
#include "test_framework.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// --- Mocks for Polymorphism Test ---

static bool destroy_called = false;
static const char *mock_name = "MockObject";

// Signature match: VTable expects non-const obj for get_name in object.h
static const char *mock_get_name(meas_object_t *obj) {
  (void)obj;
  return mock_name;
}

// Signature match: destroy returns void in object.h
static void mock_destroy(meas_object_t *obj) {
  (void)obj;
  destroy_called = true;
}

static const meas_object_api_t mock_api = {
    .get_name = mock_get_name,
    .destroy = mock_destroy,
    .get_prop = NULL, // Optional
    .set_prop = NULL  // Optional
};

// --- Tests ---

void test_object_lifecycle(void) {
  destroy_called = false;

  // 1. Create Object manually
  meas_object_t obj;
  memset(&obj, 0, sizeof(obj));
  obj.api = &mock_api;
  obj.ref_count = 1; // Initial ref

  // 2. Ref
  meas_object_ref(&obj);
  TEST_ASSERT_EQUAL(2, obj.ref_count);

  // 3. Unref (should not destroy yet)
  meas_object_unref(&obj);
  TEST_ASSERT_EQUAL(1, obj.ref_count);
  TEST_ASSERT_EQUAL(0, destroy_called);

  // 4. Unref (should destroy)
  meas_object_unref(&obj);
  TEST_ASSERT_EQUAL(1, destroy_called);
}

void test_object_polymorphism(void) {
  meas_object_t obj;
  memset(&obj, 0, sizeof(obj));
  obj.api = &mock_api;

  // Test VTable dispatch via wrapper
  const char *name = meas_object_get_name(&obj);
  TEST_ASSERT_EQUAL_STRING(mock_name, name);
}

void run_core_object_tests(void) {
  printf("--- Running Core Object Tests ---\n");
  RUN_TEST(test_object_lifecycle);
  RUN_TEST(test_object_polymorphism);
}

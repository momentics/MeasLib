/**
 * @file test_events.c
 * @brief Unit Tests for Event Subsystem.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Verifies event publishing, subscription, and queue overflow handling.
 */

#include "measlib/core/event.h"
#include "test_framework.h"

static int callback_count = 0;
static meas_event_t last_event;

static void test_handler(const meas_event_t *e, void *ctx) {
  (void)ctx;
  callback_count++;
  last_event = *e;
}

void test_event_pub_sub(void) {
  callback_count = 0;

  // 1. Subscribe (NULL = All sources)
  meas_status_t status = meas_subscribe(NULL, test_handler, NULL);
  TEST_ASSERT_EQUAL(MEAS_OK, status);

  // 2. Publish
  meas_event_t evt = {.type = EVENT_DATA_READY, .payload.i_val = 123};
  status = meas_event_publish(evt);
  TEST_ASSERT_EQUAL(MEAS_OK, status);

  // 3. Dispatch (Simulate Main Loop)
  meas_dispatch_events();

  // 4. Verify
  TEST_ASSERT_EQUAL(1, callback_count);
  TEST_ASSERT_EQUAL(EVENT_DATA_READY, last_event.type);
  TEST_ASSERT_EQUAL(123, last_event.payload.i_val);
}

void test_event_queue_overflow(void) {
  // Queue size is 16, so it holds 15 items (one slot for head/tail distinction)

  meas_event_t evt = {.type = EVENT_DATA_READY};
  meas_status_t status = MEAS_OK;

  // Fill up to 15
  for (int i = 0; i < 15; i++) {
    status = meas_event_publish(evt);
    if (status != MEAS_OK)
      break;
  }
  TEST_ASSERT_EQUAL(MEAS_OK, status); // Should fit 15

  // Try 16th - Should Fail
  status = meas_event_publish(evt);
  TEST_ASSERT_EQUAL(MEAS_BUSY, status);

  // Clear queue
  meas_dispatch_events();
}

void run_event_tests(void) {
  printf("--- Running Event Tests ---\n");
  RUN_TEST(test_event_pub_sub);
  RUN_TEST(test_event_queue_overflow);
}

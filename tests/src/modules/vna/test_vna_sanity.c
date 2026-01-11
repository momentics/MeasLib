/**
 * @file test_vna_sanity.c
 * @brief Sanity Check for VNA Channel FSM and DSP Chain.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Verifies the initialization, state transitions, and pipeline configuration
 * of the VNA module using Mock drivers.
 */

#include "measlib/core/event.h"
#include "measlib/drivers/hal.h"
#include "measlib/modules/vna/channel.h"
#include "test_framework.h"
#include <string.h>

// -- Mock Context --
static meas_vna_channel_t vna_ch;
static meas_trace_t mock_trace;

extern const meas_hal_synth_api_t mock_synth_api;
extern const meas_hal_rx_api_t mock_rx_api;

// -- Test Verification --

void test_vna_init_state(void) {
  // 1. Init Trace
  memset(&mock_trace, 0, sizeof(mock_trace));
  mock_trace.base.api = NULL; // No trace API needed for this test

  // 2. Init Channel
  memset(&vna_ch, 0, sizeof(vna_ch));
  meas_status_t status = meas_vna_channel_init(&vna_ch, &mock_trace);
  TEST_ASSERT_EQUAL(MEAS_OK, status);

  // 3. Inject Mock Drivers
  vna_ch.hal_synth = (void *)&mock_synth_api;
  vna_ch.hal_rx = (void *)&mock_rx_api;

  // 4. Verify Defaults
  TEST_ASSERT_EQUAL(VNA_MAX_POINTS, vna_ch.points);
  TEST_ASSERT_EQUAL(VNA_MIN_FREQ, vna_ch.start_freq_hz);
  TEST_ASSERT_EQUAL(VNA_CH_STATE_IDLE, vna_ch.state);
}

void test_vna_start_sweep(void) {
  // Configure valid sweep params
  vna_ch.start_freq_hz = 1000000;
  vna_ch.stop_freq_hz = 10000000;
  vna_ch.points = 101;
  vna_ch.user_buffer = NULL; // Simulate implicit buffer

  // Call Configure (applies defaults and creates pipeline)
  meas_status_t status = vna_ch.base.base.api->set_prop(
      (meas_object_t *)&vna_ch, MEAS_PROP_VNA_START_FREQ,
      (meas_variant_t){.type = PROP_TYPE_INT64, .i_val = 1000000});
  TEST_ASSERT_EQUAL(MEAS_OK, status);

  // Explicitly call configure via API
  const meas_channel_api_t *api =
      (const meas_channel_api_t *)vna_ch.base.base.api;
  status = api->configure(&vna_ch.base);
  TEST_ASSERT_EQUAL(MEAS_OK, status);

  // Start Sweep
  status = api->start_sweep(&vna_ch.base);
  TEST_ASSERT_EQUAL(MEAS_OK, status);

  // Verify State Transition: IDLE -> SETUP
  TEST_ASSERT_EQUAL(VNA_CH_STATE_SETUP, vna_ch.state);

  // Run Tick: SETUP -> ACQUIRE
  api->tick(&vna_ch.base); // Should set freq and move to ACQUIRE (assuming
                           // immediate PLL lock in mock)
  // Note: Our Mock HAL is synchronous, so it might need multiple ticks if we
  // implemented wait states. In vna/channel.c, SETUP sets freq then moves to
  // ACQUIRE immediately in current implementation? Let's check source: case
  // VNA_CH_STATE_SETUP:
  //   synth->set_freq(...)
  //   ch->state = VNA_CH_STATE_ACQUIRE;

  // So one tick should move it.
  TEST_ASSERT_EQUAL(VNA_CH_STATE_ACQUIRE, vna_ch.state);

  // Run Tick: ACQUIRE -> WAIT_DMA
  api->tick(&vna_ch.base);
  // Source:
  // case VNA_CH_STATE_ACQUIRE:
  //   rx->start(...)
  //   ch->state = VNA_CH_STATE_WAIT_DMA;
  TEST_ASSERT_EQUAL(VNA_CH_STATE_WAIT_DMA, vna_ch.state);
}

// Global entry point for this test suite
// Global entry point for this test suite
void run_vna_sanity_tests(void) {
  printf("--- Running VNA Tests ---\n");
  RUN_TEST(test_vna_init_state);
  RUN_TEST(test_vna_start_sweep);
}

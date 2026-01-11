/**
 * @file mock_hal.c
 * @brief Mock Implementation of Hardware Abstraction Layer for Host Testing.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * This file provides stub implementations for the HAL interfaces (Synth, RX)
 * allowing the firmware logic to compile and run on a host machine (Linux)
 * without physical hardware.
 */

#include "measlib/drivers/hal.h"
#include "measlib/types.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// -- Mock State --
static meas_real_t mock_synth_freq = 0.0;
static bool mock_synth_enabled = false;

// -- Synth Driver --

static meas_status_t mock_synth_set_freq(void *ctx, meas_real_t hz) {
  (void)ctx;
  mock_synth_freq = hz;
  printf("[MOCK] Synth Set Freq: %.2f Hz\n", hz);
  return MEAS_OK;
}

static meas_status_t mock_synth_set_power(void *ctx, meas_real_t dbm) {
  (void)ctx;
  printf("[MOCK] Synth Set Power: %.2f dBm\n", dbm);
  return MEAS_OK;
}

static meas_status_t mock_synth_enable(void *ctx, bool enable) {
  (void)ctx;
  mock_synth_enabled = enable;
  printf("[MOCK] Synth Enable: %d\n", enable);
  return MEAS_OK;
}

const meas_hal_synth_api_t mock_synth_api = {.set_freq = mock_synth_set_freq,
                                             .set_power = mock_synth_set_power,
                                             .enable_output =
                                                 mock_synth_enable};

// -- RX Driver --

static meas_status_t mock_rx_configure(void *ctx, meas_real_t sample_rate,
                                       int decimation) {
  (void)ctx;
  printf("[MOCK] RX Configure: SR=%.2f, Dec=%d\n", sample_rate, decimation);
  return MEAS_OK;
}

static meas_status_t mock_rx_start(void *ctx, void *buffer, size_t size) {
  (void)ctx;
  printf("[MOCK] RX Start DMA: Buffer=%p, Size=%zu\n", buffer, size);
  // In a real test, we might trigger a timer to simulate data ready later.
  return MEAS_OK;
}

static meas_status_t mock_rx_stop(void *ctx) {
  (void)ctx;
  printf("[MOCK] RX Stop\n");
  return MEAS_OK;
}

const meas_hal_rx_api_t mock_rx_api = {.configure = mock_rx_configure,
                                       .start = mock_rx_start,
                                       .stop = mock_rx_stop};

// -- Board Init --
// Mocks the "board.c" functionality

void sys_init(void) { printf("[MOCK] System Initialized (Host)\n"); }

void sys_enter_critical(void) {
  // No-op on single-threaded host test
}

void sys_exit_critical(uint32_t status) { (void)status; }

void sys_wait_for_interrupt(void) {
  // Return immediately to keep test running
}

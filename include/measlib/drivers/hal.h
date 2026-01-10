/**
 * @file hal.h
 * @brief Hardware Abstraction Layer (Internal Drivers).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the low-level interfaces for hardware components (Synthesizers,
 * Switches, ADCs) that are composed to build a full Instrument Driver. These
 * are not exposed to the App logic.
 */

#ifndef MEASLIB_DRIVERS_HAL_H
#define MEASLIB_DRIVERS_HAL_H

#include "measlib/types.h"

/**
 * @brief Frequency Synthesizer Interface
 * Example: Si5351, ADF4350.
 */
typedef struct {
  meas_status_t (*set_freq)(void *ctx, meas_real_t hz);
  meas_status_t (*set_power)(void *ctx, meas_real_t dbm);
  meas_status_t (*enable_output)(void *ctx, bool enable);
} meas_hal_synth_api_t;

/**
 * @brief Receiver / ADC Interface
 * Example: TLV320, Internal ADC, SAI.
 */
typedef struct {
  meas_status_t (*configure)(void *ctx, meas_real_t sample_rate,
                             int decimation);
  meas_status_t (*start)(void *ctx, void *buffer, size_t size); // Start DMA
  meas_status_t (*stop)(void *ctx);
} meas_hal_rx_api_t;

/**
 * @brief Front-End Switch Interface
 * Controls RF path switching (e.g. S11/S21 relays).
 */
typedef struct {
  meas_status_t (*set_path)(void *ctx, int path_id);
} meas_hal_fe_api_t;

#endif // MEASLIB_DRIVERS_HAL_H

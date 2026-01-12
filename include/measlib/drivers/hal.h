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

/**
 * @brief IO / Controls Interface
 * LED, Buttons, etc.
 */
typedef struct {
  meas_status_t (*set_led)(void *ctx, bool on);
  uint32_t (*read_buttons)(void *ctx); // Returns bitmask
} meas_hal_io_api_t;

/**
 * @brief Touch Screen Interface (Resistive/Capacitive)
 */
typedef struct {
  // Returns raw point (adc values) or calibrated if driver handles it
  meas_status_t (*read_point)(void *ctx, int16_t *x, int16_t *y);
} meas_hal_touch_api_t;

/**
 * @brief Watchdog Interface
 */
typedef struct {
  meas_status_t (*start)(void *ctx, uint32_t timeout_ms);
  meas_status_t (*kick)(void *ctx);
} meas_hal_wdg_api_t;

/**
 * @brief Flash Memory Interface
 * For storing settings/calibrations.
 */
typedef struct {
  meas_status_t (*unlock)(void *ctx);
  meas_status_t (*lock)(void *ctx);
  meas_status_t (*erase_page)(void *ctx, uint32_t address);
  meas_status_t (*program)(void *ctx, uint32_t address, const void *data,
                           size_t len);
} meas_hal_flash_api_t;

/**
 * @brief Communication Link Interface (USB CDC / UART)
 */
typedef struct {
  meas_status_t (*send)(void *ctx, const void *data, size_t len);
  meas_status_t (*recv)(void *ctx, void *data, size_t len, size_t *read);
  bool (*is_connected)(void *ctx);
} meas_hal_link_api_t;

/**
 * @brief Storage (Block Device) Interface
 * For SD Card, Flash, etc.
 */
typedef struct {
  meas_status_t (*read)(void *ctx, uint32_t sector, void *buffer,
                        uint32_t count);
  meas_status_t (*write)(void *ctx, uint32_t sector, const void *buffer,
                         uint32_t count);
  uint32_t (*get_capacity)(void *ctx); // Returns sector count
  bool (*is_ready)(void *ctx);
} meas_hal_storage_api_t;

/**
 * @brief Display Driver Interface
 * Low-level window/pixel access.
 */
typedef struct {
  meas_status_t (*set_window)(void *ctx, uint16_t x, uint16_t y, uint16_t w,
                              uint16_t h);
  meas_status_t (*fill_rect)(void *ctx, uint16_t x, uint16_t y, uint16_t w,
                             uint16_t h, uint16_t color);
  meas_status_t (*blit)(void *ctx, uint16_t x, uint16_t y, uint16_t w,
                        uint16_t h, const void *pixels);
  meas_status_t (*set_orientation)(void *ctx, uint8_t rotation, bool bgr);
  uint16_t (*get_width)(void *ctx);
  uint16_t (*get_height)(void *ctx);
} meas_hal_display_api_t;

#endif // MEASLIB_DRIVERS_HAL_H

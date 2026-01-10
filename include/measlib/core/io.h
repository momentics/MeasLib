/**
 * @file io.h
 * @brief Communication Stream Interface.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Abstracts byte-oriented IO peripherals (USB CDC, UART, BLE) into a consistent
 * stream API. Handles zero-copy transmission and input status buffering.
 */

#ifndef MEASLIB_CORE_IO_H
#define MEASLIB_CORE_IO_H

#include "measlib/core/object.h"

/**
 * @brief IO Stream Handle
 */
typedef struct {
  meas_object_t base;
} meas_io_t;

/**
 * @brief IO Stream API (VTable)
 */
typedef struct {
  meas_object_api_t base;

  /**
   * @brief Send data (Zero-Copy).
   * Queues the buffer for transmission (e.g. via DMA).
   * @param io IO Stream instance.
   * @param data Pointer to data buffer.
   * @param size Size of data to send.
   * @return meas_status_t
   */
  meas_status_t (*send)(meas_io_t *io, const void *data, size_t size);

  /**
   * @brief Get counts of available bytes in input buffer.
   * @param io IO Stream instance.
   * @return size_t Count of bytes ready to read.
   */
  size_t (*get_available)(meas_io_t *io);

  /**
   * @brief Check connection status.
   * @param io IO Stream instance.
   * @return bool True if connected (e.g. USB DTR active).
   */
  bool (*is_connected)(meas_io_t *io);

  /**
   * @brief Read data from input buffer.
   * @param io IO Stream instance.
   * @param buffer Destination buffer.
   * @param size Maximum bytes to read.
   * @param read_count Pointer to store actual bytes read.
   * @return meas_status_t
   */
  meas_status_t (*receive)(meas_io_t *io, void *buffer, size_t size,
                           size_t *read_count);

  /**
   * @brief Flush output buffers.
   * Blocks until all queued data is transmitted.
   * @param io IO Stream instance.
   * @return meas_status_t
   */
  meas_status_t (*flush)(meas_io_t *io);

} meas_io_api_t;

#endif // MEASLIB_CORE_IO_H

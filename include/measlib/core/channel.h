/**
 * @file channel.h
 * @brief Measurement Channel Interface.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the `meas_channel_t` which represents a logical measurement unit
 * (e.g. S11 Sweep). It coordinates the timing and configuration of underlying
 * hardware drivers.
 */

#ifndef MEASLIB_CORE_CHANNEL_H
#define MEASLIB_CORE_CHANNEL_H

#include "measlib/core/object.h"

/**
 * @brief Channel Handle
 * Represents a specific measurement configuration and sequence.
 */
struct meas_channel_s {
  meas_object_t base; /**< Inherits from generic object */
};

// Typedef for consistency
typedef struct meas_channel_s meas_channel_t;

/**
 * @brief Channel API (VTable)
 */
typedef struct {
  meas_object_api_t base;

  /**
   * @brief Configure the channel hardware.
   * Applies all set properties to the underlying drivers.
   * @param ch Channel instance.
   * @return meas_status_t
   */
  meas_status_t (*configure)(meas_channel_t *ch);

  /**
   * @brief Start the measurement sweep/acquisition.
   * @param ch Channel instance.
   * @return meas_status_t
   */
  meas_status_t (*start_sweep)(meas_channel_t *ch);

  /**
   * @brief Abort the current operation.
   * @param ch Channel instance.
   * @return meas_status_t
   */
  meas_status_t (*abort_sweep)(meas_channel_t *ch);

  /**
   * @brief Periodic Update (FSM Step).
   * Must be non-blocking and return quickly (< 100us).
   * @param ch Channel instance.
   */
  void (*tick)(meas_channel_t *ch);

} meas_channel_api_t;

#endif // MEASLIB_CORE_CHANNEL_H

/**
 * @file device.h
 * @brief Instrument Facade Interface.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the `meas_device_t` interface, which serves as the primary entry
 * point for the application to control hardware. It acts as a factory for
 * Channels.
 */

#ifndef MEASLIB_CORE_DEVICE_H
#define MEASLIB_CORE_DEVICE_H

#include "measlib/core/object.h"

// Forward declaration of Channel to resolve dependency
typedef struct meas_channel_s meas_channel_t;

/**
 * @brief Device Information Structure
 * Returned by `get_info` to identify the connected hardware.
 */
typedef struct {
  char name[32];         /**< Device Name (e.g. "NanoVNA-H4") */
  uint32_t version;      /**< Firmware/Hardware Version */
  uint32_t capabilities; /**< Bitmask of supported features */
} meas_device_info_t;

/**
 * @brief Device Handle
 * Opaque handle representing the physical instrument.
 */
typedef struct {
  meas_object_t base; /**< Inherits from generic object */
} meas_device_t;

/**
 * @brief Device API (VTable)
 */
typedef struct {
  meas_object_api_t base;

  /**
   * @brief Open access to the hardware resource.
   * @param dev Device instance.
   * @param resource_id Hardware identifier string (e.g. "SPI1").
   * @return meas_status_t
   */
  meas_status_t (*open)(meas_device_t *dev, const char *resource_id);

  /**
   * @brief Close access and release resources.
   * @param dev Device instance.
   * @return meas_status_t
   */
  meas_status_t (*close)(meas_device_t *dev);

  /**
   * @brief Reset the device to a known initial state.
   * @param dev Device instance.
   * @return meas_status_t
   */
  meas_status_t (*reset)(meas_device_t *dev);

  /**
   * @brief Retrieve device identification info.
   * @param dev Device instance.
   * @param info Output pointer for info structure.
   * @return meas_status_t
   */
  meas_status_t (*get_info)(meas_device_t *dev, meas_device_info_t *info);

  /**
   * @brief Create a measurement channel.
   * @param dev Device instance.
   * @param ch_id Identifier for the new channel.
   * @param out_ch Pointer to store the created channel handle.
   * @return meas_status_t
   */
  meas_status_t (*create_channel)(meas_device_t *dev, meas_id_t ch_id,
                                  meas_channel_t **out_ch);

} meas_device_api_t;

#endif // MEASLIB_CORE_DEVICE_H

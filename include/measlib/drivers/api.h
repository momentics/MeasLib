/**
 * @file api.h
 * @brief Driver Registration API.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Mechanisms for registering hardware drivers at runtime or link-time.
 */

#ifndef MEASLIB_DRIVERS_API_H
#define MEASLIB_DRIVERS_API_H

#include "measlib/types.h"

/**
 * @brief Driver Descriptor
 * Metadata and methods for initializing a driver.
 */
typedef struct {
  const char *name; /**< Unique driver name */
  meas_status_t (*init)(
      void); /**< Initialization function (called at startup) */
  meas_status_t (*probe)(
      void); /**< Probe function to check hardware presence */
} meas_driver_desc_t;

/**
 * @brief Register a driver with the system.
 * @param desc Pointer to driver descriptor.
 * @return meas_status_t
 */
meas_status_t meas_driver_register(const meas_driver_desc_t *desc);

/**
 * @brief Enter Critical Section
 * Disables interrupts to protect atomic operations.
 *
 * @return uint32_t Opaque state value (e.g. PRIMASK) to be passed to
 * sys_exit_critical.
 */
uint32_t sys_enter_critical(void);

/**
 * @brief Exit Critical Section
 * Restores interrupt state.
 *
 * @param state State value returned by sys_enter_critical.
 */
void sys_exit_critical(uint32_t state);

/**
 * @brief System Initialization
 * Platform-specific hardware setup (Clocks, SysTick, HAL).
 */
void sys_init(void);

#endif // MEASLIB_DRIVERS_API_H

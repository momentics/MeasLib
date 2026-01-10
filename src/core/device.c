/**
 * @file device.c
 * @brief Device Facade Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/core/device.h"

// Global Tick Wrapper (called from main loop)
void meas_device_tick(meas_device_t *dev) {
  // This function orchestrates background tasks for the device.
  // If the device has a specific 'tick' method in its derived interface or
  // impl, call it. For now, we assume it manages internal state machines.
  if (!dev)
    return;

  // Implementation specific dispatch...
  // ((meas_device_impl_t*)dev->impl)->tick(dev);
}

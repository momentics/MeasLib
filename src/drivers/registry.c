/**
 * @file registry.c
 * @brief Driver Registry Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/drivers/api.h"
#include <string.h>

#define MAX_DRIVERS 16

static const meas_driver_desc_t *drivers[MAX_DRIVERS];
static int driver_count = 0;

meas_status_t meas_driver_register(const meas_driver_desc_t *desc) {
  if (driver_count >= MAX_DRIVERS)
    return MEAS_ERROR;
  if (!desc)
    return MEAS_ERROR;

  drivers[driver_count++] = desc;
  return MEAS_OK;
}

// Internal: System Init calls this
void sys_init_drivers(void) {
  for (int i = 0; i < driver_count; i++) {
    if (drivers[i]->init) {
      drivers[i]->init();
    }
  }
}

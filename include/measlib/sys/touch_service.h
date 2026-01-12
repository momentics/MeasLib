/**
 * @file touch_service.h
 * @brief Touch Input Service API.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEAS_SYS_TOUCH_SERVICE_H
#define MEAS_SYS_TOUCH_SERVICE_H

#include "measlib/drivers/hal.h"

/**
 * @brief Initialize the Touch Input Service.
 * @param api Pointer to the Touch driver API VTable.
 * @param ctx Pointer to the driver context.
 */
void meas_touch_service_init(const meas_hal_touch_api_t *api, void *ctx);

/**
 * @brief Poll the touch panel.
 * Dispatches EVENT_INPUT_TOUCH if pressed.
 */
void meas_touch_service_poll(void);

#endif // MEAS_SYS_TOUCH_SERVICE_H

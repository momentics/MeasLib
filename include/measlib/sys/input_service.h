/**
 * @file input_service.h
 * @brief Input Service API (GPIO Buttons).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEAS_SYS_INPUT_SERVICE_H
#define MEAS_SYS_INPUT_SERVICE_H

#include "measlib/drivers/hal.h"

/**
 * @brief Initialize the Input Service.
 * @param io Pointer to the IO HAL API.
 * @param ctx Driver context (optional).
 */
void meas_input_service_init(const meas_hal_io_api_t *io, void *ctx);

/**
 * @brief Poll for input events.
 * Should be called periodically from the main loop.
 */
void meas_input_service_poll(void);

#endif // MEAS_SYS_INPUT_SERVICE_H

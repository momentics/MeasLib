/**
 * @file render_service.h
 * @brief Render Service API (HAL Bridge).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEAS_SYS_RENDER_SERVICE_H
#define MEAS_SYS_RENDER_SERVICE_H

#include "measlib/ui/core.h"

/**
 * @brief Initialize the Render Service.
 * Binds the LCD HAL to the UI Rendering Engine and initializes the display.
 * @return meas_ui_t* Pointer to the initialized UI instance (for Main Loop).
 */
meas_ui_t *meas_render_service_init(void);

/**
 * @brief Poll/Update the Render Service.
 * Draws only dirty tiles using the dirty bitmap.
 */
void meas_render_service_update(void);

#endif // MEAS_SYS_RENDER_SERVICE_H

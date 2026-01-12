/**
 * @file drv_touch.h
 * @brief Resistive Touch Driver API
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Provides the public interface for the resistive touch screen driver.
 */

#ifndef __DRV_TOUCH_H__
#define __DRV_TOUCH_H__

#include "measlib/drivers/hal.h"
#include <stdbool.h>
#include <stdint.h>

// --- Public API ---

/**
 * @brief Initialize the resistive touch driver.
 * Configures GPIOs and ADC2 for touch sensing.
 * @return void* Pointer to the// No change needed if it only exposes get_api or
individual getters which I kept.
// But checking is safer. NULL on failure).
 */
void *meas_drv_touch_init(void);

/**
 * @brief Get the generic HAL Touch API.
 * @return const meas_hal_touch_api_t*
 */
const meas_hal_touch_api_t *meas_drv_touch_get_api(void);

// --- Direct Driver API (Optional, for advanced usage) ---

/**
 * @brief Check if the screen is currently touched.
 * @param ctx Driver Context (can be NULL)
 * @return true if touched, false otherwise
 */
bool meas_drv_touch_is_pressed(void *ctx);

/**
 * @brief Get the raw X coordinate.
 * @param ctx Driver Context (can be NULL)
 * @return Raw X ADC value (0-4095)
 */
uint16_t meas_drv_touch_get_x(void *ctx);

/**
 * @brief Get the raw Y coordinate.
 * @param ctx Driver Context (can be NULL)
 * @return Raw Y ADC value (0-4095)
 */
uint16_t meas_drv_touch_get_y(void *ctx);

#endif // __DRV_TOUCH_H__

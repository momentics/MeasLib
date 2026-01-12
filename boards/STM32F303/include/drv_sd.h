/**
 * @file drv_sd.h
 * @brief STM32F303 SD Card Driver (SPI Mode).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements a low-level Block Device driver for SD Cards using SPI.
 * Handles the initialization, block reading, and block writing.
 * Does not include a Filesystem.
 */

#ifndef MEASLIB_DRIVERS_STM32F303_DRV_SD_H
#define MEASLIB_DRIVERS_STM32F303_DRV_SD_H

#include "measlib/drivers/hal.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- Definitions ---

/// Status codes for SD operations
typedef enum {
  MEAS_SD_OK = 0,      ///< Success
  MEAS_SD_NO_INIT,     ///< Driver not initialized or Card not inserted
  MEAS_SD_ERROR,       ///< Generic Error
  MEAS_SD_TIMEOUT,     ///< Timeout occurred
  MEAS_SD_CRC_ERROR,   ///< CRC Check failed
  MEAS_SD_WRITE_ERROR, ///< Write protection or internal error
} meas_sd_status_t;

// --- API ---

/**
 * @brief Initialize the SD Card Driver (SPI based).
 *
 * Configures GPIO and SPI, then attempts to initialize the SD Card.
 * @return Pointer to the Storage Driver Context, or NULL if failed.
 */
void *meas_drv_sd_init(void);

/**
 * @brief Get the Storage HAL API VTable.
 * @return Pointer to the API VTable
 */
const meas_hal_storage_api_t *meas_drv_sd_get_api(void);

/**
 * @brief Read one or more 512-byte blocks from the SD Card.
 *
 * @param ctx Driver context.
 * @param sector Start sector index (LBA).
 * @param buffer Pointer to the destination buffer.
 * @param count Number of blocks to read.
 * @return MEAS_SD_OK on success.
 */
meas_sd_status_t meas_drv_sd_read_blocks(void *ctx, uint32_t sector,
                                         uint8_t *buffer, uint32_t count);

/**
 * @brief Write one or more 512-byte blocks to the SD Card.
 *
 * @param ctx Driver context.
 * @param sector Start sector index (LBA).
 * @param buffer Pointer to the source data.
 * @param count Number of blocks to write.
 * @return MEAS_SD_OK on success.
 */
meas_sd_status_t meas_drv_sd_write_blocks(void *ctx, uint32_t sector,
                                          const uint8_t *buffer,
                                          uint32_t count);

/**
 * @brief Get the total number of sectors on the card.
 *
 * @return Total sector count, or 0 if not initialized.
 */
uint32_t meas_drv_sd_get_sector_count(void);

/**
 * @brief Check if the driver is initialized and card is ready.
 *
 * @return true if initialized, false otherwise.
 */
int meas_drv_sd_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif // MEASLIB_DRIVERS_STM32F303_DRV_SD_H

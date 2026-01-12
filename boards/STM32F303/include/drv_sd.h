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
 * @brief Initialize the SD Card driver and the card itself.
 *
 * Performs the SPI initialization and the SD Card initialization sequence:
 * CMD0 (Reset), CMD8 (Check Voltage), ACMD41 (Init), CMD58 (OCR/Capacity).
 * Switches SPI to High Speed upon success.
 *
 * @return MEAS_SD_OK on success, error code otherwise.
 */
meas_sd_status_t meas_drv_sd_init(void);

/**
 * @brief Read one or more 512-byte blocks from the SD Card.
 *
 * @param sector Start sector index (LBA).
 * @param buffer Pointer to the destination buffer.
 * @param count Number of blocks to read.
 * @return MEAS_SD_OK on success.
 */
meas_sd_status_t meas_drv_sd_read_blocks(uint32_t sector, uint8_t *buffer,
                                         uint32_t count);

/**
 * @brief Write one or more 512-byte blocks to the SD Card.
 *
 * @param sector Start sector index (LBA).
 * @param buffer Pointer to the source data.
 * @param count Number of blocks to write.
 * @return MEAS_SD_OK on success.
 */
meas_sd_status_t meas_drv_sd_write_blocks(uint32_t sector,
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

/**
 * @file data.h
 * @brief Generic Data Block (Zero-Copy).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Defines the high-throughput data structure used for transferring large
 * buffers (e.g. ADC samples) between drivers and the processing chain without
 * copying.
 */

#ifndef MEASLIB_CORE_DATA_H
#define MEASLIB_CORE_DATA_H

#include "measlib/types.h"

/**
 * @brief Data Block Structure
 * Represents a chunk of data (usually DMA-backed) flowing through the system.
 */
typedef struct {
  meas_id_t source_id; /**< ID of the source (e.g. Channel ID or Driver ID) */
  uint32_t sequence;   /**< Sequence number for packet tracking */
  size_t size;         /**< Size of the data payload in bytes */
  void *data;          /**< Pointer to the payload (DMA buffer or Shared Mem) */
} meas_data_block_t;

#endif // MEASLIB_CORE_DATA_H

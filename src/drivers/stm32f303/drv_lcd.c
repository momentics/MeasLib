/**
 * @file drv_lcd.c
 * @brief STM32F303 LCD Driver Stub.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements the Display Interface (Stub).
 */

#include "measlib/drivers/hal.h"
#include <stddef.h>

// Placeholders used to satisfy linker or init calls if any exist.
// Currently NO public API defined for LCD in hal.h yet.
// We just provide the init function for board.c

void *meas_drv_lcd_init(void) {
  // Stub Init
  // TODO: Init SPI, GPIOs (PB6/PB7/PA15 etc)
  return NULL;
}

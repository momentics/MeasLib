/**
 * @file drv_i2c.h
 * @brief STM32F303 Soft I2C Shared Driver (PB8/PB9).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#ifndef MEAS_DRV_I2C_H
#define MEAS_DRV_I2C_H

#include "measlib/types.h"
#include <stdbool.h>
#include <stdint.h>

// Initialize GPIOs for I2C (PB8/PB9)
void meas_drv_i2c_init(void);

// Write multiple bytes to I2C device
// addr: 7-bit address (will be shifted left by 1 internally)
meas_status_t meas_drv_i2c_write(uint8_t addr, const uint8_t *data, size_t len);

// Write list of Register-Value pairs (commonly used for Codec/Synth init)
// Pairs should be [Reg, Val, Reg, Val...]
meas_status_t meas_drv_i2c_write_regs(uint8_t addr,
                                      const uint8_t *reg_val_pairs,
                                      size_t count);

// Read multiple bytes from I2C device
meas_status_t meas_drv_i2c_read(uint8_t addr, uint8_t *data, size_t len);

#endif // MEAS_DRV_I2C_H

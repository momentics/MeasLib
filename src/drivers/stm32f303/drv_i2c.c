/**
 * @file drv_i2c.c
 * @brief STM32F303 Soft I2C Implementation (PB8/PB9).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "drv_i2c.h"
#include <stddef.h>

// --- Hardware Definitions ---

typedef struct {
  volatile uint32_t MODER;
  volatile uint32_t OTYPER;
  volatile uint32_t OSPEEDR;
  volatile uint32_t PUPDR;
  volatile uint32_t IDR;
  volatile uint32_t ODR;
  volatile uint32_t BSRR;
  volatile uint32_t LCKR;
  volatile uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct {
  volatile uint32_t AHBENR;
} RCC_TypeDef;

#define RCC_BASE 0x40021000UL
#define GPIOB_BASE 0x48000400UL
#define RCC ((RCC_TypeDef *)RCC_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)

#define RCC_AHBENR_GPIOBEN (1UL << 18)

#define SCL_PIN 8
#define SDA_PIN 9

// --- Helper Functions ---

static void delay_short(void) {
  for (volatile int i = 0; i < 10; i++)
    ;
}

static void sda_high(void) { GPIOB->BSRR = (1UL << SDA_PIN); }
static void sda_low(void) { GPIOB->BSRR = (1UL << (SDA_PIN + 16)); }
static void scl_high(void) { GPIOB->BSRR = (1UL << SCL_PIN); }
static void scl_low(void) { GPIOB->BSRR = (1UL << (SCL_PIN + 16)); }

static bool sda_read(void) { return (GPIOB->IDR & (1UL << SDA_PIN)) != 0; }

static void i2c_start(void) {
  sda_high();
  scl_high();
  delay_short();
  sda_low();
  delay_short();
  scl_low();
  delay_short();
}

static void i2c_stop(void) {
  sda_low();
  scl_high();
  delay_short();
  sda_high();
  delay_short();
}

static bool i2c_write_byte(uint8_t byte) {
  for (int i = 0; i < 8; i++) {
    if (byte & 0x80)
      sda_high();
    else
      sda_low();
    byte <<= 1;
    delay_short();
    scl_high();
    delay_short();
    scl_low();
    delay_short();
  }

  // ACK/NACK
  sda_high(); // Release SDA
  delay_short();
  scl_high();
  delay_short();
  bool ack = !sda_read();
  scl_low();
  delay_short();
  return ack;
}

static uint8_t i2c_read_byte(bool ack) {
  uint8_t byte = 0;
  sda_high(); // Release SDA for input
  delay_short();

  for (int i = 0; i < 8; i++) {
    byte <<= 1;
    scl_high();
    delay_short();
    if (sda_read()) {
      byte |= 1;
    }
    scl_low();
    delay_short();
  }

  // Send ACK/NACK
  if (ack)
    sda_low();
  else
    sda_high();
  delay_short();

  scl_high();
  delay_short();
  scl_low();
  delay_short();

  return byte;
}

// --- Public API ---

void meas_drv_i2c_init(void) {
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

  // Configure PB8 (SCL) and PB9 (SDA) as Output Open-Drain
  GPIOB->MODER &= ~((3UL << (SCL_PIN * 2)) | (3UL << (SDA_PIN * 2)));
  GPIOB->MODER |= ((1UL << (SCL_PIN * 2)) | (1UL << (SDA_PIN * 2))); // Output

  GPIOB->OTYPER |= ((1UL << SCL_PIN) | (1UL << SDA_PIN)); // Open-Drain

  GPIOB->OSPEEDR |= ((3UL << (SCL_PIN * 2)) | (3UL << (SDA_PIN * 2))); // High

  // Default High (Idle)
  sda_high();
  scl_high();
}

meas_status_t meas_drv_i2c_write(uint8_t addr, const uint8_t *data,
                                 size_t len) {
  i2c_start();

  if (!i2c_write_byte(addr << 1)) {
    i2c_stop();
    return MEAS_ERROR;
  }

  for (size_t i = 0; i < len; i++) {
    if (!i2c_write_byte(data[i])) {
      i2c_stop();
      return MEAS_ERROR;
    }
  }

  i2c_stop();
  return MEAS_OK;
}

meas_status_t meas_drv_i2c_read(uint8_t addr, uint8_t *data, size_t len) {
  i2c_start();

  // Address + Read Bit
  if (!i2c_write_byte((addr << 1) | 1)) {
    i2c_stop();
    return MEAS_ERROR;
  }

  for (size_t i = 0; i < len; i++) {
    // ACK for all bytes except the last one
    bool ack = (i < (len - 1));
    data[i] = i2c_read_byte(ack);
  }

  i2c_stop();
  return MEAS_OK;
}

meas_status_t meas_drv_i2c_write_regs(uint8_t addr,
                                      const uint8_t *reg_val_pairs,
                                      size_t count) {
  // Since register writes are often pairs, we can just burst them if the device
  // supports auto-increment OR we write them one by one. Many drivers
  // (including TLV320) prefer Address+Reg+Data or Address+Reg+Data... TLV320
  // usually works with Auto-Increment. Si5351 uses register addressing.

  // For simplicity and safety (handling non-auto-increment logic if mixed), we
  // write pairs individually efficiently. Or better: Just check how
  // `tlv320aic3204.c` does it. It does: `i2c_transfer(AIC3204_ADDR, buf, len);`
  // where buf is pairs. If we pass the whole buffer, it implies the device
  // treats it as Reg, Val, Reg, Val... OR Reg, Val, Val, Val... (Auto-inc).
  // Reference code for TLV: `tlv320aic3204_config` calls
  // `tlv320aic3204_bulk_write(data, 2)` inside loop. So it does ONE Register
  // write per transaction (Start, Addr, Reg, Val, Stop).

  for (size_t i = 0; i < count; i += 2) {
    uint8_t buf[2] = {reg_val_pairs[i], reg_val_pairs[i + 1]};
    if (meas_drv_i2c_write(addr, buf, 2) != MEAS_OK) {
      return MEAS_ERROR;
    }
  }
  return MEAS_OK;
}

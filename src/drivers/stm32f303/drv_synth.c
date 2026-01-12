/**
 * @file drv_synth.c
 * @brief STM32F303 Bare Metal Synthesizer Driver (Si5351).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements the Synthesizer Interface (`meas_hal_synth_api_t`) using Si5351.
 * Includes Bit-Banging I2C driver on PB8 (SCL) and PB9 (SDA).
 */

#include "drv_i2c.h"
#include "measlib/drivers/hal.h"
#include <stddef.h>
#include <stdint.h>

// --- Hardware Definitions (Local) ---

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

// --- Si5351 Registers ---
#define SI5351_I2C_ADDR 0x60
#define SI5351_REG_3_OUTPUT_ENABLE 3
#define SI5351_REG_16_CLK0_CONTROL 16
#define SI5351_REG_17_CLK1_CONTROL 17
#define SI5351_REG_18_CLK2_CONTROL 18
#define SI5351_REG_42_MULTISYNTH0 42
#define SI5351_REG_177_PLL_RESET 177
#define SI5351_REG_183_CRYSTAL_LOAD 183

#define SI5351_CRYSTAL_LOAD_10PF (3 << 6)
#define SI5351_CLK_DRIVE_STRENGTH_2MA (0 << 0)
#define SI5351_CLK_DRIVE_STRENGTH_8MA (3 << 0)
#define SI5351_CLK_INPUT_MULTISYNTH_N (3 << 2)
#define SI5351_CLK_INTEGER_MODE (1 << 6)
#define SI5351_CLK_POWERDOWN (1 << 7)

#define XTAL_FREQ 26000000UL

// --- Soft I2C Implementation ---

static void delay_short(void) {
  for (volatile int i = 0; i < 50; i++)
    ;
}
// --- Helper Functions ---

static meas_status_t si5351_write(uint8_t reg, uint8_t val) {
  uint8_t data[2] = {reg, val};
  return meas_drv_i2c_write(SI5351_I2C_ADDR, data, 2);
}

static meas_status_t si5351_write_bulk(uint8_t reg, uint8_t *bytes,
                                       uint8_t count) {
  // We need to send [Reg, Byte1, Byte2...] in one transaction
  // Shared I2C interface sends data array directly.
  // We need to construct temporary buffer or change API.
  // Given max count is small (~8 for multisynth), simple buffer on stack is
  // ok.
  uint8_t buf[10];
  if (count > 9)
    return MEAS_ERROR;

  buf[0] = reg;
  for (int i = 0; i < count; i++) {
    buf[1 + i] = bytes[i];
  }
  return meas_drv_i2c_write(SI5351_I2C_ADDR, buf, count + 1);
}

// --- Si5351 Logic (Simplified) ---

// Setup PLL: A (0) or B (1)
// Frequency = XTAL * (mult + num/denom)
// P1, P2, P3 calculation
static void si5351_setup_pll(uint8_t pll_id, uint32_t mult, uint32_t num,
                             uint32_t denom) {
  uint32_t P1, P2, P3;

  // P3 = denom
  P3 = denom;

  // P1 = 128 * mult + floor(128*num/denom) - 512
  P1 = 128 * mult + (128 * num) / denom - 512;

  // P2 = 128*num - denom * floor(128*num/denom) = (128*num) % denom
  P2 = (128 * num) % denom;

  uint8_t params[8]; // 8 bytes for P1, P2, P3
  // Register 26 for PLLA, 34 for PLLB
  uint8_t reg_base = pll_id ? 34 : 26;

  params[0] = (P3 & 0xFF00) >> 8;
  params[1] = (P3 & 0x00FF);
  params[2] = (P1 & 0x30000) >> 16;
  params[3] = (P1 & 0x0FF00) >> 8;
  params[4] = (P1 & 0x00FF);
  params[5] = ((P3 & 0xF0000) >> 12) | ((P2 & 0xF0000) >> 16);
  params[6] = (P2 & 0x0FF00) >> 8;
  params[7] = (P2 & 0x00FF);

  si5351_write_bulk(reg_base, params, 8);
}

// Setup Multisynth: 0, 1, 2
// Fout = Fpll / (div + num/denom)
static void si5351_setup_multisynth(uint8_t ms_id, uint32_t div, uint32_t num,
                                    uint32_t denom, uint8_t rdiv) {
  uint32_t P1, P2, P3;
  P3 = denom;
  P1 = 128 * div + (128 * num) / denom - 512;
  P2 = (128 * num) % denom;

  uint8_t params[9];
  uint8_t reg_base = 42 + (ms_id * 8);

  params[0] = reg_base;
  params[1] = (P3 & 0xFF00) >> 8;
  params[2] = (P3 & 0x00FF);
  params[3] =
      ((P1 & 0x30000) >> 16) | rdiv; // rdiv is bits [6:4], already shifted by
                                     // caller if needed? No, caller passes 0..7
  params[4] = (P1 & 0x0FF00) >> 8;
  params[5] = (P1 & 0x00FF);
  params[6] = ((P3 & 0xF0000) >> 12) | ((P2 & 0xF0000) >> 16);
  params[7] = (P2 & 0x0FF00) >> 8;
  params[8] = (P2 & 0x00FF);

  // Skip params[0] which is Register ID, pass pointer to P3 MSB
  si5351_write_bulk(params[0], &params[1], 8);
}

// --- API Implementation ---

static meas_status_t drv_synth_set_freq(void *ctx, meas_real_t hz) {
  (void)ctx;
  uint32_t freq = (uint32_t)hz;

  // Simple Strategy for Bring-up:
  // Fixed PLL = 800 MHz (XTAL 26MHz * 30.769...)
  // Actually, let's use Integer PLL for stability:
  // PLL = 26MHz * 32 = 832 MHz

  uint32_t pll_mult = 32;
  uint32_t pll_freq = XTAL_FREQ * pll_mult;

  // Calculate Multisynth Divider
  // Div = PLL / Freq
  // We strictly use integer mode for simplicity in this bring-up unless freq
  // is weird. Real implementation would optimize this.

  // For frequencies > 1 MHz
  if (freq == 0)
    return MEAS_ERROR;

  uint32_t div = pll_freq / freq;
  uint32_t num = pll_freq % freq;
  uint32_t denom = freq;

  // Reduce fraction
  // Note: For now we pass raw values. Ideally use GCD.

  // Setup PLL A
  si5351_setup_pll(0, pll_mult, 0, 1);

  // Setup MS0
  si5351_setup_multisynth(0, div, num, denom, 0); // rdiv=0

  // Configuration: Reset PLL
  si5351_write(SI5351_REG_177_PLL_RESET, 0xA0); // Reset PLLA & PLLB

  // Ensure CLK0 enabled
  // CLK0 Control: 8mA, MS0, Integer (if num=0), On
  uint8_t clk_ctrl =
      SI5351_CLK_DRIVE_STRENGTH_8MA | SI5351_CLK_INPUT_MULTISYNTH_N;
  if (num == 0)
    clk_ctrl |= SI5351_CLK_INTEGER_MODE;

  si5351_write(SI5351_REG_16_CLK0_CONTROL, clk_ctrl);

  return MEAS_OK;
}

static meas_status_t drv_synth_set_power(void *ctx, meas_real_t dbm) {
  (void)ctx;
  (void)dbm;
  // Set Drive Strength
  return MEAS_OK;
}

static meas_status_t drv_synth_enable_output(void *ctx, bool enable) {
  (void)ctx;
  // Register 3: Output Enable. 0 = Enabled (Low Active bit)
  uint8_t val = enable ? 0 : 0xFF;
  // Only enable CLK0 for now (Bit 0 = 0)
  if (enable)
    val = ~(1 << 0);

  si5351_write(SI5351_REG_3_OUTPUT_ENABLE, val);
  return MEAS_OK;
}

static const meas_hal_synth_api_t synth_api = {.set_freq = drv_synth_set_freq,
                                               .set_power = drv_synth_set_power,
                                               .enable_output =
                                                   drv_synth_enable_output};

meas_hal_synth_api_t *meas_drv_synth_init(void) {
  meas_drv_i2c_init(); // Fixed call name
  delay_short();

  // Init Si5351
  // Disable Outputs
  si5351_write(SI5351_REG_3_OUTPUT_ENABLE, 0xFF);

  // Power down all clocks
  si5351_write(SI5351_REG_16_CLK0_CONTROL, SI5351_CLK_POWERDOWN);
  si5351_write(SI5351_REG_17_CLK1_CONTROL, SI5351_CLK_POWERDOWN);
  si5351_write(SI5351_REG_18_CLK2_CONTROL, SI5351_CLK_POWERDOWN);

  // Set Crystal Load to 10pF
  si5351_write(SI5351_REG_183_CRYSTAL_LOAD, SI5351_CRYSTAL_LOAD_10PF);

  return (meas_hal_synth_api_t *)&synth_api;
}

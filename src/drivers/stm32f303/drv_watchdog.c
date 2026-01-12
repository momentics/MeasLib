/**
 * @file drv_watchdog.c
 * @brief STM32F303 Independent Watchdog (IWDG) Driver.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements the Watchdog Interface (`meas_hal_wdg_api_t`).
 * Uses the LSI (40kHz) clock source.
 */

#include "measlib/drivers/hal.h"
#include <stddef.h>
#include <stdint.h>

// -- Hardware Registers --
#define IWDG_BASE 0x40003000UL

typedef struct {
  volatile uint32_t KR;  // Key Register
  volatile uint32_t PR;  // Prescaler Register
  volatile uint32_t RLR; // Reload Register
  volatile uint32_t SR;  // Status Register
} IWDG_TypeDef;

#define IWDG ((IWDG_TypeDef *)IWDG_BASE)

// Key Values
#define IWDG_KEY_RELOAD 0x0000AAAA
#define IWDG_KEY_ENABLE 0x0000CCCC
#define IWDG_KEY_ACCESS 0x00005555

// Prescaler Values
#define IWDG_PR_DIV_4 0x00
#define IWDG_PR_DIV_8 0x01
#define IWDG_PR_DIV_16 0x02
#define IWDG_PR_DIV_32 0x03
#define IWDG_PR_DIV_64 0x04
#define IWDG_PR_DIV_128 0x05
#define IWDG_PR_DIV_256 0x06

#define LSI_FREQ_HZ 40000UL

// -- Private Context --
static struct {
  bool active;
} wdg_ctx;

// -- Implementation --

static meas_status_t wdg_start(void *ctx, uint32_t timeout_ms) {
  (void)ctx;

  // 1. Enable IWDG (LSI started by hardware)
  IWDG->KR = IWDG_KEY_ENABLE;

  // 2. Enable Access to PR/RLR
  IWDG->KR = IWDG_KEY_ACCESS;

  // 3. Calculate Prescaler & Reload
  // Timeout = (RLR * PR_DIV) / LSI_FREQ
  // Max Timeout @ Div256 = (4095 * 256) / 40000 = ~26.2 sec
  // Min Timeout @ Div4   = (0 * 4) / 40000       = 0.1 ms (approx)

  // Use fixed Prescaler /256 for simplicity and broad range
  // Tick = 256 / 40000 = 6.4ms
  IWDG->PR = IWDG_PR_DIV_256;

  // RLR = Timeout_ms / 6.4
  //     = (Timeout_ms * 1000) / 6400
  //     = (Timeout_ms * 10) / 64
  //     = (Timeout_ms * 5) / 32
  uint32_t reload = (timeout_ms * 5) / 32;

  if (reload > 0xFFF) {
    reload = 0xFFF; // Clamp to 12-bit max
  }

  while (IWDG->SR != 0) {
    // Wait for register update
  }

  IWDG->RLR = reload;

  // 4. Reload Counter (Start Counting)
  IWDG->KR = IWDG_KEY_RELOAD;

  wdg_ctx.active = true;
  return MEAS_OK;
}

static meas_status_t wdg_kick(void *ctx) {
  (void)ctx;
  if (!wdg_ctx.active) {
    return MEAS_ERROR;
  }
  IWDG->KR = IWDG_KEY_RELOAD;
  return MEAS_OK;
}

static const meas_hal_wdg_api_t api = {
    .start = wdg_start,
    .kick = wdg_kick,
};

// -- Public Initialization --

meas_hal_wdg_api_t *meas_drv_wdg_init(void) {
  wdg_ctx.active = false;
  return (meas_hal_wdg_api_t *)&api;
}

/**
 * @file drv_flash.c
 * @brief STM32F303 Bare Metal Flash Driver.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements the Flash Interface (`meas_hal_flash_api_t`).
 * Handles unlocking, erasing pages, and programming data to the internal Flash
 * memory.
 */

#include "measlib/drivers/hal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// --- Register Definitions ---

typedef struct {
  volatile uint32_t ACR;      // 0x00
  volatile uint32_t KEYR;     // 0x04
  volatile uint32_t OPTKEYR;  // 0x08
  volatile uint32_t SR;       // 0x0C
  volatile uint32_t CR;       // 0x10
  volatile uint32_t AR;       // 0x14
  volatile uint32_t RESERVED; // 0x18
  volatile uint32_t OBR;      // 0x1C
  volatile uint32_t WRPR;     // 0x20
} FLASH_TypeDef;

#define FLASH_BASE 0x40022000UL
#define FLASH ((FLASH_TypeDef *)FLASH_BASE)

// Keys
#define FLASH_KEY1 0x45670123UL
#define FLASH_KEY2 0xCDEF89ABUL

// Status Register Flags
#define FLASH_SR_BSY (1UL << 0)
#define FLASH_SR_PGERR (1UL << 2)
#define FLASH_SR_WRPERR (1UL << 4)
#define FLASH_SR_EOP (1UL << 5)

// Control Register Flags
#define FLASH_CR_PG (1UL << 0)
#define FLASH_CR_PER (1UL << 1)
#define FLASH_CR_MER (1UL << 2)
#define FLASH_CR_OPTPG (1UL << 4)
#define FLASH_CR_OPTER (1UL << 5)
#define FLASH_CR_STRT (1UL << 6)
#define FLASH_CR_LOCK (1UL << 7)
#define FLASH_CR_EOPIE (1UL << 12)
#define FLASH_CR_ERRIE (1UL << 10)

#define FLASH_PAGESIZE 2048UL

// --- Helper Functions ---

static inline void flash_clear_status_flags(void) {
  uint32_t flags = FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGERR;
  FLASH->SR = flags;
}

static int flash_wait_for_last_operation(void) {
  volatile uint32_t timeout = 0x100000;

  while ((FLASH->SR & FLASH_SR_BSY) && (timeout > 0)) {
    timeout--;
  }

  if (timeout == 0) {
    FLASH->SR |= FLASH_SR_EOP; // Try to clear if stuck
    return -1;
  }

  // Check errors
  if (FLASH->SR & (FLASH_SR_WRPERR | FLASH_SR_PGERR)) {
    flash_clear_status_flags();
    return -1;
  }

  flash_clear_status_flags();
  return 0;
}

// --- Critical Section Helpers ---

// Defined in boards/STM32F303/board.c
extern uint32_t sys_enter_critical(void);
extern void sys_exit_critical(uint32_t state);

static meas_status_t drv_flash_unlock(void *ctx) {
  (void)ctx;
  if ((FLASH->CR & FLASH_CR_LOCK) != 0) {
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
  }
  return MEAS_OK;
}

static meas_status_t drv_flash_lock(void *ctx) {
  (void)ctx;
  FLASH->CR |= FLASH_CR_LOCK;
  return MEAS_OK;
}

static meas_status_t drv_flash_erase_page(void *ctx, uint32_t address) {
  (void)ctx;

  uint32_t primask = sys_enter_critical();
  drv_flash_unlock(ctx);

  if (flash_wait_for_last_operation() != 0) {
    drv_flash_lock(ctx);
    sys_exit_critical(primask);
    return MEAS_ERROR;
  }

  FLASH->CR |= FLASH_CR_PER;
  FLASH->AR = address;
  FLASH->CR |= FLASH_CR_STRT;

  if (flash_wait_for_last_operation() != 0) {
    FLASH->CR &= ~FLASH_CR_PER;
    drv_flash_lock(ctx);
    sys_exit_critical(primask);
    return MEAS_ERROR;
  }

  FLASH->CR &= ~FLASH_CR_PER;
  drv_flash_lock(ctx);
  sys_exit_critical(primask);
  return MEAS_OK;
}

static meas_status_t drv_flash_program(void *ctx, uint32_t address,
                                       const void *data, size_t len) {
  (void)ctx;
  const uint16_t *p_data = (const uint16_t *)data;
  uint16_t *p_addr = (uint16_t *)address; // Flash is mapped
  size_t num_half_words = len / 2;

  // Ensure 16-bit alignment
  if ((address & 1) || (len & 1)) {
    return MEAS_ERROR;
  }

  drv_flash_unlock(ctx);

  for (size_t i = 0; i < num_half_words; i++) {
    uint32_t primask = sys_enter_critical();

    if (flash_wait_for_last_operation() != 0) {
      sys_exit_critical(primask);
      drv_flash_lock(ctx);
      return MEAS_ERROR;
    }

    FLASH->CR |= FLASH_CR_PG;
    p_addr[i] = p_data[i];

    if (flash_wait_for_last_operation() != 0) {
      FLASH->CR &= ~FLASH_CR_PG;
      sys_exit_critical(primask);
      drv_flash_lock(ctx);
      return MEAS_ERROR;
    }

    FLASH->CR &= ~FLASH_CR_PG;
    sys_exit_critical(primask);
  }

  drv_flash_lock(ctx);
  return MEAS_OK;
}

static const meas_hal_flash_api_t flash_api = {.unlock = drv_flash_unlock,
                                               .lock = drv_flash_lock,
                                               .erase_page =
                                                   drv_flash_erase_page,
                                               .program = drv_flash_program};

meas_hal_flash_api_t *meas_drv_flash_init(void) {
  // Flash doesn't typically need RCC enable on F303 (it's always on),
  // but let's clear status flags just in case.
  flash_clear_status_flags();
  return (meas_hal_flash_api_t *)&flash_api;
}

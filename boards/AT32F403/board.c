/**
 * @file board.c
 * @brief AT32F403 Board Support Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 */

#include "measlib/drivers/api.h"
#include <stdint.h>

// CMSIS-style inline assembly for Cortex-M4
static inline uint32_t __get_PRIMASK(void) {
  uint32_t result;
  __asm volatile("MRS %0, primask" : "=r"(result));
  return result;
}

static inline void __set_PRIMASK(uint32_t primask) {
  __asm volatile("MSR primask, %0" : : "r"(primask) : "memory");
}

static inline void __disable_irq(void) {
  __asm volatile("cpsid i" : : : "memory");
}

uint32_t sys_enter_critical(void) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  return primask;
}

void sys_exit_critical(uint32_t state) { __set_PRIMASK(state); }

/*
 * System Core Clock (Standard CMSIS)
 */
uint32_t SystemCoreClock = 240000000; // Default to 240MHz (Max for AT32F403)

/**
 * @brief  Setup the microcontroller system
 *         Initialize the Embedded Flash Interface, the PLL and update the
 *         SystemCoreClock variable.
 */
void SystemInit(void) {
  // FPU is handled in startup.s

  // TODO: Implement real clock configuration here.
  // For now, we assume the bootloader or hardware defaults are sufficient
  // or that a later driver initializes the PLL.
  // Ideally, this should set up the RCC to run at max frequency (240MHz).

  SystemCoreClock = 240000000;
}

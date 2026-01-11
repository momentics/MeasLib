/**
 * @file board.c
 * @brief STM32F072 Board Support Implementation.
 */

#include "measlib/drivers/api.h"
#include <stdint.h>

// CMSIS-style inline assembly for Cortex-M0
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

// -- Hardware Registers (Minimal Definitions for F0) --
#define SysTick_BASE (0xE000E010UL)
typedef struct {
  volatile uint32_t CTRL;
  volatile uint32_t LOAD;
  volatile uint32_t VAL;
  volatile uint32_t CALIB;
} SysTick_TypeDef;
#define SysTick ((SysTick_TypeDef *)SysTick_BASE)

#define SysTick_CTRL_CLKSOURCE_Msk (1UL << 2)
#define SysTick_CTRL_TICKINT_Msk (1UL << 1)
#define SysTick_CTRL_ENABLE_Msk (1UL << 0)

uint32_t SystemCoreClock = 8000000; // Default HSI 8MHz

void sys_tick_init(void) {
  // 8MHz / 1000 = 8000 ticks
  SysTick->LOAD = (SystemCoreClock / 1000) - 1;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |
                  SysTick_CTRL_ENABLE_Msk;
}

void sys_init(void) { sys_tick_init(); }

volatile uint32_t sys_tick_counter = 0;

void SysTick_Handler(void) { sys_tick_counter++; }

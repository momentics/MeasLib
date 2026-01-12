/**
 * @file drv_controls.c
 * @brief STM32F303 Bare Metal Controls Driver (LED, Buttons).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements the IO Interface (`meas_hal_io_api_t`).
 * Target Pinout:
 * - LED: PC13
 * - Button Left:   PA1
 * - Button Middle: PA2
 * - Button Right:  PA3
 */

#include "measlib/drivers/hal.h"
#include <stddef.h>
#include <stdint.h>

// --- Register Definitions ---

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
#define RCC ((RCC_TypeDef *)RCC_BASE)

#define GPIOA_BASE 0x48000000UL
#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)

#define GPIOC_BASE 0x48000800UL
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)

#define RCC_AHBENR_GPIOAEN (1UL << 17)
#define RCC_AHBENR_GPIOCEN (1UL << 19)

#define PIN_BTN_MENU 0  // PA0 (Unused/Free in Ref)
#define PIN_BTN_LEFT 1  // PA1
#define PIN_BTN_RIGHT 2 // PA2 (Swapped to match Ref)
#define PIN_BTN_PUSH 3  // PA3 (Swapped to match Ref)
#define PIN_LED 13      // PC13

// ...

// --- API Implementation ---

static meas_status_t drv_set_led(void *ctx, bool on) {
  (void)ctx;
  if (on) {
    GPIOC->BSRR = (1UL << (PIN_LED + 16));
  } else {
    GPIOC->BSRR = (1UL << PIN_LED);
  }
  return MEAS_OK;
}

static uint32_t drv_read_buttons(void *ctx) {
  (void)ctx;
  uint32_t idr = GPIOA->IDR;
  uint32_t mask = 0;

  if (idr & (1UL << PIN_BTN_MENU))
    mask |= 8;
  if (idr & (1UL << PIN_BTN_LEFT))
    mask |= 1;
  if (idr & (1UL << PIN_BTN_PUSH))
    mask |= 2;
  if (idr & (1UL << PIN_BTN_RIGHT))
    mask |= 4;

  return mask;
}

static const meas_hal_io_api_t io_api = {.set_led = drv_set_led,
                                         .read_buttons = drv_read_buttons};

meas_hal_io_api_t *meas_drv_controls_init(void) {
  // 1. Enable Clocks (GPIOA, GPIOC)
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOCEN;

  // 2. Configure LED (PC13) -> Output PushPull
  GPIOC->MODER &= ~(3UL << (PIN_LED * 2));
  GPIOC->MODER |= (1UL << (PIN_LED * 2)); // 01: Output
  // Default Off (High)
  GPIOC->BSRR = (1UL << PIN_LED);

  // 3. Configure Buttons (PA0, PA1, PA2, PA3) -> Input, Pull-Down
  // MODER: 00 (Input) - Default
  GPIOA->MODER &= ~((3UL << (PIN_BTN_MENU * 2)) | (3UL << (PIN_BTN_LEFT * 2)) |
                    (3UL << (PIN_BTN_PUSH * 2)) | (3UL << (PIN_BTN_RIGHT * 2)));

  // PUPDR: 10 (Pull-Down)
  // Clear
  GPIOA->PUPDR &= ~((3UL << (PIN_BTN_MENU * 2)) | (3UL << (PIN_BTN_LEFT * 2)) |
                    (3UL << (PIN_BTN_PUSH * 2)) | (3UL << (PIN_BTN_RIGHT * 2)));
  // Set 10 (Binary 2)
  GPIOA->PUPDR |= ((2UL << (PIN_BTN_MENU * 2)) | (2UL << (PIN_BTN_LEFT * 2)) |
                   (2UL << (PIN_BTN_PUSH * 2)) | (2UL << (PIN_BTN_RIGHT * 2)));

  return (meas_hal_io_api_t *)&io_api;
}

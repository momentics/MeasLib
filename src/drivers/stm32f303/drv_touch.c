/**
 * @file drv_touch.c
 * @brief STM32F303 Bare Metal Resistive Touch Driver.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements the Touch Interface (`meas_hal_touch_api_t`).
 * Hardware:
 * - XP: PA6 (ADC2_IN3)
 * - YP: PA7 (ADC2_IN4)
 * - XN: PB0
 * - YN: PB1
 * - ADC: ADC2
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
  volatile uint32_t CR;
  volatile uint32_t IER;
  volatile uint32_t ISR;
  volatile uint32_t CFGR;
  volatile uint32_t SMPR1;
  volatile uint32_t SMPR2;
  volatile uint32_t TR1;
  volatile uint32_t TR2;
  volatile uint32_t TR3;
  volatile uint32_t SQR1;
  volatile uint32_t SQR2;
  volatile uint32_t SQR3;
  volatile uint32_t SQR4;
  volatile uint32_t DR;
  volatile uint32_t JSQR;
  volatile uint32_t OFR1;
  volatile uint32_t OFR2;
  volatile uint32_t OFR3;
  volatile uint32_t OFR4;
  volatile uint32_t JDR1;
  volatile uint32_t JDR2;
  volatile uint32_t JDR3;
  volatile uint32_t JDR4;
  volatile uint32_t JIER;
  volatile uint32_t JISR;
  volatile uint32_t JINJ;
  volatile uint32_t DIFSEL;
  volatile uint32_t CALFACT;
} ADC_TypeDef;

typedef struct {
  volatile uint32_t AHBENR;
} RCC_TypeDef;

#define RCC_BASE 0x40021000UL
#define RCC ((RCC_TypeDef *)RCC_BASE)

#define GPIOA_BASE 0x48000000UL
#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)

#define GPIOB_BASE 0x48000400UL
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)

#define ADC2_BASE 0x50000100UL
#define ADC2 ((ADC_TypeDef *)ADC2_BASE)

#define RCC_AHBENR_ADC12EN (1UL << 28)
#define RCC_AHBENR_GPIOAEN (1UL << 17)
#define RCC_AHBENR_GPIOBEN (1UL << 18)

// Bit Definitions
#define ADC_CR_ADEN (1UL << 0)
#define ADC_CR_ADSTART (1UL << 2)
#define ADC_CR_ADCAL (1UL << 31)
#define ADC_CR_ADVREGEN_0 (1UL << 28)
#define ADC_ISR_ADRDY (1UL << 0)
#define ADC_ISR_EOC (1UL << 2)

// Pins
#define PIN_XP 6
#define PIN_YP 7
#define PIN_XN 0
#define PIN_YN 1

// --- Helper Functions ---

static void delay_us(int us) {
  for (volatile int i = 0; i < us * 10; i++)
    ;
}

// Config Helper
// Mode: 00=In, 01=Out, 10=Alt, 11=Analog
static void set_gpio_mode(GPIO_TypeDef *port, int pin, int mode) {
  port->MODER &= ~(3UL << (pin * 2));
  port->MODER |= ((uint32_t)mode << (pin * 2));
}

static void set_gpio_low(GPIO_TypeDef *port, int pin) {
  port->BSRR = (1UL << (pin + 16));
}

static void set_gpio_high(GPIO_TypeDef *port, int pin) {
  port->BSRR = (1UL << pin);
}

// ADC Read (Blocking)
// Requires ADC2 to be enabled
static uint16_t adc2_read(int channel) {
  // SQR1: L=0 (1 conversion), SQ1=channel
  // Channel is in SQ1 bits 6..10
  ADC2->SQR1 = (channel << 6);

  ADC2->CR |= ADC_CR_ADSTART;
  while (!(ADC2->ISR & ADC_ISR_EOC))
    ;
  return (uint16_t)ADC2->DR;
}

// --- Logic ---

static meas_status_t drv_touch_read(void *ctx, int16_t *x, int16_t *y) {
  (void)ctx;
  // 1. Read X
  // Drive X: XP=1, XN=0 (Output)
  // Read Y: YP (Input Analog)

  set_gpio_mode(GPIOA, PIN_XP, 1);
  set_gpio_high(GPIOA, PIN_XP);
  set_gpio_mode(GPIOB, PIN_XN, 1);
  set_gpio_low(GPIOB, PIN_XN);

  set_gpio_mode(GPIOA, PIN_YP, 3); // Analog
  set_gpio_mode(GPIOB, PIN_YN, 0); // Input Floating

  delay_us(20); // Settle

  // Read Channel 4 (PA7 - YP)
  uint16_t x_val = adc2_read(4);

  // 2. Read Y
  // Drive Y: YP=1, YN=0 (Output)
  // Read X: XP (Input Analog)

  set_gpio_mode(GPIOA, PIN_YP, 1);
  set_gpio_high(GPIOA, PIN_YP);
  set_gpio_mode(GPIOB, PIN_YN, 1);
  set_gpio_low(GPIOB, PIN_YN);

  set_gpio_mode(GPIOA, PIN_XP, 3); // Analog
  set_gpio_mode(GPIOB, PIN_XN, 0); // Input Floating

  delay_us(20);

  // Read Channel 3 (PA6 - XP)
  uint16_t y_val = adc2_read(3);

  // 3. Restore to Standby (Touch Detect) state if desired, or High-Z
  set_gpio_mode(GPIOA, PIN_XP, 0);
  set_gpio_mode(GPIOA, PIN_YP, 0);
  set_gpio_mode(GPIOB, PIN_XN, 0);
  set_gpio_mode(GPIOB, PIN_YN, 0);

  // Simple pressure check (if X is near 0 or max, no touch)
  if (x_val < 100 || x_val > 4000 || y_val < 100 || y_val > 4000) {
    return MEAS_ERROR; // No Touch
  }

  *x = x_val;
  *y = y_val;
  return MEAS_OK;
}

static const meas_hal_touch_api_t touch_api = {.read_point = drv_touch_read};

// --- Init ---

meas_hal_touch_api_t *meas_drv_touch_init(void) {
  // 1. Clocks
  RCC->AHBENR |= RCC_AHBENR_ADC12EN | RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;

  // 2. ADC2 Init
  if (ADC2->CR & ADC_CR_ADEN) {
    ADC2->CR |= (1UL << 4); // ADSTP
    while (ADC2->CR & ADC_CR_ADSTART)
      ;
    ADC2->CR |= (1UL << 1); // ADDIS
    while (ADC2->CR & ADC_CR_ADEN)
      ;
  }

  ADC2->CR |= ADC_CR_ADVREGEN_0;
  delay_us(20);

  ADC2->CR |= ADC_CR_ADCAL;
  while (ADC2->CR & ADC_CR_ADCAL)
    ;

  ADC2->CR |= ADC_CR_ADEN;
  while (!(ADC2->ISR & ADC_ISR_ADRDY))
    ;

  return (meas_hal_touch_api_t *)&touch_api;
}

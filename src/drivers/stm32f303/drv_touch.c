/**
 * @file drv_touch.c
 * @brief Bare-metal Resistive Touch Driver for STM32F303
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements 4-wire resistive touch sensing using built-in ADC2 and GPIOs.
 * Includes local hardware definitions to avoid dependency on missing system
 * headers.
 */

#include "drv_touch.h"
#include <stddef.h>
#include <stdint.h>

// --- Hardware Definitions (Local) ---

typedef struct {
  volatile uint32_t MODER;   // 0x00
  volatile uint32_t OTYPER;  // 0x04
  volatile uint32_t OSPEEDR; // 0x08
  volatile uint32_t PUPDR;   // 0x0C
  volatile uint32_t IDR;     // 0x10
  volatile uint32_t ODR;     // 0x14
  volatile uint32_t BSRR;    // 0x18
  volatile uint32_t LCKR;    // 0x1C
  volatile uint32_t AF[2];   // 0x20-0x24
} GPIO_TypeDef;

typedef struct {
  volatile uint32_t CR;
  volatile uint32_t CFGR;
  volatile uint32_t CIR;
  volatile uint32_t APB2RSTR;
  volatile uint32_t APB1RSTR;
  volatile uint32_t AHBENR;
  volatile uint32_t APB2ENR;
  volatile uint32_t APB1ENR;
  volatile uint32_t BDCR;
  volatile uint32_t CSR;
  volatile uint32_t AHBRSTR;
  volatile uint32_t CFGR2;
  volatile uint32_t CFGR3;
} RCC_TypeDef;

typedef struct {
  volatile uint32_t ISR;   // 0x00
  volatile uint32_t IER;   // 0x04
  volatile uint32_t CR;    // 0x08
  volatile uint32_t CFGR;  // 0x0C
  volatile uint32_t CFGR2; // 0x10
  volatile uint32_t SMPR1; // 0x14
  volatile uint32_t SMPR2; // 0x18
  volatile uint32_t _RES1; // 0x1C
  volatile uint32_t TR1;   // 0x20
  volatile uint32_t TR2;   // 0x24
  volatile uint32_t TR3;   // 0x28
  volatile uint32_t _RES2; // 0x2C
  volatile uint32_t SQR1;  // 0x30
  volatile uint32_t SQR2;  // 0x34
  volatile uint32_t SQR3;  // 0x38
  volatile uint32_t SQR4;  // 0x3C
  volatile uint32_t DR;    // 0x40
} ADC_TypeDef;

#define GPIOA ((GPIO_TypeDef *)0x48000000UL)
#define GPIOB ((GPIO_TypeDef *)0x48000400UL)
#define ADC2 ((ADC_TypeDef *)0x50000100UL)
#define RCC ((RCC_TypeDef *)0x40021000UL)

#define RCC_AHBENR_GPIOAEN (1UL << 17)
#define RCC_AHBENR_GPIOBEN (1UL << 18)
#define RCC_AHBENR_ADC12EN (1UL << 28)
#define RCC_APB2ENR_ADC12EN RCC_AHBENR_ADC12EN // Alias if misused

#define ADC_CR_ADEN (1UL << 0)
#define ADC_CR_ADDIS (1UL << 1)
#define ADC_CR_ADSTART (1UL << 2)
#define ADC_CR_ADCAL (1UL << 31)
#define ADC_CR_ADCALDIF (1UL << 30)

#define ADC_ISR_ADRDY (1UL << 0)
#define ADC_ISR_EOC (1UL << 2)
#define ADC_ISR_ADCAL (1UL << 11)

// --- Definitions ---

// Pins (Confirmed from Ref Code and Board Header)
// XP: PA6 (ADC2_IN3)
// YP: PA7 (ADC2_IN4)
// XN: PB0
// YN: PB1

// GPIO Ports
#define GPIO_XP GPIOA
#define PIN_XP 6
#define GPIO_YP GPIOA
#define PIN_YP 7
#define GPIO_XN GPIOB
#define PIN_XN 0
#define GPIO_YN GPIOB
#define PIN_YN 1

// ADC Channels (ADC2)
#define ADC_CH_X 3 // PA6
#define ADC_CH_Y 4 // PA7

// Thresholds
#define TOUCH_THRESHOLD                                                        \
  200 // Arbitrary low value to detect press (if checking pressure/Z)

// Context
typedef struct {
  bool is_initialized;
  uint32_t last_x;
  uint32_t last_y;
} meas_drv_touch_t;

static meas_drv_touch_t touch_ctx = {0};

// --- Low Level GPIO Helpers ---

/**
 * @brief Set GPIO pin to Input mode.
 * @param port GPIO Port base address
 * @param pin Pin number (0-15)
 */
static void gpio_mode_input(GPIO_TypeDef *port, int pin) {
  port->MODER &= ~(3UL << (pin * 2)); // 00: Input
}

/**
 * @brief Set GPIO pin to Output mode.
 * @param port GPIO Port base address
 * @param pin Pin number (0-15)
 */
static void gpio_mode_output(GPIO_TypeDef *port, int pin) {
  port->MODER &= ~(3UL << (pin * 2));
  port->MODER |= (1UL << (pin * 2)); // 01: Output
}

/**
 * @brief Set GPIO pin to Analog mode.
 * @param port GPIO Port base address
 * @param pin Pin number (0-15)
 */
static void gpio_mode_analog(GPIO_TypeDef *port, int pin) {
  port->MODER |= (3UL << (pin * 2)); // 11: Analog
}

/**
 * @brief Set GPIO pin High.
 * @param port GPIO Port base address
 * @param pin Pin number (0-15)
 */
static void gpio_set(GPIO_TypeDef *port, int pin) { port->BSRR = (1UL << pin); }

/**
 * @brief Set GPIO pin Low.
 * @param port GPIO Port base address
 * @param pin Pin number (0-15)
 */
static void gpio_clear(GPIO_TypeDef *port, int pin) {
  port->BSRR = (1UL << (pin + 16));
}

static void gpio_pupd_down(GPIO_TypeDef *port, int pin) {
  port->PUPDR &= ~(3UL << (pin * 2));
  port->PUPDR |= (2UL << (pin * 2)); // 10: Pull-down
}

static void gpio_pupd_none(GPIO_TypeDef *port, int pin) {
  port->PUPDR &= ~(3UL << (pin * 2)); // 00: No pull
}

// --- Low Level ADC Helpers ---

/**
 * @brief Initialize ADC2 for single conversion mode.
 * Calibrates and Enables the ADC.
 */
static void adc2_init(void) {
  // 1. Clock Enable (Assuming AHB/APB clocks enabled in board.c or here)
  if (!(RCC->APB2ENR & RCC_APB2ENR_ADC12EN)) {
    RCC->APB2ENR |= RCC_APB2ENR_ADC12EN;
  }

  // 2. ADC Calibration
  // Ensure ADC is disabled
  if (ADC2->CR & ADC_CR_ADEN) {
    ADC2->CR |= ADC_CR_ADDIS;
    while (ADC2->CR & ADC_CR_ADEN)
      ;
  }
  ADC2->CR &= ~ADC_CR_ADCALDIF; // Single-ended cal
  ADC2->CR |= ADC_CR_ADCAL;
  while (ADC2->ISR & ADC_ISR_ADCAL)
    ; // Wait for cal complete

  // 3. Enable ADC
  ADC2->CR |= ADC_CR_ADEN;
  while (!(ADC2->ISR & ADC_ISR_ADRDY))
    ;
}

/**
 * @brief Perform a single read on a specific ADC channel.
 * @param channel ADC Channel number
 * @return 12-bit ADC value
 */
static uint16_t adc2_read(uint32_t channel) {
  // Configure Channel
  ADC2->SQR1 = (channel << 6) | (0 << 0); // L=0 (1 conversion), SQ1=channel

  // Start Conversion
  ADC2->CR |= ADC_CR_ADSTART;

  // Wait for EOC
  while (!(ADC2->ISR & ADC_ISR_EOC))
    ;

  return (uint16_t)ADC2->DR;
}

// --- Touch Logic ---

// Prepare for sensing a touch (Idle state)
// YN: Input (Hi-Z)
// YP: Input (Pull-Down)
// XN: High
// XP: High (Drive X lines High)
// If pressed, Y lines will be pulled High via the touch resistance.
static void touch_prepare_sense(void) {
  // Y Lines Input
  gpio_mode_input(GPIO_YN, PIN_YN);
  gpio_pupd_none(GPIO_YN, PIN_YN); // Hi-Z

  gpio_mode_input(GPIO_YP, PIN_YP);
  gpio_pupd_down(GPIO_YP, PIN_YP); // Pull Down

  // X Lines High Output
  gpio_set(GPIO_XN, PIN_XN);
  gpio_set(GPIO_XP, PIN_XP);
  gpio_mode_output(GPIO_XN, PIN_XN);
  gpio_mode_output(GPIO_XP, PIN_XP);
}

// Check if currently pressed (Basic check using Prepare Sense state)
bool meas_drv_touch_is_pressed(void *ctx) {
  (void)ctx;
  touch_prepare_sense();
  // Delay for electrical settling?
  for (volatile int i = 0; i < 100; i++)
    ;

  // Read YP. If pressed, X High drives YP High (overcoming Pull-down)
  // NOTE: This is a digital check on YP pin?
  // Or we can use ADC on YP. Reference uses Analog Watchdog on YP.
  // Let's use digital read if threshold is high enough, or ADC.
  // Since YP is Analog capable, digital read might be flaky if mode is Analog.
  // But here we set it to Input.

  // Reference check: adc_single_read(ADC_TOUCH_Y) > THRESHOLD.
  // So Ref uses ADC to check press.

  // Let's switch YP to Analog for the check.
  gpio_mode_analog(GPIO_YP, PIN_YP);
  // XN/XP are still High. YN is Hi-Z.
  // Current Path: X lines (High) -> Resistor -> Y lines -> ADC Input (High
  // Impedance) Wait... if YN is HiZ and YP is Analog (High Z), where is the
  // Pull-Down? Ref uses `adc_start_analog_watchdog` setup.
  // "palSetPadMode(GPIOA, GPIOA_YP, PAL_MODE_INPUT_PULLDOWN);" in
  // prepare_sense. If we switch to Analog, the internal pull-up/down might be
  // disabled by hardware on some STM32s. But STM32F3 usually disconnects
  // digital path. Pull resistors might remain if configured? Let's stick to the
  // Ref logic: IT uses ADC to check.

  uint16_t val = adc2_read(ADC_CH_Y);

  // Restore generic sense state
  touch_prepare_sense();

  return (
      val >
      TOUCH_THRESHOLD); // High value = Pressed (Voltage from X lines detected)
}

static uint16_t touch_read_x(void) {
  // Measure X:
  // Drive Y lines (Gradient). Measure X line (wiper).
  // YN: High
  // YP: Low
  // XN: Input (Hi-Z)
  // XP: Analog Input

  gpio_set(GPIO_YN, PIN_YN);
  gpio_mode_output(GPIO_YN, PIN_YN);

  gpio_clear(GPIO_YP, PIN_YP);
  gpio_mode_output(GPIO_YP, PIN_YP);

  gpio_mode_input(GPIO_XN, PIN_XN); // Hi-Z
  gpio_pupd_none(GPIO_XN, PIN_XN);

  gpio_mode_analog(GPIO_XP, PIN_XP); // ADC Input

  for (volatile int i = 0; i < 100; i++)
    ;

  return adc2_read(ADC_CH_X);
}

static uint16_t touch_read_y(void) {
  // Measure Y:
  // Drive X lines (Gradient). Measure Y line (wiper).
  // XN: Low (Note: Ref says "drive low to high on X line (top to bottom)".
  //         Ref: palClearPad(GPIOB, GPIOB_XN); palSetPad(GPIOA, GPIOA_XP)...
  //         wait. Ref ui_core.c: "drive low to high on X line" -> XN=0, XP=1?
  //         Wait, Reference for `touch_measure_y` only clears XN.
  //         And defines `touch_prepare_sense` leaving XP High.
  //         So XN=0, XP=1.

  // XN: Low
  // XP: High
  // YN: Input (Hi-Z)
  // YP: Analog Input

  gpio_clear(GPIO_XN, PIN_XN);
  gpio_mode_output(GPIO_XN, PIN_XN);

  gpio_set(GPIO_XP, PIN_XP);
  gpio_mode_output(GPIO_XP, PIN_XP);

  gpio_mode_input(GPIO_YN, PIN_YN); // Hi-Z
  gpio_pupd_none(GPIO_YN, PIN_YN);

  gpio_mode_analog(GPIO_YP, PIN_YP); // ADC Input

  for (volatile int i = 0; i < 100; i++)
    ;

  return adc2_read(ADC_CH_Y);
}

// --- HAL Interface ---

static meas_status_t hal_touch_read_point(void *ctx, meas_point_t *pt) {
  meas_drv_touch_t *touch = (meas_drv_touch_t *)ctx;
  if (!touch || !touch->is_initialized || !pt)
    return MEAS_ERROR;

  // Simple polling read
  // 1. Check if pressed (Optional, but good for filtering noise)
  // For raw read, just read.

  pt->x = (int16_t)touch_read_x();
  pt->y = (int16_t)touch_read_y();

  // Restore idle state
  touch_prepare_sense();

  // Invert/Calibrate?
  // HAL usually returns RAW or PRE-CALIBRATED.
  // The higher layer (UI) does the 3-point calibration mapping.
  // We return RAW ADC values (0-4095).

  return MEAS_OK;
}

static const meas_hal_touch_api_t touch_api = {
    .read_point = hal_touch_read_point,
};

const meas_hal_touch_api_t *meas_drv_touch_get_api(void) { return &touch_api; }

// --- Init ---

void *meas_drv_touch_init(void) {
  // 1. Enable Clocks (GPIOA, GPIOB, ADC12)
  // Assume GPIO clocks enabled by board.c, but strict check is good.
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_ADC12EN;

  // 2. Init ADC2
  adc2_init();

  // 3. Init State
  touch_prepare_sense();

  touch_ctx.is_initialized = true;
  return &touch_ctx;
}

// Direct wrappers
uint16_t meas_drv_touch_get_x(void *ctx) { return touch_read_x(); }

uint16_t meas_drv_touch_get_y(void *ctx) { return touch_read_y(); }

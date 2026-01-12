/**
 * @file board.c
 * @brief STM32F303 Board Support Implementation.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 */

#include "drv_lcd.h"
#include "drv_sd.h"
#include "gpio_defaults.h"
#include "measlib/drivers/api.h"
#include "measlib/drivers/hal.h"
#include <stdint.h>
#include <string.h> // for memset

// Driver Init Prototypes (implemented in src/drivers/stm32f303/)
meas_hal_rx_api_t *meas_drv_adc_init(void);
meas_hal_synth_api_t *meas_drv_synth_init(void);
meas_hal_io_api_t *meas_drv_controls_init(void);
meas_hal_touch_api_t *meas_drv_touch_init(void);
meas_hal_wdg_api_t *meas_drv_wdg_init(void);
meas_hal_flash_api_t *meas_drv_flash_init(void);
meas_hal_link_api_t *meas_drv_usb_init(void);

// -- Hardware Registers (Minimal Definitions) --

#define PERIPH_BASE 0x40000000UL
#define AHBPERIPH_BASE (PERIPH_BASE + 0x00020000UL)
#define RCC_BASE (AHBPERIPH_BASE + 0x1000UL)
#define FLASH_R_BASE (AHBPERIPH_BASE + 0x2000UL)

typedef struct {
  volatile uint32_t ACR;
  volatile uint32_t KEYR;
  volatile uint32_t OPTKEYR;
  volatile uint32_t SR;
  volatile uint32_t CR;
  volatile uint32_t AR;
  volatile uint32_t OBR;
  volatile uint32_t WRPR;
} FLASH_TypeDef;

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

#define RCC ((RCC_TypeDef *)RCC_BASE)
#define FLASH ((FLASH_TypeDef *)FLASH_R_BASE)

// -- Register Bit Defs --
// RCC CR
#define RCC_CR_HSION (1U << 0)
#define RCC_CR_HSIRDY (1U << 1)
#define RCC_CR_HSEON (1U << 16)
#define RCC_CR_HSERDY (1U << 17)
#define RCC_CR_PLLON (1U << 24)
#define RCC_CR_PLLRDY (1U << 25)

// RCC CFGR
#define RCC_CFGR_SW_HSE (1U << 0)
#define RCC_CFGR_SW_PLL (2U << 0)
#define RCC_CFGR_SWS_HSE (1U << 2)
#define RCC_CFGR_SWS_PLL (2U << 2)
#define RCC_CFGR_HPRE_DIV1 (0U << 4)
#define RCC_CFGR_PPRE1_DIV2 (4U << 8)
#define RCC_CFGR_PPRE2_DIV1 (0U << 11)
#define RCC_CFGR_PLLSRC_HSE_PREDIV (1U << 16)
#define RCC_CFGR_PLLMUL9 (7U << 18)

// RCC CFGR2
#define RCC_CFGR2_PREDIV_DIV1 (0U << 0)

// FLASH ACR
#define FLASH_ACR_LATENCY_2 (2U << 0)
#define FLASH_ACR_PRFTBE (1U << 4)

// Linker Symbols
extern uint32_t _sccm;
extern uint32_t _eccm;

// GPIO Defs
#define GPIOA_BASE 0x48000000UL
#define GPIOB_BASE 0x48000400UL
#define GPIOC_BASE 0x48000800UL
#define GPIOD_BASE 0x48000C00UL
#define GPIOE_BASE 0x48001000UL
#define GPIOF_BASE 0x48001400UL

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

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOF ((GPIO_TypeDef *)GPIOF_BASE)

// RCC AHB
#define RCC_AHBENR_GPIOAEN (1UL << 17)
#define RCC_AHBENR_GPIOBEN (1UL << 18)
#define RCC_AHBENR_GPIOCEN (1UL << 19)
#define RCC_AHBENR_GPIODEN (1UL << 20)
#define RCC_AHBENR_GPIOFEN (1UL << 22)

// SysTick
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
 * System Core Clock
 */
uint32_t SystemCoreClock = 72000000;

/**
 * @brief  Setup the microcontroller system
 *         Initialize the Embedded Flash Interface, the PLL and update the
 *         SystemCoreClock variable.
 */
void SystemInit(void) {
  // 1. Reset RCC to default
  RCC->CR |= (uint32_t)0x00000001; // HSION
  RCC->CFGR = 0x00000000;
  RCC->CR &= (uint32_t)0xFEF6FFFF;
  RCC->CR &= (uint32_t)0xFFFBFFFF;
  RCC->CFGR &= (uint32_t)0xFF80FFFF;
  RCC->CIR = 0x00000000;

  // 2. Enable HSE
  RCC->CR |= RCC_CR_HSEON;
  // Wait for HSE Ready
  while ((RCC->CR & RCC_CR_HSERDY) == 0) {
  }

  // 3. Configure Flash Latency
  // 72MHz -> 2 Wait States
  FLASH->ACR |= FLASH_ACR_LATENCY_2 | FLASH_ACR_PRFTBE;

  // 4. Configure Buses
  // AHB = SYSCLK (72M)
  // APB1 = SYSCLK/2 (36M)
  // APB2 = SYSCLK (72M)
  RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;

  // 5. Configure PLL
  // Source = HSE (8M) / PREDIV(1)
  // Mul = 9 -> 72MHz
  RCC->CFGR2 = RCC_CFGR2_PREDIV_DIV1; // Div1
  RCC->CFGR |= RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR_PLLMUL9;

  // 6. Enable PLL
  RCC->CR |= RCC_CR_PLLON;
  while ((RCC->CR & RCC_CR_PLLRDY) == 0) {
  }

  // 7. Select PLL as System Clock
  RCC->CFGR &= (uint32_t)((void *)~(RCC_CFGR_SW_PLL));
  RCC->CFGR |= RCC_CFGR_SW_PLL;

  // Wait for Switch
  while ((RCC->CFGR & RCC_CFGR_SWS_PLL) != RCC_CFGR_SWS_PLL) {
  }

  SystemCoreClock = 72000000;
}

/**
 * @brief Initialize Systick
 * Called by main loop or drivers to setup 1ms tick
 */
void sys_tick_init(void) {
  // 72MHz / 1000 = 72000 ticks
  SysTick->LOAD = (SystemCoreClock / 1000) - 1;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |
                  SysTick_CTRL_ENABLE_Msk;
}

static void ccm_ram_init(void) {
  // Clear CCM RAM to zero
  uint32_t *p = &_sccm;
  while (p < &_eccm) {
    *p++ = 0;
  }
}

static void gpio_init_defaults(void) {
  // 1. Enable Clocks for Ports A-F
  RCC->AHBENR |= (RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN |
                  RCC_AHBENR_GPIODEN | RCC_AHBENR_GPIOFEN);

  // 2. Setup Defaults

  // GPIOA
  GPIOA->MODER = VAL_GPIOA_MODER;
  GPIOA->OTYPER = VAL_GPIOA_OTYPER;
  GPIOA->OSPEEDR = VAL_GPIOA_OSPEEDR;
  GPIOA->PUPDR = VAL_GPIOA_PUPDR;
  GPIOA->ODR = VAL_GPIOA_ODR;
  GPIOA->AFR[0] = VAL_GPIOA_AFRL;
  GPIOA->AFR[1] = VAL_GPIOA_AFRH;

  // GPIOB
  GPIOB->MODER = VAL_GPIOB_MODER;
  GPIOB->OTYPER = VAL_GPIOB_OTYPER;
  GPIOB->OSPEEDR = VAL_GPIOB_OSPEEDR;
  GPIOB->PUPDR = VAL_GPIOB_PUPDR;
  GPIOB->ODR = VAL_GPIOB_ODR;
  GPIOB->AFR[0] = VAL_GPIOB_AFRL;
  GPIOB->AFR[1] = VAL_GPIOB_AFRH;

  // GPIOC
  GPIOC->MODER = VAL_GPIOC_MODER;
  GPIOC->OTYPER = VAL_GPIOC_OTYPER;
  GPIOC->OSPEEDR = VAL_GPIOC_OSPEEDR;
  GPIOC->PUPDR = VAL_GPIOC_PUPDR;
  GPIOC->ODR = VAL_GPIOC_ODR;
  GPIOC->AFR[0] = VAL_GPIOC_AFRL;
  GPIOC->AFR[1] = VAL_GPIOC_AFRH;

  // GPIOD
  GPIOD->MODER = VAL_GPIOD_MODER;
  GPIOD->OTYPER = VAL_GPIOD_OTYPER;
  GPIOD->OSPEEDR = VAL_GPIOD_OSPEEDR;
  GPIOD->PUPDR = VAL_GPIOD_PUPDR;
  GPIOD->ODR = VAL_GPIOD_ODR;
  GPIOD->AFR[0] = VAL_GPIOD_AFRL;
  GPIOD->AFR[1] = VAL_GPIOD_AFRH;

  // GPIOF
  GPIOF->MODER = VAL_GPIOF_MODER;
  GPIOF->OTYPER = VAL_GPIOF_OTYPER;
  GPIOF->OSPEEDR = VAL_GPIOF_OSPEEDR;
  GPIOF->PUPDR = VAL_GPIOF_PUPDR;
  GPIOF->ODR = VAL_GPIOF_ODR;
  GPIOF->AFR[0] = VAL_GPIOF_AFRL;
  GPIOF->AFR[1] = VAL_GPIOF_AFRH;
}

/**
 * @brief System Initialization (Framework Hook)
 * Called from main loop to initialize platform drivers.
 */
void sys_init(void) {
  // For now, using default HSI (8MHz) or what Startup provides

  ccm_ram_init();
  gpio_init_defaults();
  sys_tick_init();

  meas_drv_adc_init();
  meas_drv_synth_init();
  meas_drv_controls_init();
  meas_drv_touch_init();
  meas_drv_wdg_init();
  meas_drv_flash_init();
  meas_drv_usb_init(); // Restored USB Init

  meas_drv_sd_init();
  meas_drv_lcd_init();

  // 2. Event Loop Init
  // meas_event_loop_init();
}

volatile uint32_t sys_tick_counter = 0;

void SysTick_Handler(void) { sys_tick_counter++; }

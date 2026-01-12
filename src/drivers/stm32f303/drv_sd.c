/**
 * @file drv_sd.c
 * @brief STM32F303 SD Card Driver Implementation (SPI Mode).
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Details:
 * - Uses SPI1 (Shared with LCD).
 * - Pins:
 *   - PB3: SCK
 *   - PB4: MISO
 *   - PB5: MOSI
 *   - PB11: CS (Active Low)
 */

#include "drv_sd.h"
#include "measlib/drivers/hal.h"
#include <stdbool.h>
#include <stdint.h>

// --- Hardware Definitions (Bare Metal) ---

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
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t SR;
  volatile uint32_t DR;
  volatile uint32_t CRCPR;
  volatile uint32_t RXCRCR;
  volatile uint32_t TXCRCR;
  volatile uint32_t I2SCFGR;
  volatile uint32_t I2SPR;
} SPI_TypeDef;

typedef struct {
  volatile uint32_t CR;
  volatile uint32_t CFGR;
  volatile uint32_t CIR;
  volatile uint32_t APB2RSTR;
  volatile uint32_t APB1RSTR;
  volatile uint32_t AHBENR;
  volatile uint32_t APB2ENR;
  volatile uint32_t APB1ENR;
} RCC_TypeDef;

#define RCC_ADDR 0x40021000UL
#define GPIOB_BASE (0x48000400UL) // AHB2
#define SPI1_BASE (0x40013000UL)  // APB2

#define RCC ((RCC_TypeDef *)RCC_ADDR)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define SPI1 ((SPI_TypeDef *)SPI1_BASE)

#define RCC_AHBENR_GPIOBEN (1UL << 18)
#define RCC_APB2ENR_SPI1EN (1UL << 12)

// SPI Register Bits
#define SPI_CR1_CPHA (1UL << 0)
#define SPI_CR1_CPOL (1UL << 1)
#define SPI_CR1_MSTR (1UL << 2)
#define SPI_CR1_BR_0 (1UL << 3)
#define SPI_CR1_BR_1 (1UL << 4)
#define SPI_CR1_BR_2 (1UL << 5)
#define SPI_CR1_SPE (1UL << 6)
#define SPI_CR1_SSI (1UL << 8)
#define SPI_CR1_SSM (1UL << 9)

#define SPI_CR2_DS_0 (1UL << 8)
#define SPI_CR2_DS_1 (1UL << 9)
#define SPI_CR2_DS_2 (1UL << 10)
#define SPI_CR2_FRXTH (1UL << 12)

#define SPI_SR_RXNE (1UL << 0)
#define SPI_SR_TXE (1UL << 1)
#define SPI_SR_BSY (1UL << 7)

// Pin Defs
#define SD_CS_PIN 11 // PB11

#define SD_SPI SPI1

// SPI Commands
#define CMD0 (0)           // GO_IDLE_STATE
#define CMD1 (1)           // SEND_OP_COND (MMC)
#define ACMD41 (41 | 0x80) // SEND_OP_COND (SDC)
#define CMD8 (8)           // SEND_IF_COND
#define CMD9 (9)           // SEND_CSD
#define CMD10 (10)         // SEND_CID
#define CMD12 (12)         // STOP_TRANSMISSION
#define ACMD13 (13 | 0x80) // SD_STATUS (SDC)
#define CMD16 (16)         // SET_BLOCKLEN
#define CMD17 (17)         // READ_SINGLE_BLOCK
#define CMD18 (18)         // READ_MULTIPLE_BLOCK
#define CMD23 (23)         // SET_BLOCK_COUNT (MMC)
#define ACMD23 (23 | 0x80) // SET_WR_BLK_ERASE_COUNT (SDC)
#define CMD24 (24)         // WRITE_BLOCK
#define CMD25 (25)         // WRITE_MULTIPLE_BLOCK
#define CMD55 (55)         // APP_CMD
#define CMD58 (58)         // READ_OCR

// --- Context Definition ---

typedef struct {
  bool initialized;
  uint8_t card_type; // 0:Unknown, 1:MMC, 2:v1, 4:v2, 8:Block
  uint32_t sector_count;
} meas_drv_sd_t;

static meas_drv_sd_t sd_ctx;

// --- Hardware Helper Functions ---

static void sd_select(void) {
  GPIOB->BSRR = (1UL << (SD_CS_PIN + 16)); // Reset (Low)
}

static void sd_deselect(void) {
  GPIOB->BSRR = (1UL << SD_CS_PIN); // Set (High)
}

static uint8_t sd_spi_transfer(uint8_t data) {
  while (!(SD_SPI->SR & SPI_SR_TXE))
    ;
  *(volatile uint8_t *)&SD_SPI->DR = data;
  while (!(SD_SPI->SR & SPI_SR_RXNE))
    ;
  return *(volatile uint8_t *)&SD_SPI->DR;
}

static uint8_t sd_spi_receive(void) { return sd_spi_transfer(0xFF); }

static void sd_spi_init(bool fast) {
  // Disable
  SD_SPI->CR1 &= ~SPI_CR1_SPE;

  // Master, SSI, SSM
  uint32_t cr1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;

  if (fast) {
    // PCLK/4 (~18MHz)
    cr1 |= SPI_CR1_BR_0;
  } else {
    // PCLK/256 (~281kHz)
    cr1 |= (SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2);
  }

  // 8-bit Data (0111 = 7)
  uint32_t cr2 = (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2) | SPI_CR2_FRXTH;

  SD_SPI->CR1 = cr1;
  SD_SPI->CR2 = cr2;
  SD_SPI->CR1 |= SPI_CR1_SPE;
}

// Helper: Restore SPI Config for Data Transfer (8-bit, High Speed)
// Must be called before any data transfer to recover from Shared Bus (LCD)
// changes.
static void sd_spi_config_fast(void) {
  // Check if we need to reconfig? Just DO IT to be safe and atomic.
  SD_SPI->CR1 &= ~SPI_CR1_SPE;

  // Master, SSI, SSM, PCLK/4 (High Speed)
  uint32_t cr1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_0;

  // 8-bit Data
  uint32_t cr2 = (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2) | SPI_CR2_FRXTH;

  SD_SPI->CR1 = cr1;
  SD_SPI->CR2 = cr2;
  SD_SPI->CR1 |= SPI_CR1_SPE;
}

// --- Protocol Helper Functions ---

static uint8_t sd_wait_ready(void) {
  uint8_t res;
  uint16_t timeout = 5000;
  do {
    res = sd_spi_receive();
    if (res == 0xFF)
      return 0xFF;
    for (volatile int i = 0; i < 100; i++)
      ;
  } while (--timeout);
  return res;
}

static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg) {
  uint8_t n, res;

  if (cmd & 0x80) {
    cmd &= 0x7F;
    res = sd_send_cmd(CMD55, 0);
    if (res > 1)
      return res;
  }

  sd_deselect();
  sd_select();
  sd_wait_ready();

  sd_spi_transfer(0x40 | cmd);
  sd_spi_transfer((arg >> 24) & 0xFF);
  sd_spi_transfer((arg >> 16) & 0xFF);
  sd_spi_transfer((arg >> 8) & 0xFF);
  sd_spi_transfer(arg & 0xFF);

  n = 0x01;
  if (cmd == CMD0)
    n = 0x95;
  if (cmd == CMD8)
    n = 0x87;
  sd_spi_transfer(n);

  if (cmd == CMD12)
    sd_spi_receive();

  n = 10;
  do {
    res = sd_spi_receive();
  } while ((res & 0x80) && --n);

  return res;
}

// --- Public API Implementation (Internal but exposed via HAL) ---

meas_sd_status_t meas_drv_sd_read_blocks(void *ctx, uint32_t sector,
                                         uint8_t *buffer, uint32_t count) {
  meas_drv_sd_t *sd = (meas_drv_sd_t *)ctx;
  if (!sd || !sd->initialized)
    return MEAS_SD_NO_INIT;
  if (!(sd->card_type & 8))
    sector *= 512;

  sd_select();
  // Restore proper SPI Config (Shared Bus Safety)
  sd_spi_config_fast();

  if (count == 1) {
    if (sd_send_cmd(CMD17, sector) == 0) {
      uint8_t token;
      uint32_t to = 20000;
      do {
        token = sd_spi_receive();
      } while ((token == 0xFF) && --to);
      if (token != 0xFE) {
        sd_deselect();
        return MEAS_SD_TIMEOUT;
      }
      for (int i = 0; i < 512; i++)
        *buffer++ = sd_spi_receive();
      sd_spi_receive();
      sd_spi_receive();
    } else {
      sd_deselect();
      return MEAS_SD_ERROR;
    }
  } else {
    if (sd_send_cmd(CMD18, sector) == 0) {
      do {
        uint8_t token;
        uint32_t to = 20000;
        do {
          token = sd_spi_receive();
        } while ((token == 0xFF) && --to);
        if (token != 0xFE) {
          sd_deselect();
          return MEAS_SD_TIMEOUT;
        }
        for (int i = 0; i < 512; i++)
          *buffer++ = sd_spi_receive();
        sd_spi_receive();
        sd_spi_receive();
      } while (--count);
      sd_send_cmd(CMD12, 0);
    } else {
      sd_deselect();
      return MEAS_SD_ERROR;
    }
  }
  sd_deselect();
  sd_spi_receive();
  return MEAS_SD_OK;
}

meas_sd_status_t meas_drv_sd_write_blocks(void *ctx, uint32_t sector,
                                          const uint8_t *buffer,
                                          uint32_t count) {
  meas_drv_sd_t *sd = (meas_drv_sd_t *)ctx;
  if (!sd || !sd->initialized)
    return MEAS_SD_NO_INIT;
  if (!(sd->card_type & 8))
    sector *= 512;

  sd_select();
  // Restore proper SPI Config (Shared Bus Safety)
  sd_spi_config_fast();

  if (count == 1) {
    if (sd_send_cmd(CMD24, sector) == 0) {
      sd_spi_transfer(0xFF);
      sd_spi_transfer(0xFF);
      sd_spi_transfer(0xFE);
      for (int i = 0; i < 512; i++)
        sd_spi_transfer(*buffer++);
      sd_spi_transfer(0xFF);
      sd_spi_transfer(0xFF);
      if ((sd_spi_receive() & 0x1F) != 0x05) {
        sd_deselect();
        return MEAS_SD_WRITE_ERROR;
      }
      while (sd_spi_receive() == 0)
        ;
    } else {
      sd_deselect();
      return MEAS_SD_ERROR;
    }
  } else {
    if (sd_send_cmd(CMD25, sector) == 0) {
      sd_spi_transfer(0xFF);
      sd_spi_transfer(0xFF);
      do {
        sd_spi_transfer(0xFC);
        for (int i = 0; i < 512; i++)
          sd_spi_transfer(*buffer++);
        sd_spi_transfer(0xFF);
        sd_spi_transfer(0xFF);
        if ((sd_spi_receive() & 0x1F) != 0x05) {
          sd_deselect();
          return MEAS_SD_WRITE_ERROR;
        }
        while (sd_spi_receive() == 0)
          ;
      } while (--count);
      sd_spi_transfer(0xFD);
      while (sd_spi_receive() == 0)
        ;
    } else {
      sd_deselect();
      return MEAS_SD_ERROR;
    }
  }
  sd_deselect();
  sd_spi_receive();
  return MEAS_SD_OK;
}

uint32_t meas_drv_sd_get_sector_count(void) { return sd_ctx.sector_count; }

int meas_drv_sd_is_initialized(void) { return sd_ctx.initialized; }

// --- HAL Interface Wrappers ---

static meas_status_t hal_sd_read(void *ctx, uint32_t sector, void *buffer,
                                 uint32_t count) {
  if (ctx != &sd_ctx)
    return MEAS_ERROR;
  meas_sd_status_t res =
      meas_drv_sd_read_blocks(ctx, sector, (uint8_t *)buffer, count);
  return (res == MEAS_SD_OK) ? MEAS_OK : MEAS_ERROR;
}

static meas_status_t hal_sd_write(void *ctx, uint32_t sector,
                                  const void *buffer, uint32_t count) {
  if (ctx != &sd_ctx)
    return MEAS_ERROR;
  meas_sd_status_t res =
      meas_drv_sd_write_blocks(ctx, sector, (const uint8_t *)buffer, count);
  return (res == MEAS_SD_OK) ? MEAS_OK : MEAS_ERROR;
}

static uint32_t hal_sd_get_capacity(void *ctx) {
  if (ctx != &sd_ctx)
    return 0;
  return meas_drv_sd_get_sector_count();
}

static bool hal_sd_is_ready(void *ctx) {
  if (ctx != &sd_ctx)
    return false;
  meas_drv_sd_t *sd = (meas_drv_sd_t *)ctx;
  return sd->initialized;
}

static const meas_hal_storage_api_t sd_api = {
    .read = hal_sd_read,
    .write = hal_sd_write,
    .get_capacity = hal_sd_get_capacity,
    .is_ready = hal_sd_is_ready,
};

const meas_hal_storage_api_t *meas_drv_sd_get_api(void) { return &sd_api; }

void *meas_drv_sd_init(void) {
  // 1. GPIO Init
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

  sd_ctx.initialized = false;
  sd_ctx.card_type = 0;
  sd_ctx.sector_count = 0;

  GPIOB->MODER &= ~((3UL << (3 * 2)) | (3UL << (4 * 2)) | (3UL << (5 * 2)) |
                    (3UL << (11 * 2)));
  GPIOB->MODER |= ((2UL << (3 * 2)) | (2UL << (4 * 2)) | (2UL << (5 * 2)) |
                   (1UL << (11 * 2)));

  GPIOB->AFR[0] &= ~((0xFUL << 12) | (0xFUL << 16) | (0xFUL << 20));
  GPIOB->AFR[0] |= ((5UL << 12) | (5UL << 16) | (5UL << 20));

  sd_spi_init(false);
  sd_deselect();

  for (volatile int i = 0; i < 10000; i++)
    ;

  for (int i = 0; i < 10; i++)
    sd_spi_transfer(0xFF);

  // 4. Initialization
  sd_select();
  uint8_t n, cmd, ty, ocr[4];

  // CDM0 Retry Loop
  int i = 0;
  bool cmd0_ok = false;
  while (i++ < 10) {
    if (sd_send_cmd(CMD0, 0) == 1) { // 1 = Idle State
      cmd0_ok = true;
      break;
    }
    sd_deselect();
    for (volatile int d = 0; d < 10000; d++)
      ; // ~10ms delay substitute
    sd_select();
  }

  if (cmd0_ok) {
    if (sd_send_cmd(CMD8, 0x1AA) == 1) {
      for (n = 0; n < 4; n++)
        ocr[n] = sd_spi_receive();
      if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
        int retry = 10000;
        while (retry-- && sd_send_cmd(ACMD41, 1UL << 30))
          ;
        if (retry && sd_send_cmd(CMD58, 0) == 0) {
          for (n = 0; n < 4; n++)
            ocr[n] = sd_spi_receive();
          ty = (ocr[0] & 0x40) ? 12 : 4;
        }
      }
    } else {
      if (sd_send_cmd(ACMD41, 0) <= 1) {
        ty = 2;
        cmd = ACMD41;
      } else {
        ty = 1;
        cmd = CMD1;
      }
      int retry = 10000;
      while (retry-- && sd_send_cmd(cmd, 0))
        ;
      if (!retry || sd_send_cmd(CMD16, 512) != 0)
        ty = 0;
    }
  }
  sd_ctx.card_type = ty;
  sd_deselect();

  if (ty) {
    sd_spi_init(true);
    sd_ctx.initialized = true;
    return &sd_ctx;
  }

  return NULL;
}

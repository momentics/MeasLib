/**
 * @file drv_lcd.c
 * @brief Bare-metal LCD Driver for STM32F303
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Implements support for ILI9341 and ST7789V controllers using SPI1 and DMA.
 * Handles SPI bus locking/unlocking to coexist with SD Card driver.
 */

#include "drv_lcd.h"
#include "measlib/drivers/hal.h"
#include <stddef.h>

// --- Configuration ---

// Pin Definitions (Matches board.h)
// PB6 = CS, PB7 = CD (DC), PA15 = RST
#define LCD_CS_PIN 6
#define LCD_CD_PIN 7
#define LCD_RST_PIN 15

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

typedef struct {
  volatile uint32_t CCR;
  volatile uint32_t CNDTR;
  volatile uint32_t CPAR;
  volatile uint32_t CMAR;
  volatile uint32_t RES;
} DMA_Channel_TypeDef;

typedef struct {
  volatile uint32_t ISR;
  volatile uint32_t IFCR;
  DMA_Channel_TypeDef Channel[7];
} DMA_TypeDef;

#define SPI1 ((SPI_TypeDef *)0x40013000UL)
#define GPIOB ((GPIO_TypeDef *)0x48000400UL)
#define GPIOA ((GPIO_TypeDef *)0x48000000UL)
#define RCC ((RCC_TypeDef *)0x40021000UL)
#define DMA1 ((DMA_TypeDef *)0x40020000UL)
#define DMA1_Channel3 ((DMA_Channel_TypeDef *)0x40020030UL)

#define RCC_AHBENR_DMA1EN (1UL << 0)
#define RCC_AHBENR_GPIOAEN (1UL << 17)
#define RCC_AHBENR_GPIOBEN (1UL << 18)
#define RCC_APB2ENR_SPI1EN (1UL << 12)

#define SPI_CR1_CPHA (1UL << 0)
#define SPI_CR1_CPOL (1UL << 1)
#define SPI_CR1_MSTR (1UL << 2)
#define SPI_CR1_SPE (1UL << 6)
#define SPI_CR1_SSI (1UL << 8)
#define SPI_CR1_SSM (1UL << 9)

#define SPI_CR2_RXDMAEN (1UL << 0)
#define SPI_CR2_TXDMAEN (1UL << 1)
#define SPI_CR2_FRXTH (1UL << 12)

#define SPI_SR_RXNE (1UL << 0)
#define SPI_SR_TXE (1UL << 1)
#define SPI_SR_BSY (1UL << 7)

#define DMA_CCR_EN (1UL << 0)
#define DMA_CCR_DIR (1UL << 4)
#define DMA_CCR_MINC (1UL << 7)
#define DMA_CCR_PSIZE_0 (1UL << 8)
#define DMA_CCR_MSIZE_0 (1UL << 10)
#define DMA_CCR_PL_1 (1UL << 13)

#define DMA_ISR_TCIF3 (1UL << 9)
#define DMA_IFCR_CGIF3 (1UL << 9)

#define LCD_CS_PORT GPIOB
#define LCD_CD_PORT GPIOB
#define LCD_RST_PORT GPIOA

// Control Macros
#define LCD_CS_LOW() (LCD_CS_PORT->BSRR = (1UL << (LCD_CS_PIN + 16)))
#define LCD_CS_HIGH() (LCD_CS_PORT->BSRR = (1UL << LCD_CS_PIN))
#define LCD_CD_CMD() (LCD_CD_PORT->BSRR = (1UL << (LCD_CD_PIN + 16)))
#define LCD_CD_DATA() (LCD_CD_PORT->BSRR = (1UL << LCD_CD_PIN))
#define LCD_RST_LOW() (LCD_RST_PORT->BSRR = (1UL << (LCD_RST_PIN + 16)))
#define LCD_RST_HIGH() (LCD_RST_PORT->BSRR = (1UL << LCD_RST_PIN))

// Commands
#define CMD_SWRESET 0x01
#define CMD_RDDID 0x04
#define CMD_SLPOUT 0x11
#define CMD_DISPON 0x29
#define CMD_CASET 0x2A
#define CMD_RASET 0x2B
#define CMD_RAMWR 0x2C
#define CMD_MADCTL 0x36
#define CMD_COLMOD 0x3A

// Controller IDs
#define ID_ST7789V 0x858552
#define ID_ILI9341                                                             \
  0x000000 // ILI9341 often returns 0 or garbage on generic module

// --- Context Definition ---

typedef struct {
  uint16_t width;
  uint16_t height;
  bool is_initialized;
} meas_drv_lcd_t;

static meas_drv_lcd_t lcd_ctx;

// --- Helper Functions ---
static bool is_st7789 = false;

// --- Low-Level SPI ---

/**
 * @brief Configure SPI1 for LCD operation (High Speed, Master).
 * Using 8-bit datasize initially.
 */
static void lcd_spi_config_8bit(void) {
  // Disable SPI to reconfigure
  SPI1->CR1 &= ~SPI_CR1_SPE;

  // Clear relevant bits
  SPI1->CR1 = 0;
  SPI1->CR2 = 0;

  // Config: Master, SSM=1, SSI=1, Baud=Div2 (High Speed), CPOL=0, CPHA=0
  // Note: 8-bit data size is handled by FRXTH/DS default (0x7 = 8bit) in CR2 if
  // not touched? Actually on F303, DS[3:0] in CR2 controls data size. 0111 =
  // 8-bit.
  SPI1->CR2 = (7UL << 8) | SPI_CR2_FRXTH; // 8-bit, RXNE at 1/4 (8bit)

  SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (0UL << 3); // Div2

  // Enable
  SPI1->CR1 |= SPI_CR1_SPE;
}

/**
 * @brief Configure SPI1 for 16-bit operation (High Speed, Master).
 * Used for pixel data transfer.
 */
static void lcd_spi_config_16bit(void) {
  SPI1->CR1 &= ~SPI_CR1_SPE;
  SPI1->CR1 = 0;
  SPI1->CR2 = 0;

  // DS[3:0] = 1111 (15) -> 16-bit.
  SPI1->CR2 = (15UL << 8) | SPI_CR2_FRXTH;

  SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (0UL << 3); // Div2

  SPI1->CR1 |= SPI_CR1_SPE;
}

static inline void spi_write8(uint8_t data) {
  // Wait TXE
  while (!(SPI1->SR & SPI_SR_TXE))
    ;
  // Write DR (cast to uint8_t force 8-bit access)
  *(volatile uint8_t *)&SPI1->DR = data;
}

static inline uint8_t spi_transfer8(uint8_t data) {
  while (!(SPI1->SR & SPI_SR_TXE))
    ;
  *(volatile uint8_t *)&SPI1->DR = data;
  while (!(SPI1->SR & SPI_SR_RXNE))
    ;
  return *(volatile uint8_t *)&SPI1->DR;
}

/**
 * @brief Flush SPI Wait for Busy flag to clear.
 */
static void spi_wait_busy(void) {
  while (SPI1->SR & SPI_SR_BSY)
    ;
}

// --- Driver Primitives ---

static void lcd_write_cmd(uint8_t cmd) {
  LCD_CD_CMD();
  spi_write8(cmd);
  spi_wait_busy(); // Wait before switching DC
  LCD_CD_DATA();
}

static void lcd_write_data8(uint8_t data) { spi_write8(data); }

static void lcd_write_data32(uint32_t data) {
  spi_write8((data >> 24) & 0xFF);
  spi_write8((data >> 16) & 0xFF);
  spi_write8((data >> 8) & 0xFF);
  spi_write8(data & 0xFF);
}

// --- Initialization Logic ---

static void lcd_hard_reset(void) {
  LCD_RST_LOW();
  for (volatile int i = 0; i < 100000; i++)
    ; // ~5ms
  LCD_RST_HIGH();
  for (volatile int i = 0; i < 100000; i++)
    ; // ~5ms
}

static uint32_t lcd_read_id(void) {
  uint32_t id = 0;
  // Standard RDID command
  lcd_write_cmd(CMD_RDDID);

  // Read 4 bytes (Dummy, ID1, ID2, ID3)
  spi_transfer8(0xFF); // Dummy
  id |= (uint32_t)spi_transfer8(0xFF) << 16;
  id |= (uint32_t)spi_transfer8(0xFF) << 8;
  id |= (uint32_t)spi_transfer8(0xFF);

  return id;
}

// Init Sequences (Ported from Reference)
static const uint8_t init_seq_st7789[] = {
    CMD_SWRESET, 0, CMD_SLPOUT, 0, CMD_COLMOD, 1, 0x55, // 16-bit
    CMD_MADCTL,  1, 0x60,                               // Landscape (MX|MV)
    CMD_DISPON,  0, 0                                   // End
};

static const uint8_t init_seq_ili9341[] = {
    CMD_SWRESET, 0,    0xCB,       5,    0x39, 0x2C, 0x00, 0x34, 0x02, 0xCF,
    3,           0x00, 0xC1,       0x30, 0xE8, 3,    0x85, 0x00, 0x78, 0xEA,
    2,           0x00, 0x00,       0xED, 4,    0x64, 0x03, 0x12, 0x81, 0xF7,
    1,           0x20, 0xC0,       1,    0x23, 0xC1, 1,    0x10, 0xC5, 2,
    0x3E,        0x28, 0xC7,       1,    0x86, 0x36, 1,    0x48, 0x3A, 1,
    0x55,        0xB1, 2,          0x00, 0x18, 0xB6, 3,    0x08, 0x82, 0x27,
    0xF2,        1,    0x00,       0x26, 1,    0x01, 0xE0, 15,   0x0F, 0x31,
    0x2B,        0x0C, 0x0E,       0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03,
    0x0E,        0x09, 0x00,       0xE1, 15,   0x00, 0x0E, 0x14, 0x03, 0x11,
    0x07,        0x31, 0xC1,       0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
    CMD_SLPOUT,  0,    CMD_DISPON, 0,    0};

static void lcd_run_seq(const uint8_t *seq) {
  while (*seq) {
    uint8_t cmd = *seq++;
    uint8_t len = *seq++;
    lcd_write_cmd(cmd);
    while (len--) {
      lcd_write_data8(*seq++);
    }
    // Small delay between commands often helps
    for (volatile int i = 0; i < 1000; i++)
      ;
  }
}

// --- Public API ---

// Helper: Blocking Wait for DMA
static void lcd_dma_wait(void) {
  while (!(DMA1->ISR & DMA_ISR_TCIF3))
    ;
  DMA1->IFCR = DMA_IFCR_CGIF3;       // Clear flags
  DMA1_Channel3->CCR &= ~DMA_CCR_EN; // Disable
}

// --- High-Level Drawing API (Context Aware) ---

void meas_drv_lcd_set_window(void *ctx, uint16_t x, uint16_t y, uint16_t w,
                             uint16_t h) {
  (void)ctx; // Validated by caller or ignored if singleton logic persists
             // But for purity we should check initialization?
             // static vars are still used for SPI/GPIO which are SINGLETON
             // HARDWARE. So ctx is mostly for resolving width/height state.

  meas_drv_lcd_t *lcd = (meas_drv_lcd_t *)ctx;
  if (!lcd || !lcd->is_initialized)
    return;

  LCD_CS_LOW();

  lcd_spi_config_8bit();
  lcd_write_cmd(CMD_CASET);
  lcd_write_data32((x << 16) | (x + w - 1));
  lcd_write_cmd(CMD_RASET);
  lcd_write_data32((y << 16) | (y + h - 1));
  lcd_write_cmd(CMD_RAMWR);
  LCD_CS_HIGH();
}

void meas_drv_lcd_fill_rect(void *ctx, uint16_t x, uint16_t y, uint16_t w,
                            uint16_t h, uint16_t color) {
  meas_drv_lcd_t *lcd = (meas_drv_lcd_t *)ctx;
  if (!lcd || !lcd->is_initialized)
    return;

  uint32_t total_pixels = (uint32_t)w * h;
  if (total_pixels == 0)
    return;
  LCD_CS_LOW();

  // 2. Set Window
  meas_drv_lcd_set_window(ctx, x, y, w, h);

  // 3. Switch to 16-bit SPI for pixel data
  spi_wait_busy();
  lcd_spi_config_16bit();
  // Note: CS stays LOW during reconfig if we are careful, or we toggle it?
  // Safe to keeping low as long as CLK doesn't toggle.

  // 4. Setup DMA for FILL
  // Mem2Periph, 16-bit (MSIZE=01, PSIZE=01), MINC=0 (Fixed Color), Priority
  // High
  DMA1_Channel3->CCR = DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_DIR |
                       DMA_CCR_PL_1; // No MINC!
  DMA1_Channel3->CNDTR = total_pixels;
  DMA1_Channel3->CMAR = (uint32_t)&color;

  // 5. Enable DMA Request on SPI
  SPI1->CR2 |= SPI_CR2_TXDMAEN;

  // 6. Start DMA
  DMA1_Channel3->CCR |= DMA_CCR_EN;

  // 7. Wait Completion (Blocking for safety)
  lcd_dma_wait();

  // 8. Cleanup
  SPI1->CR2 &= ~SPI_CR2_TXDMAEN;
  spi_wait_busy(); // Wait used SPI FIFO to drain
  while (SPI1->SR & SPI_SR_BSY)
    ;

  LCD_CS_HIGH();
}

void meas_drv_lcd_blit(void *ctx, uint16_t x, uint16_t y, uint16_t w,
                       uint16_t h, const uint16_t *pixels) {
  meas_drv_lcd_t *lcd = (meas_drv_lcd_t *)ctx;
  if (!lcd || !lcd->is_initialized)
    return;

  uint32_t total_pixels = (uint32_t)w * h;
  if (total_pixels == 0)
    return;

  meas_drv_lcd_set_window(ctx, x, y, w, h);

  spi_wait_busy();
  lcd_spi_config_16bit();

  LCD_CS_LOW();
  lcd_write_cmd(CMD_RAMWR);
  LCD_CD_DATA(); // Data Mode

  // DMA for BLIT (MINC=1)
  DMA1_Channel3->CCR = DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_DIR |
                       DMA_CCR_PL_1 | DMA_CCR_MINC; // MINC Enabled
  DMA1_Channel3->CNDTR = total_pixels;
  DMA1_Channel3->CMAR = (uint32_t)pixels;

  SPI1->CR2 |= SPI_CR2_TXDMAEN;
  DMA1_Channel3->CCR |= DMA_CCR_EN;

  lcd_dma_wait();

  SPI1->CR2 &= ~SPI_CR2_TXDMAEN;
  while (SPI1->SR & SPI_SR_BSY)
    ;

  LCD_CS_HIGH();
}

void meas_drv_lcd_set_orientation(void *ctx, uint8_t rotation, bool bgr_order) {
  meas_drv_lcd_t *lcd = (meas_drv_lcd_t *)ctx;
  if (!lcd || !lcd->is_initialized)
    return;

  uint8_t madctl = 0;

  // Standard rotation mapping (adjust as needed for specific panel)
  switch (rotation % 4) {
  case 0:
    madctl = 0x40 | 0x80;
    break; // MX | MY
  case 1:
    madctl = 0x20 | 0x80;
    break; // MV | MY
  case 2:
    madctl = 0x00;
    break; // Normal
  case 3:
    madctl = 0x20 | 0x40;
    break; // MV | MX
  }

  if (bgr_order) {
    madctl |= 0x08; // BGR Bit
  }

  // Let's implement dynamic swap based on ST7789/ILI9341 behavior
  // ST7789 240x320.
  if (rotation % 2 != 0) {
    lcd->width = 320;
    lcd->height = 240;
  } else {
    lcd->width = 240;
    lcd->height = 320;
  }

  lcd_spi_config_8bit();
  LCD_CS_LOW();
  lcd_write_cmd(CMD_MADCTL);
  lcd_write_data8(madctl);
  LCD_CS_HIGH();
}

uint16_t meas_drv_lcd_get_width(void *ctx) {
  meas_drv_lcd_t *lcd = (meas_drv_lcd_t *)ctx;
  if (!lcd)
    return 0;
  return lcd->width;
}
uint16_t meas_drv_lcd_get_height(void *ctx) {
  meas_drv_lcd_t *lcd = (meas_drv_lcd_t *)ctx;
  if (!lcd)
    return 0;
  return lcd->height;
}

// --- HAL Interface Wrappers ---

static meas_status_t hal_lcd_set_window(void *ctx, uint16_t x, uint16_t y,
                                        uint16_t w, uint16_t h) {
  if (ctx != &lcd_ctx)
    return MEAS_ERROR;
  meas_drv_lcd_set_window(ctx, x, y, w, h);
  return MEAS_OK;
}

static meas_status_t hal_lcd_fill_rect(void *ctx, uint16_t x, uint16_t y,
                                       uint16_t w, uint16_t h, uint16_t color) {
  if (ctx != &lcd_ctx)
    return MEAS_ERROR;
  meas_drv_lcd_fill_rect(ctx, x, y, w, h, color);
  return MEAS_OK;
}

static meas_status_t hal_lcd_blit(void *ctx, uint16_t x, uint16_t y, uint16_t w,
                                  uint16_t h, const void *pixels) {
  if (ctx != &lcd_ctx)
    return MEAS_ERROR;
  meas_drv_lcd_blit(ctx, x, y, w, h, (const uint16_t *)pixels);
  return MEAS_OK;
}

static meas_status_t hal_lcd_set_orientation(void *ctx, uint8_t rotation,
                                             bool bgr) {
  if (ctx != &lcd_ctx)
    return MEAS_ERROR;
  meas_drv_lcd_set_orientation(ctx, rotation, bgr);
  return MEAS_OK;
}

static uint16_t hal_lcd_get_width(void *ctx) {
  if (ctx != &lcd_ctx)
    return 0;
  return meas_drv_lcd_get_width(ctx);
}

static uint16_t hal_lcd_get_height(void *ctx) {
  if (ctx != &lcd_ctx)
    return 0;
  return meas_drv_lcd_get_height(ctx);
}

static const meas_hal_display_api_t lcd_api = {
    .set_window = hal_lcd_set_window,
    .fill_rect = hal_lcd_fill_rect,
    .blit = hal_lcd_blit,
    .set_orientation = hal_lcd_set_orientation,
    .get_width = hal_lcd_get_width,
    .get_height = hal_lcd_get_height,
};

const meas_hal_display_api_t *meas_drv_lcd_get_api(void) { return &lcd_api; }

// Helper functions for init sequence and reset, extracted from
// meas_drv_lcd_init
static void meas_drv_lcd_reset(void) {
  lcd_hard_reset();
  LCD_CS_LOW();
}

static void meas_drv_lcd_init_seq(void) {
  uint32_t id = lcd_read_id();

  if (id == ID_ST7789V || id == 0x858552) { // 0x858552 is ST7789
    is_st7789 = true;
    lcd_run_seq(init_seq_st7789);
  } else {
    // Fallback to ILI9341
    is_st7789 = false;
    lcd_run_seq(init_seq_ili9341);
  }
}

void *meas_drv_lcd_init(void) {
  // 1. Enable Clocks
  RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOAEN | RCC_AHBENR_DMA1EN;
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

  // 2. Configure GPIO
  // CS(PB6), CD(PB7) -> Output
  GPIOB->MODER &= ~((3UL << (6 * 2)) | (3UL << (7 * 2)));
  GPIOB->MODER |= ((1UL << (6 * 2)) | (1UL << (7 * 2)));

  // SPI(PB3,4,5) -> AF5
  GPIOB->MODER &= ~((3UL << (3 * 2)) | (3UL << (4 * 2)) | (3UL << (5 * 2)));
  GPIOB->MODER |= ((2UL << (3 * 2)) | (2UL << (4 * 2)) | (2UL << (5 * 2)));
  GPIOB->AFR[0] &= ~((0xFUL << 12) | (0xFUL << 16) | (0xFUL << 20));
  GPIOB->AFR[0] |= ((5UL << 12) | (5UL << 16) | (5UL << 20));

  // RST(PA15) -> Output
  GPIOA->MODER &= ~(3UL << (15 * 2));
  GPIOA->MODER |= (1UL << (15 * 2));

  LCD_CS_HIGH();
  LCD_RST_HIGH();

  // 3. SPI Config
  lcd_spi_config_8bit();

  // 4. Reset & Detection
  meas_drv_lcd_reset();
  meas_drv_lcd_init_seq();

  LCD_CS_HIGH();

  // 5. Init Sequence
  LCD_CS_LOW();
  lcd_run_seq(init_seq_st7789);

  // Default to Landscape 320x240
  lcd_ctx.width = 320;
  lcd_ctx.height = 240;

  LCD_CS_HIGH();

  lcd_ctx.is_initialized = true;
  return &lcd_ctx;
}

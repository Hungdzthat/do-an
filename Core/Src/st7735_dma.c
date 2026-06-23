#include "st7735_dma.h"
#include "fonts.h"
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/*  GPIO helpers */
/* -------------------------------------------------------------------------- */
#define TFT_CS_LOW() HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_RESET)
#define TFT_CS_HIGH() HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_SET)
#define TFT_DC_CMD() HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_RESET)
#define TFT_DC_DATA() HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET)

/* -------------------------------------------------------------------------- */
/*  SPI double-buffer for DMA */
/* -------------------------------------------------------------------------- */
static uint8_t lineBuffer[2][320];
static volatile uint8_t pingPong = 0;

#include "FreeRTOS.h"
#include "semphr.h"
static SemaphoreHandle_t spiDmaSem = NULL;
static StaticSemaphore_t spiDmaSemBuffer;

/* -------------------------------------------------------------------------- */
/*  Oscilloscope colour palette (RGB565) */
/* -------------------------------------------------------------------------- */
#define COLOR_BG 0x0000u       /* black       - background          */
#define COLOR_GRID_V 0x4208u   /* dark gray   - vertical grid       */
#define COLOR_GRID_H 0x4208u   /* dark gray   - horizontal grid     */
#define COLOR_AXIS_X 0xFFFFu   /* white       - X-axis (0 V)        */
#define COLOR_WAVE 0xD01Fu     /* red/pink    - waveform            */
#define COLOR_LABEL_BG 0x2104u /* very dark   - label strip bg      */
#define COLOR_LABEL 0x07E0u    /* green       - parameter text      */

/* VOL_DIV_MV and TIME_DIV_US are defined in st7735_dma.h.
 * Change them THERE to rescale the waveform and update the labels. */

/* -------------------------------------------------------------------------- */
/*  Low-level SPI helpers */
/* -------------------------------------------------------------------------- */
static void writeCMDTFT(uint8_t cmd) {
  TFT_CS_LOW();
  TFT_DC_CMD();
  HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
  TFT_CS_HIGH();
}

static void writeDataTFT(uint8_t data) {
  TFT_CS_LOW();
  TFT_DC_DATA();
  HAL_SPI_Transmit(&hspi1, &data, 1, 100);
  TFT_CS_HIGH();
}

/* -------------------------------------------------------------------------- */
/*  ST7735 initialisation */
/* -------------------------------------------------------------------------- */
static void DelayMs(uint32_t ms) {
  for (volatile uint32_t i = 0; i < ms * 12000; i++);
}

void ST7735_Init(void) {
  if (spiDmaSem == NULL) {
    spiDmaSem = xSemaphoreCreateBinaryStatic(&spiDmaSemBuffer);
    xSemaphoreGive(spiDmaSem);
  }

  TFT_CS_HIGH();

  /* HW RESET */
  HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_RESET);
  DelayMs(200);
  HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_SET);
  DelayMs(200);
  /* SW RESET */
  writeCMDTFT(0x01);
  DelayMs(120);
  /* SLEEP OUT */
  writeCMDTFT(0x11);
  DelayMs(200);

  writeCMDTFT(0xB1);
  writeDataTFT(0x01);
  writeDataTFT(0x2C);
  writeDataTFT(0x2D);
  writeCMDTFT(0xB2);
  writeDataTFT(0x01);
  writeDataTFT(0x2C);
  writeDataTFT(0x2D);
  writeCMDTFT(0xB3);
  writeDataTFT(0x01);
  writeDataTFT(0x2C);
  writeDataTFT(0x2D);
  writeDataTFT(0x01);
  writeDataTFT(0x2C);
  writeDataTFT(0x2D);
  writeCMDTFT(0xB4);
  writeDataTFT(0x07);
  writeCMDTFT(0xC0);
  writeDataTFT(0xA2);
  writeDataTFT(0x02);
  writeDataTFT(0x84);
  writeCMDTFT(0xC1);
  writeDataTFT(0xC5);
  writeCMDTFT(0xC2);
  writeDataTFT(0x0A);
  writeDataTFT(0x00);
  writeCMDTFT(0xC3);
  writeDataTFT(0x8A);
  writeDataTFT(0x2A);
  writeCMDTFT(0xC4);
  writeDataTFT(0x8A);
  writeDataTFT(0xEE);
  writeCMDTFT(0xC5);
  writeDataTFT(0x0E);

  writeCMDTFT(0xE0);
  writeDataTFT(0x02);
  writeDataTFT(0x1C);
  writeDataTFT(0x07);
  writeDataTFT(0x12);
  writeDataTFT(0x37);
  writeDataTFT(0x32);
  writeDataTFT(0x29);
  writeDataTFT(0x2D);
  writeDataTFT(0x29);
  writeDataTFT(0x25);
  writeDataTFT(0x2B);
  writeDataTFT(0x39);
  writeDataTFT(0x00);
  writeDataTFT(0x01);
  writeDataTFT(0x03);
  writeDataTFT(0x10);

  writeCMDTFT(0xE1);
  writeDataTFT(0x03);
  writeDataTFT(0x1D);
  writeDataTFT(0x07);
  writeDataTFT(0x06);
  writeDataTFT(0x2E);
  writeDataTFT(0x2C);
  writeDataTFT(0x29);
  writeDataTFT(0x2D);
  writeDataTFT(0x2E);
  writeDataTFT(0x2E);
  writeDataTFT(0x37);
  writeDataTFT(0x3F);
  writeDataTFT(0x00);
  writeDataTFT(0x00);
  writeDataTFT(0x02);
  writeDataTFT(0x10);

  writeCMDTFT(0x36);
  writeDataTFT(0x68); /* Memory Data Access Control */
  writeCMDTFT(0x20);  /* Display Inversion Off      */
  writeCMDTFT(0x3A);
  writeDataTFT(0x05); /* Interface Pixel Format      */
  writeCMDTFT(0x29);  /* Display ON                  */
  DelayMs(120);

  /* Fill entire screen black to clear random GRAM contents (white screen fix) */
  ST7735_SetWindow(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
  TFT_CS_LOW();
  TFT_DC_DATA();
  {
    uint8_t zero[2] = {0, 0};
    for (int i = 0; i < TFT_WIDTH * TFT_HEIGHT; i++) {
      HAL_SPI_Transmit(&hspi1, zero, 2, 10);
    }
  }
  TFT_CS_HIGH();
}

/* -------------------------------------------------------------------------- */
/*  Window / command helpers */
/* -------------------------------------------------------------------------- */
void ST7735_SetWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
  writeCMDTFT(0x2A);
  writeDataTFT(0x00);
  writeDataTFT(x0);
  writeDataTFT(0x00);
  writeDataTFT(x1);

  writeCMDTFT(0x2B);
  writeDataTFT(0x00);
  writeDataTFT(y0);
  writeDataTFT(0x00);
  writeDataTFT(y1);

  writeCMDTFT(0x2C);
}

/* -------------------------------------------------------------------------- */
/*  DMA TX complete callback */
/* -------------------------------------------------------------------------- */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi->Instance == SPI1) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (spiDmaSem != NULL) {
      xSemaphoreGiveFromISR(spiDmaSem, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

/* -------------------------------------------------------------------------- */
/*  Pixel / line primitives (used only during init / debug) */
/* -------------------------------------------------------------------------- */
void ST7735_DrawPixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || x >= TFT_WIDTH || y < 0 || y >= TFT_HEIGHT)
    return;
  TFT_CS_LOW();
  ST7735_SetWindow((uint8_t)x, (uint8_t)y, (uint8_t)(x + 1), (uint8_t)(y + 1));
  writeDataTFT(color >> 8);
  writeDataTFT(color & 0xFF);
  TFT_CS_HIGH();
}

void ST7735_DrawLine(int x0, int y0, int x1, int y1, uint16_t color) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;

  while (1) {
    ST7735_DrawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    int e2 = err << 1;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

/* -------------------------------------------------------------------------- */
/*  ST7735_RenderFrame */
/*                                                                             */
/*  RAM-efficient scan-line oscilloscope renderer (fits in 20 KB SRAM).       */
/*                                                                             */
/*  Grid:  vertical   every 16 px  - solid dark-gray                          */
/*         horizontal every 16 px  - dashed dark-gray (4 ON / 4 OFF)          */
/*  Axis:  y = 64                  - solid white (0 V / X-axis)               */
/*  Wave:  vertical segments connecting adjacent waveY[] samples - red        */
/*  Text:  bottom 11 px strip      - Vol/div (left), Time/div (right)         */
/*                                                                             */
/*  Extra RAM used beyond the two 320-byte DMA line buffers:                  */
/*    yLo / yHi    160 + 160  = 320 bytes (static, BSS)                       */
/* -------------------------------------------------------------------------- */

static inline int IsTextPixel(int x, int y, int startX, int startY, const char *text) {
    if (y < startY || y >= startY + 10 || x < startX) return 0;
    int charIdx = (x - startX) / 8;
    int charPixelX = (x - startX) % 8;
    if (charPixelX >= 7) return 0;
    
    for (int i = 0; i <= charIdx; i++) {
        if (text[i] == '\0') return 0;
    }
    char c = text[charIdx];
    if (c < ' ' || c > '~') return 0;
    
    uint16_t bits = Font_7x10.data[(c - ' ') * 10 + (y - startY)];
    return (bits >> (15 - charPixelX)) & 1;
}
void ST7735_RenderFrame(uint8_t waveY[], uint32_t vol_div_mv, uint32_t time_div_us, uint8_t selMode, uint8_t showInfo, float vrms, float freq, float vpp, float vdc, float duty) {
  /* ---- 1. Precompute oscilloscope vertical spans ---- */
  static uint8_t yLo[TFT_WIDTH];
  static uint8_t yHi[TFT_WIDTH];

  yLo[0] = waveY[0];
  yHi[0] = waveY[0];
  for (int x = 1; x < TFT_WIDTH; x++) {
    if (waveY[x - 1] < waveY[x]) {
      yLo[x] = waveY[x - 1];
      yHi[x] = waveY[x];
    } else {
      yLo[x] = waveY[x];
      yHi[x] = waveY[x - 1];
    }
  }

  /* ---- 2. Prepare text strings ---- */
  char lblFreq[16] = {0};
  char lblVdc[16] = {0};
  char lblDuty[16] = {0};
  char lblVpp[16] = {0};
  if (showInfo) {
    /* Right side */
    snprintf(lblFreq, sizeof(lblFreq), "F:%uHz", (unsigned int)freq);
    snprintf(lblVdc, sizeof(lblVdc), "Vdc:%.2fV", vdc);
    /* Left side */
    if (duty > 0.0f) snprintf(lblDuty, sizeof(lblDuty), "Duty:%.1f%%", duty);
    else snprintf(lblDuty, sizeof(lblDuty), "Duty:---");
    snprintf(lblVpp, sizeof(lblVpp), "Vpp:%.2fV", vpp);
  }

  char lblVol[16] = {0};
  snprintf(lblVol, sizeof(lblVol), "%umV/d", (unsigned int)vol_div_mv);

  char lblTime[16] = {0};
  snprintf(lblTime, sizeof(lblTime), "%uus/d", (unsigned int)time_div_us);
  
  uint16_t vColor = (selMode == 0) ? COLOR_LABEL : COLOR_GRID_V;
  uint16_t tColor = (selMode == 1) ? COLOR_LABEL : COLOR_GRID_V;

  /* Label strip: last 11 rows of screen (y = 117..127) */
  const int LABEL_Y = TFT_HEIGHT - 11; /* = 117 */

  /* ---- 3. Stream all scan lines via double-buffer DMA ---- */
  ST7735_SetWindow(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
  TFT_CS_LOW();
  TFT_DC_DATA();

  /* Inline lambda: compute RGB565 colour for pixel (x, y) */
  /* Implemented as a static inline to avoid code-size macros */

  /* Build & send line 0 (y=0 is always a horizontal grid row since 0%16==0) */
  {
    uint8_t *buf = lineBuffer[0];
    for (int x = 0; x < TFT_WIDTH; x++) {
      uint16_t c = COLOR_GRID_H; /* y=0: always a grid row */

      /* Vertical grid intersection - same colour, no change needed */

      /* Waveform on y=0 (overrides grid) */
      if (yLo[x] == 0)
        c = COLOR_WAVE;

      buf[x * 2] = c >> 8;
      buf[x * 2 + 1] = c & 0xFF;
    }
  }
  pingPong = 0;
  xSemaphoreTake(spiDmaSem, portMAX_DELAY);
  HAL_SPI_Transmit_DMA(&hspi1, lineBuffer[0], TFT_WIDTH * 2);

  for (int y = 1; y < TFT_HEIGHT; y++) {
    uint8_t nextBuf = pingPong ^ 1;
    uint8_t *buf = lineBuffer[nextBuf];
    uint8_t hgrid = (y % 16 == 0);
    uint8_t inLabel = (y >= LABEL_Y);
    int lrow = y - LABEL_Y;

    for (int x = 0; x < TFT_WIDTH; x++) {
      uint16_t c = COLOR_BG;

      /* Vertical grid - solid dark gray */
      if (x % 16 == 0)
        c = COLOR_GRID_V;

      /* Horizontal grid - solid dark gray */
      if (hgrid)
        c = COLOR_GRID_H;

      /* X-axis (0 V) - DASHED white (4 ON / 4 OFF) */
      if (y == 64 && ((x & 7) < 4))
        c = COLOR_AXIS_X;

      /* Waveform - red, highest priority in main area */
      if (y >= yLo[x] && y <= yHi[x])
        c = COLOR_WAVE;

      /* Info Box - Right side (F and Vdc) */
      if (showInfo && y < 24 && x >= (TFT_WIDTH - 76)) {
        if (IsTextPixel(x, y, TFT_WIDTH - 74, 2, lblFreq)) c = 0xFFE0u; /* Yellow */
        else if (IsTextPixel(x, y, TFT_WIDTH - 74, 12, lblVdc)) c = 0xFFE0u;
        else c = 0x0000u;
      } 
      /* Info Box - Left side (Duty and Vpp) */
      else if (showInfo && y < 24 && x < 76) {
        if (IsTextPixel(x, y, 2, 2, lblDuty)) c = 0xFFE0u;
        else if (IsTextPixel(x, y, 2, 12, lblVpp)) c = 0xFFE0u;
        else c = 0x0000u;
      } else if (inLabel) {
        /* Label strip (overrides everything) */
        c = COLOR_LABEL_BG;
        int lrow = y - LABEL_Y;
        if (lrow >= 1 && lrow < 11) {
          if (x < 80 && IsTextPixel(x, y, 2, LABEL_Y + 1, lblVol)) c = vColor;
          else if (x >= 80 && IsTextPixel(x, y, TFT_WIDTH - 56, LABEL_Y + 1, lblTime)) c = tColor;
        }
      }

      buf[x * 2] = c >> 8;
      buf[x * 2 + 1] = c & 0xFF;
    }

    pingPong = nextBuf;
    xSemaphoreTake(spiDmaSem, portMAX_DELAY);
    HAL_SPI_Transmit_DMA(&hspi1, buf, TFT_WIDTH * 2);
  }

  xSemaphoreTake(spiDmaSem, portMAX_DELAY);
  xSemaphoreGive(spiDmaSem);
  TFT_CS_HIGH();
}


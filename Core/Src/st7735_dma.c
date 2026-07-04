#include "st7735_dma.h"
#include "button.h"
#include "fonts.h"
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/*  GPIO helpers */
/* -------------------------------------------------------------------------- */
#define TFT_CS_LOW() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET)
#define TFT_CS_HIGH() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET)
#define TFT_DC_CMD() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET)
#define TFT_DC_DATA() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET)

/* -------------------------------------------------------------------------- */
/*  Band-buffer for DMA (Frame Buffer by band)                                */
/*  128 rows / 4 bands = 32 rows per band = 160×32×2 = 10240 bytes           */
/* -------------------------------------------------------------------------- */
#define BAND_H      32
#define BAND_PIXELS (TFT_WIDTH * BAND_H)
#define BAND_BYTES  (BAND_PIXELS * 2)
#define NUM_BANDS   (TFT_HEIGHT / BAND_H)

static uint8_t bandBuffer[BAND_BYTES];
volatile uint8_t spiBusy = 0;

/* Overlay / label geometry */
#define INFO_ROWS 44
#define INFO_COLS 80
#define HOLD_ROWS 10
#define HOLD_COLS 40

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
#define COLOR_LABEL_SEL 0xFFE0u /* yellow     - selected label      */
#define COLOR_INFO_BG 0x18E3u  /* dark blue-gray - info overlay bg  */
#define COLOR_INFO 0xFFFFu     /* white       - info overlay text   */
#define COLOR_HOLD 0xF800u     /* red         - HOLD indicator      */

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
void ST7735_Init(void) {
  /* HW RESET */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 0);
  HAL_Delay(200);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 1);
  HAL_Delay(200);
  /* SW RESET */
  writeCMDTFT(0x01);
  HAL_Delay(120);
  /* SLEEP OUT */
  writeCMDTFT(0x11);
  HAL_Delay(200);

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
  HAL_Delay(100);
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
  if (hspi->Instance == SPI1)
    spiBusy = 0;
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
/*  Helper: write one pixel into the band buffer (big-endian for SPI)         */
/* -------------------------------------------------------------------------- */
static inline void BandPutPixel(int x, int ly, uint16_t color) {
  int idx = (ly * TFT_WIDTH + x) * 2;
  bandBuffer[idx]     = (uint8_t)(color >> 8);
  bandBuffer[idx + 1] = (uint8_t)(color & 0xFF);
}

/* -------------------------------------------------------------------------- */
/*  Draw_Grid_To_Band — dotted grid lines (pixel at even positions only)      */
/*  Matches the dotted-line grid described in Section 2.3.5.1                 */
/* -------------------------------------------------------------------------- */
static void Draw_Grid_To_Band(int bandY) {
  for (int ly = 0; ly < BAND_H; ly++) {
    int gy = bandY + ly;
    /* Horizontal dotted grid: every 16 rows, pixel at even x only */
    if (gy % 16 == 0) {
      for (int x = 0; x < TFT_WIDTH; x += 2)
        BandPutPixel(x, ly, COLOR_GRID_H);
    }
    /* Vertical dotted grid: every 16 cols, pixel at even y only */
    if (gy % 2 == 0) {
      for (int x = 0; x < TFT_WIDTH; x += 16)
        BandPutPixel(x, ly, COLOR_GRID_V);
    }
  }
}

/* -------------------------------------------------------------------------- */
/*  Draw_Line_Bresenham_Band — Bresenham line clipped to current band         */
/*  Integer-only arithmetic for maximum speed on Cortex-M3 (Section 2.3.5.1) */
/* -------------------------------------------------------------------------- */
static void Draw_Line_Bresenham_Band(int x0, int y0, int x1, int y1,
                                     uint16_t color, int bandY) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (1) {
    int ly = y0 - bandY;
    if (x0 >= 0 && x0 < TFT_WIDTH && ly >= 0 && ly < BAND_H)
      BandPutPixel(x0, ly, color);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x0 += sx; }
    if (e2 <  dx) { err += dx; y0 += sy; }
  }
}

/* -------------------------------------------------------------------------- */
/*  DrawChar_ToBand — render one font character clipped to current band       */
/* -------------------------------------------------------------------------- */
static void DrawChar_ToBand(int cx, int cy, char ch,
                            uint16_t color, int bandY) {
  if (ch < ' ' || ch > '~') return;
  int ci = (ch - ' ') * 10;
  for (int row = 0; row < 10; row++) {
    int ly = (cy + row) - bandY;
    if (ly < 0 || ly >= BAND_H) continue;
    uint16_t bits = Font_7x10.data[ci + row];
    for (int col = 0; col < 7; col++) {
      int fx = cx + col;
      if (fx < 0 || fx >= TFT_WIDTH) continue;
      if ((bits >> (15 - col)) & 1)
        BandPutPixel(fx, ly, color);
    }
  }
}

/* -------------------------------------------------------------------------- */
/*  DrawString_ToBand — render a string into band buffer                      */
/* -------------------------------------------------------------------------- */
static void DrawString_ToBand(int x, int y, const char *str,
                              uint16_t color, int bandY) {
  for (const char *p = str; *p; p++, x += 8)
    DrawChar_ToBand(x, y, *p, color, bandY);
}

/* -------------------------------------------------------------------------- */
/*  FillRect_ToBand — fill a rectangle clipped to current band                */
/* -------------------------------------------------------------------------- */
static void FillRect_ToBand(int rx, int ry, int rw, int rh,
                            uint16_t color, int bandY) {
  for (int row = 0; row < rh; row++) {
    int ly = (ry + row) - bandY;
    if (ly < 0 || ly >= BAND_H) continue;
    for (int col = 0; col < rw; col++) {
      int fx = rx + col;
      if (fx < 0 || fx >= TFT_WIDTH) continue;
      BandPutPixel(fx, ly, color);
    }
  }
}

/* -------------------------------------------------------------------------- */
/*  ST7735_RenderFrame                                                         */
/*                                                                             */
/*  Band-buffered oscilloscope renderer (Section 2.3.5.2 Band-Buffering).    */
/*  Divides the 128-row display into 4 bands of 32 rows (10 KB each).        */
/*  Each band is fully composed in RAM, then DMA'd to ST7735 via SPI.        */
/*                                                                             */
/*  Grid:  dotted lines every 16 px (pixel at even positions only)            */
/*  Axis:  y = 64  — dashed white (4 ON / 4 OFF)  (0 V reference)            */
/*  Wave:  Bresenham line segments connecting adjacent waveY[] samples         */
/*  Text:  bottom 11 px strip — Vol/div (left), Time/div (right)              */
/*  Info:  top-left overlay   — Vpp, Vrms, Duty, Freq (when show_info)        */
/*  Hold:  top-right indicator — "HOLD" text (when hold_active)               */
/* -------------------------------------------------------------------------- */
void ST7735_RenderFrame(uint8_t waveY[]) {
  /* ---- 1. Prepare label strings ---- */
  char volLbl[24], timeLbl[24];
  snprintf(volLbl, sizeof(volLbl), "%umV/d", (unsigned)vol_div_mv);
  snprintf(timeLbl, sizeof(timeLbl), "%uus/d", (unsigned)time_div_us);

  const int LABEL_Y = TFT_HEIGHT - 11;  /* = 117 */
  const int HOLD_X  = TFT_WIDTH - HOLD_COLS; /* = 120 */

  char infoLines[4][16];
  if (show_info) {
    snprintf(infoLines[0], 16, "Vpp:%umV", (unsigned)sigParams.vpp_mv);
    snprintf(infoLines[1], 16, "Vrms:%umV", (unsigned)sigParams.vrms_mv);
    snprintf(infoLines[2], 16, "Duty:%u%%", (unsigned)sigParams.duty);
    snprintf(infoLines[3], 16, "F:%uHz", (unsigned)sigParams.freq_hz);
  }

  /* ---- 2. Render each band ---- */
  for (int band = 0; band < NUM_BANDS; band++) {
    int bandY   = band * BAND_H;
    int bandEnd = bandY + BAND_H;   /* exclusive upper bound */

    /* 2a. Clear band to background (COLOR_BG = 0x0000) */
    memset(bandBuffer, 0, BAND_BYTES);

    /* 2b. Draw dotted grid (Section 2.3.5.1 Grid Line Generation) */
    Draw_Grid_To_Band(bandY);

    /* 2c. Draw X-axis at y=64, dashed white (4 ON / 4 OFF) — 0 V ref */
    if (64 >= bandY && 64 < bandEnd) {
      int ly = 64 - bandY;
      for (int x = 0; x < TFT_WIDTH; x++) {
        if ((x & 7) < 4)
          BandPutPixel(x, ly, COLOR_AXIS_X);
      }
    }

    /* 2d. Draw waveform using Bresenham (Section 2.3.5.1 Waveform) */
    for (int x = 0; x < TFT_WIDTH - 1; x++) {
      int wy0 = waveY[x];
      int wy1 = waveY[x + 1];
      /* Early skip: segment entirely outside this band */
      int segMin = (wy0 < wy1) ? wy0 : wy1;
      int segMax = (wy0 > wy1) ? wy0 : wy1;
      if (segMax < bandY || segMin >= bandEnd)
        continue;
      Draw_Line_Bresenham_Band(x, wy0, x + 1, wy1, COLOR_WAVE, bandY);
    }

    /* 2e. Info overlay (top-left, 4 lines × 11px = 44px high) */
    if (show_info && bandY < INFO_ROWS) {
      FillRect_ToBand(0, 0, INFO_COLS, INFO_ROWS, COLOR_INFO_BG, bandY);
      for (int line = 0; line < 4; line++)
        DrawString_ToBand(0, line * 11, infoLines[line],
                          COLOR_INFO, bandY);
    }

    /* 2f. HOLD indicator (top-right, 10px high) */
    if (hold_active && bandY < HOLD_ROWS) {
      FillRect_ToBand(HOLD_X, 0, HOLD_COLS, HOLD_ROWS, COLOR_HOLD, bandY);
      DrawString_ToBand(HOLD_X + 4, 0, "HOLD", COLOR_INFO, bandY);
    }

    /* 2g. Label strip (bottom 11px, overrides everything) */
    if (bandEnd > LABEL_Y) {
      FillRect_ToBand(0, LABEL_Y, TFT_WIDTH, TFT_HEIGHT - LABEL_Y,
                      COLOR_LABEL_BG, bandY);
      uint16_t vc = (sel_mode == 0) ? COLOR_LABEL_SEL : COLOR_LABEL;
      DrawString_ToBand(2, LABEL_Y, volLbl, vc, bandY);
      uint16_t tc = (sel_mode == 1) ? COLOR_LABEL_SEL : COLOR_LABEL;
      DrawString_ToBand(TFT_WIDTH - 56, LABEL_Y, timeLbl, tc, bandY);
    }

    /* 2h. Send this band via SPI DMA (Section 2.3.5.3) */
    ST7735_SetWindow(0, (uint8_t)bandY, TFT_WIDTH - 1,
                     (uint8_t)(bandEnd - 1));
    TFT_CS_LOW();
    TFT_DC_DATA();
    spiBusy = 1;
    HAL_SPI_Transmit_DMA(&hspi1, bandBuffer, BAND_BYTES);
    while (spiBusy)
      ;
    TFT_CS_HIGH();
  }
}

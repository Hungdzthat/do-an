#ifndef __ST7735_DMA_H
#define __ST7735_DMA_H

#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern SPI_HandleTypeDef hspi1;
extern uint8_t waveY[160];

#define TFT_WIDTH 160
#define TFT_HEIGHT 128
#define BLOCK_H 16

/* =========================================================================
 * Oscilloscope scale settings
 *
 * VOL_DIV_MV_DEFAULT / TIME_DIV_US_DEFAULT are compile-time defaults.
 * At runtime the actual values come from vol_div_mv / time_div_us
 * (defined in button.c, declared in button.h).
 * ========================================================================= */
#define VOL_DIV_MV_DEFAULT 1000u  /* mV / div  — 1 V/div default    */
#define TIME_DIV_US_DEFAULT 500u  /* µs / div  — 500 µs/div default */

/* Hardware parameters — update if you change TIM3 or ADC configuration
 *   System clock  : HSI/2 × PLL×14 = 56 MHz
 *   TIM3          : PSC=0, ARR=1499  → f_TIM3 = 56 000 000 / 1500 = 37 333 Hz
 *   ADC dual-interleaved → 2 samples per TIM3 tick
 *   Effective ADC sample rate: ADC_FS_HZ = 2 × 37 333 = 74 667 Hz            */
#define ADC_VREF_MV 3300u /* ADC reference voltage, millivolts      */
#define ADC_FS_HZ 74667u  /* ADC effective sample rate, Hz          */

/* Derived waveform scaling — now uses runtime variables
 *
 *   Y_SCALE_F(v) = ADC counts per pixel
 *                = v × 4096 / (16 px/div × ADC_VREF_MV)
 *   X_STEP_F(t)  = ADC samples per pixel
 *                = t × ADC_FS_HZ / (16 px/div × 1 000 000 µs/s)              */
#define Y_SCALE_F_RT(v)                                                        \
  ((float)(v) * 4096.0f / (16.0f * (float)(ADC_VREF_MV)))
#define X_STEP_F_RT(t)                                                         \
  ((float)(t) * (float)(ADC_FS_HZ) / (16.0f * 1000000.0f))

/* Legacy macros (use defaults) — kept for backward compatibility */
#define Y_SCALE_F  Y_SCALE_F_RT(VOL_DIV_MV_DEFAULT)
#define X_STEP_F   X_STEP_F_RT(TIME_DIV_US_DEFAULT)

void ST7735_Init(void);
void ST7735_SetWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void ST7735_DrawPixel(int16_t x, int16_t y, uint16_t color);
void ST7735_DrawLine(int x0, int y0, int x1, int y1, uint16_t color);
void ST7735_RenderFrame(uint8_t waveY[]);

#endif
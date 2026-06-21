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
 * Change VOL_DIV_MV  to zoom Y axis (millivolts per screen division).
 * Change TIME_DIV_US to zoom X axis (microseconds per screen division).
 *
 * Changing either value automatically adjusts both the waveform rendering
 * (via Y_SCALE / X_STEP below) and the on-screen parameter labels.
 * ========================================================================= */
#define VOL_DIV_MV   1000u  /* mV / div  — 1 V/div = 1 grid square (16 px)  */
#define TIME_DIV_US  429u   /* µs / div  — default ≈ 429 µs/div (step =2.0) */

/* Hardware parameters — update if you change TIM3 or ADC configuration
 *   System clock  : HSI/2 × PLL×14 = 56 MHz
 *   TIM3          : PSC=0, ARR=1499  → f_TIM3 = 56 000 000 / 1500 = 37 333 Hz
 *   ADC dual-interleaved → 2 samples per TIM3 tick
 *   Effective ADC sample rate: ADC_FS_HZ = 2 × 37 333 = 74 667 Hz            */
#define ADC_VREF_MV  3300u           /* ADC reference voltage, millivolts      */
#define ADC_FS_HZ    74667u          /* ADC effective sample rate, Hz          */

/* Derived waveform scaling (used in BuildWaveform inside main.c)
 *
 *   Y_SCALE  = ADC counts per pixel
 *            = VOL_DIV_MV × 4096 / (16 px/div × ADC_VREF_MV)
 *   X_STEP   = ADC samples per pixel
 *            = TIME_DIV_US × ADC_FS_HZ / (16 px/div × 1 000 000 µs/s)       */
#define Y_SCALE_F  ((float)(VOL_DIV_MV)  * 4096.0f / (16.0f * (float)(ADC_VREF_MV)))
#define X_STEP_F   ((float)(TIME_DIV_US) * (float)(ADC_FS_HZ) / (16.0f * 1000000.0f))


void ST7735_Init(void);
void ST7735_SetWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void ST7735_DrawPixel(int16_t x, int16_t y, uint16_t color);
void ST7735_DrawLine(int x0, int y0, int x1, int y1, uint16_t color);
void ST7735_RenderFrame(uint8_t waveY[]);

#endif
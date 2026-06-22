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
 * Scaling is now dynamic, calculated in task_display.c based on gConfig.
 * ========================================================================= */

/* Hardware parameters — update if you change TIM3 or ADC configuration
 *   System clock  : HSI/2 × PLL×14 = 56 MHz
 *   TIM3          : PSC=0, ARR=1499  → f_TIM3 = 56 000 000 / 1500 = 37 333 Hz
 *   ADC dual-interleaved → 2 samples per TIM3 tick
 *   Effective ADC sample rate: ADC_FS_HZ = 2 × 37 333 = 74 667 Hz            */
#define ADC_VREF_MV  3300u           /* ADC reference voltage, millivolts      */
#define ADC_FS_HZ    74667u          /* ADC effective sample rate, Hz          */

/* Derived waveform scaling is now dynamically computed in BuildWaveform */


void ST7735_Init(void);
void ST7735_SetWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void ST7735_DrawPixel(int16_t x, int16_t y, uint16_t color);
void ST7735_DrawLine(int x0, int y0, int x1, int y1, uint16_t color);
void ST7735_RenderFrame(uint8_t waveY[], unsigned int vol_div_mv, unsigned int time_div_us);

#endif
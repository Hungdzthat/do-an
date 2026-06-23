#ifndef __ST7735_DMA_H
#define __ST7735_DMA_H

#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ST7735_CS_Pin GPIO_PIN_0
#define ST7735_CS_GPIO_Port GPIOB
#define ST7735_DC_Pin GPIO_PIN_1
#define ST7735_DC_GPIO_Port GPIOB
#define ST7735_RES_Pin GPIO_PIN_10
#define ST7735_RES_GPIO_Port GPIOB

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
 *   Effective ADC sample rate is dynamic based on timebase                   */
#define ADC_VREF_MV  3300u           /* ADC reference voltage, millivolts      */

/* Derived waveform scaling is now dynamically computed in BuildWaveform */


void ST7735_Init(void);
void ST7735_SetWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void ST7735_DrawPixel(int16_t x, int16_t y, uint16_t color);
void ST7735_DrawLine(int x0, int y0, int x1, int y1, uint16_t color);
void ST7735_RenderFrame(uint8_t *waveY, uint32_t vol_div_mv, uint32_t time_div_us, uint8_t selMode, uint8_t showInfo, float vrms, float freq, float vpp, float vdc);

#endif

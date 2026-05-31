#ifndef __ST7735_DMA_H
#define __ST7735_DMA_H

#include "main.h"

#define TFT_WIDTH   160
#define TFT_HEIGHT  128

extern SPI_HandleTypeDef hspi1;

void ST7735_Init(void);

void ST7735_SetWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

void ST7735_RenderFrame(uint8_t waveY[]);

#endif
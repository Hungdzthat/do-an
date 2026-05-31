#include "st7735_dma.h"

volatile uint8_t spiBusy = 0;

#define TFT_CS_LOW() \
HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_RESET)

#define TFT_CS_HIGH() \
HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET)

#define TFT_DC_CMD() \
HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET)

#define TFT_DC_DATA() \
HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET)

static void writeCMDTFT(uint8_t cmd)
{
    TFT_CS_LOW();
    TFT_DC_CMD();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    TFT_CS_HIGH();
}

static void writeDataTFT(uint8_t data)
{
    TFT_CS_LOW();
    TFT_DC_DATA();
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);
    TFT_CS_HIGH();
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi->Instance == SPI1)
    {
        spiBusy = 0;
    }
}

static void TFT_SendDMA(uint8_t *buf, uint16_t len)
{
    spiBusy = 1;
    HAL_SPI_Transmit_DMA(&hspi1, buf, len);
    while(spiBusy);
}

void ST7735_Init(void){
	//HW RESET
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 0);
	HAL_Delay(200);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, 1);
	HAL_Delay(200);
	//SW RESET
	writeCMDTFT(0x01);
	HAL_Delay(120);
	//SLEEP OUT
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
	
	writeCMDTFT(0x36);	//Memory Data Access Control
	writeDataTFT(0x68);
	
	writeCMDTFT(0x20);	//Display Inversion Off
	
	writeCMDTFT(0x3A);	//Interface Pixel Format
	writeDataTFT(0x05);
	
	writeCMDTFT(0x29);	//EN Display
	HAL_Delay(100);
}

void ST7735_SetWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
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

static uint8_t lineBuffer[320];

void ST7735_RenderFrame(uint8_t waveY[])
{
    ST7735_SetWindow(0, 0, 159, 127);
    TFT_CS_LOW();
    TFT_DC_DATA();

    for(int y=0;y<128;y++)
    {
        uint16_t color;
        for(int x=0;x<160;x++)
        {
            color = 0x0000;

            if(x % 20 == 0)
                color = 0x8410;

            if(y % 16 == 0)
                color = 0x8410;

            if(x == 80)
                color = 0x07E0;

            if(y == 64)
                color = 0x07E0;

            if(waveY[x] == y)
                color = 0xFFE0;

            lineBuffer[x*2] =
                    color >> 8;

            lineBuffer[x*2+1] =
                    color;
        }
        TFT_SendDMA(lineBuffer, 320);
    }
    TFT_CS_HIGH();
}
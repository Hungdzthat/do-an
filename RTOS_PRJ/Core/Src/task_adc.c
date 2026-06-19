#include "task_adc.h"
#include "main.h"
#include "stm32f1xx_hal.h" 

/* Buffer DMA – khai báo ? main.c ho?c ? dây */
uint16_t dmaBuf[SAMPLE_SIZE * 2];
static volatile uint8_t halfFlag;

void StartTaskADC(void const *argument)
{
    ADCData_t *pMsg;

    /* TODO: HAL_ADC_Start_DMA(&hadc1, (uint32_t*)dmaBuf, SAMPLE_SIZE*2); */

    for(;;)
    {
        /* Ch? DMA báo xong */
        xSemaphoreTake(mySem01Handle, portMAX_DELAY);

        pMsg = (ADCData_t *)osMailAlloc(myQueue01Handle, osWaitForever);
        if(pMsg != NULL)
        {
            uint16_t offset = halfFlag ? SAMPLE_SIZE : 0;
            for(uint16_t i = 0; i < SAMPLE_SIZE; i++)
                pMsg->data[i] = dmaBuf[offset + i];

            pMsg->time = osKernelSysTick();
            osMailPut(myQueue01Handle, pMsg);
        }
    }
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if(hadc->Instance == ADC1)
    {
        BaseType_t xWoken = pdFALSE;
        halfFlag = 0;
        xSemaphoreGiveFromISR(mySem01Handle, &xWoken);
        portYIELD_FROM_ISR(xWoken);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if(hadc->Instance == ADC1)
    {
        BaseType_t xWoken = pdFALSE;
        halfFlag = 1;
        xSemaphoreGiveFromISR(mySem01Handle, &xWoken);
        portYIELD_FROM_ISR(xWoken);
    }
}
#ifndef TASK_ADC_H
#define TASK_ADC_H

#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "stm32f1xx_hal.h" 

/* Kích thu?c buffer */
#define SAMPLE_SIZE   128

/* Struct d? li?u g?i sang TaskDSP */
typedef struct {
    uint16_t data[SAMPLE_SIZE];
    uint32_t time;
} ADCData_t;

/* Khai báo handle semaphore – d?nh nghia trong freertos.c */
extern SemaphoreHandle_t mySem01Handle;

/* Khai báo handle queue – d?nh nghia trong freertos.c */
extern osMailQId myQueue01Handle;

/* Khai báo hàm task */
void StartTaskADC(void const *argument);

/* DMA Callbacks */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);

#endif /* TASK_ADC_H */
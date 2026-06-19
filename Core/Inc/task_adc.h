#ifndef TASK_ADC_H
#define TASK_ADC_H

#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "osc_types.h"

extern uint16_t dmaBuf[SAMPLE_SIZE * 2];

void StartTaskADC(void const *argument);
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);

#endif

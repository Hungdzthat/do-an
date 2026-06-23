#include "Analog_signal.h"
#include "osc_rtos.h"
#include "stm32f1xx_hal.h"

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern TIM_HandleTypeDef htim3;

uint32_t ADC_VAL[ADC_BUFFER_SIZE * 2];
volatile uint8_t g_adc_half_flag = 0;

void Analog_Signal_Init(void) {
  /* Calibrate both ADCs before use - removes DC offset noise */
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_ADCEx_Calibration_Start(&hadc2);

  HAL_ADC_Start(&hadc2);
  HAL_ADCEx_MultiModeStart_DMA(&hadc1, (uint32_t *)ADC_VAL,
                               ADC_BUFFER_SIZE * 2);
  HAL_TIM_Base_Start(&htim3);
}

/* DMA half-complete: first half ready */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    g_adc_half_flag = 0;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(mySem01Handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

/* DMA full-complete: second half ready */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    g_adc_half_flag = 1;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(mySem01Handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

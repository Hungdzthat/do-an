#include "Analog_signal.h"
#include "osc_rtos.h"
#include "stm32f1xx_hal.h"

extern ADC_HandleTypeDef hadc1, hadc2;
extern TIM_HandleTypeDef htim3;

uint32_t ADC_VAL[ADC_BUFFER_SIZE * 2];
uint16_t ADC_VAL_FINAL[SAMPLE_SIZE];
volatile uint8_t g_adc_half_flag = 0;

void Analog_Signal_Init(void) {
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_ADCEx_Calibration_Start(&hadc2);
  HAL_ADC_Start(&hadc2);
  HAL_ADCEx_MultiModeStart_DMA(&hadc1, (uint32_t *)ADC_VAL, ADC_BUFFER_SIZE * 2);
  HAL_TIM_Base_Start(&htim3);
}

static void unpack_adc_data(uint32_t offset) {
  for (int i = 0; i < ADC_BUFFER_SIZE; i++) {
    uint32_t raw = ADC_VAL[offset + i];
    ADC_VAL_FINAL[2 * i] = (uint16_t)(raw & 0xFFFFu);
    ADC_VAL_FINAL[2 * i + 1] = (uint16_t)((raw >> 16) & 0xFFFFu);
  }
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    g_adc_half_flag = 0;
    unpack_adc_data(0);
    BaseType_t xWoken = pdFALSE;
    xSemaphoreGiveFromISR(mySem01Handle, &xWoken);
    portYIELD_FROM_ISR(xWoken);
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    g_adc_half_flag = 1;
    unpack_adc_data(ADC_BUFFER_SIZE);
    BaseType_t xWoken = pdFALSE;
    xSemaphoreGiveFromISR(mySem01Handle, &xWoken);
    portYIELD_FROM_ISR(xWoken);
  }
}

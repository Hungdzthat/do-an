#include "task_adc.h"
#include "Analog_signal.h"
#include "main.h"
#include "osc_rtos.h"

void StartTaskADC(void const *argument) {
  ADCData_t *pMsg;

  Analog_Signal_Init();

  while (1) {
    /* Chờ tín hiệu từ DMA ISR (cả Half và Full) */
    xSemaphoreTake(mySem01Handle, portMAX_DELAY);

    pMsg = (ADCData_t *)osMailAlloc(myQueue01Handle, osWaitForever);
    if (pMsg != NULL) {
      /* Xác định offset mảng DMA vừa hoàn thành (0 hoặc 64) */
      uint32_t offset = (g_adc_half_flag == 0) ? 0 : ADC_BUFFER_SIZE;

      /* Giải nén bit trực tiếp vào payload của Queue */
      /* 1 uint32_t chứa 2 sample (ADC1 và ADC2) */
      /* ADC_BUFFER_SIZE = 64 -> giải nén thành 128 uint16_t */
      for (uint16_t i = 0; i < ADC_BUFFER_SIZE; i++) {
        uint32_t raw = ADC_VAL[offset + i];
        pMsg->data[2 * i] = (uint16_t)(raw & 0xFFFFu);
        pMsg->data[2 * i + 1] = (uint16_t)((raw >> 16) & 0xFFFFu);
      }

      pMsg->time = osKernelSysTick();
      osMailPut(myQueue01Handle, pMsg);
    }
  }
}

#include "task_adc.h"
#include "Analog_signal.h"
#include "main.h"
#include "osc_rtos.h"

void StartTaskADC(void const *argument) {
  ADCData_t *pMsg;

  Analog_Signal_Init();

  while (1) {
    /* Wait for DMA ISR signal (Half or Full complete) */
    xSemaphoreTake(mySem01Handle, portMAX_DELAY);

    /* Use timeout instead of osWaitForever to prevent deadlock
     * when downstream tasks are busy. If queue is full, drop this sample. */
    pMsg = (ADCData_t *)osMailAlloc(myQueue01Handle, 5);
    if (pMsg != NULL) {
      /* Determine offset of completed DMA half */
      uint32_t offset = (g_adc_half_flag == 0) ? 0 : ADC_BUFFER_SIZE;

      /* Unpack dual-mode ADC data into queue payload */
      for (uint16_t i = 0; i < ADC_BUFFER_SIZE; i++) {
        uint32_t raw = ADC_VAL[offset + i];
        pMsg->data[2 * i]     = (uint16_t)(raw & 0xFFFFu);
        pMsg->data[2 * i + 1] = (uint16_t)((raw >> 16) & 0xFFFFu);
      }

      pMsg->time = osKernelSysTick();
      osMailPut(myQueue01Handle, pMsg);
    }
    /* If pMsg == NULL, queue was full -> sample dropped, no deadlock */
  }
}

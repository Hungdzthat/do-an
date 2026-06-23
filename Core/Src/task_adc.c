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
      for (uint16_t i = 0; i < SAMPLE_SIZE; i++) {
        pMsg->data[i] = ADC_VAL_FINAL[i];
      }

      pMsg->time = osKernelSysTick();
      osMailPut(myQueue01Handle, pMsg);
    }
    /* If pMsg == NULL, queue was full -> sample dropped, no deadlock */
  }
}

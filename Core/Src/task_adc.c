#include "main.h"
#include "task_adc.h"
#include "osc_rtos.h"
#include "Analog_signal.h"    

extern uint16_t ADC_VAL_FINAL[];           

void StartTaskADC(void const *argument)
{
    ADCData_t *pMsg;

    Analog_Signal_Init();                  

    while(1)
    {
        xSemaphoreTake(mySem01Handle, portMAX_DELAY);

        pMsg = (ADCData_t*)osMailAlloc(myQueue01Handle, osWaitForever);
        if(pMsg != NULL)
        {
            
            for(uint16_t i = 0; i < SAMPLE_SIZE; i++)
                pMsg->data[i] = ADC_VAL_FINAL[i];  

            pMsg->time = osKernelSysTick();
            osMailPut(myQueue01Handle, pMsg);
        }
    }
}


#include "task_dsp.h"
#include "osc_rtos.h"
#include "string.h"
// Hàm do partner vi?t - ch? khai báo extern
extern float dsp_calcVpp(const uint16_t *buf);
extern float dsp_calcVrms(const uint16_t *buf);
extern float dsp_calcFreq(const uint16_t *buf);
extern uint8_t dsp_findTrig(const uint16_t *buf);

void StartTaskDSP(void const *argument)
{
    osEvent     evt;
    ADCData_t  *pIn;
    DispData_t *pOut;

    for(;;)
    {
        // 1. Cho data cua TASKADC
        evt = osMailGet(myQueue01Handle, osWaitForever);

        if(evt.status == osEventMail)
        {
            pIn  = (ADCData_t*)evt.value.p;
            pOut = (DispData_t*)osMailAlloc(myQueue02Handle, osWaitForever);

            if(pOut != NULL)
            {
                // function...
                pOut->vpp     = dsp_calcVpp(pIn->data);
                pOut->vrms    = dsp_calcVrms(pIn->data);
                pOut->freq    = dsp_calcFreq(pIn->data);
                pOut->trigIdx = dsp_findTrig(pIn->data);
                memcpy(pOut->wave, pIn->data, SAMPLE_SIZE * sizeof(uint16_t));

                // 3. Gui sang TaskDisplay
                osMailPut(myQueue02Handle, pOut);
            }

            // 4. Tra o nho
            osMailFree(myQueue01Handle, pIn);
        }
    }
}
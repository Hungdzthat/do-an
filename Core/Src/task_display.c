#include "task_display.h"
#include "osc_rtos.h"
#include <stdio.h>

void StartTaskDisplay(void const *argument)
{
    osEvent     evt;
    DispData_t *pDisp;
    char        buf[20];



    while(1)
    {
        evt = osMailGet(myQueue02Handle, osWaitForever);

        if(evt.status == osEventMail)
        {
            pDisp = (DispData_t*)evt.value.p;


            snprintf(buf, sizeof(buf), "Vpp:%.2fV", pDisp->vpp);


            snprintf(buf, sizeof(buf), "%.2fVrms",  pDisp->vrms);

            snprintf(buf, sizeof(buf), "%.1fHz",    pDisp->freq);


            osMailFree(myQueue02Handle, pDisp);
        }

        osDelay(33);
    }
}

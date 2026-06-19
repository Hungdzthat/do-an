#include "task_btn.h"
#include "osc_rtos.h"
#include "Button_signal.h"   /* file partner */

/* Giá tr? m?c d?nh khi kh?i d?ng */
OscConfig_t gConfig = {
    .vdivScale = 1.0f,
    .timeDivMs = 5,
    .showInfo  = 1,
    .selMode   = SEL_VDIV,
    .holdRun   = OSC_RUN
};

void StartTaskBtn(void const *argument)
{
    for(;;)
    {
        /* RTOS ch? lo: d?c tín hi?u nút (partner x? lý) ? ghi gConfig có Mutex */

        if(Btn_IsSelPressed())
        {
            osMutexWait(gConfigMutexHandle, osWaitForever);
            gConfig.selMode = Btn_GetNextSelMode(gConfig.selMode);
            osMutexRelease(gConfigMutexHandle);
        }

        if(Btn_IsPlusPressed())
        {
            osMutexWait(gConfigMutexHandle, osWaitForever);
            Btn_ApplyPlus(&gConfig);
            osMutexRelease(gConfigMutexHandle);
        }

        if(Btn_IsMinusPressed())
        {
            osMutexWait(gConfigMutexHandle, osWaitForever);
            Btn_ApplyMinus(&gConfig);
            osMutexRelease(gConfigMutexHandle);
        }

        if(Btn_IsInfoPressed())
        {
            osMutexWait(gConfigMutexHandle, osWaitForever);
            gConfig.showInfo = !gConfig.showInfo;
            osMutexRelease(gConfigMutexHandle);
        }

        if(Btn_IsHoldPressed())
        {
            osMutexWait(gConfigMutexHandle, osWaitForever);
            gConfig.holdRun = !gConfig.holdRun;
            osMutexRelease(gConfigMutexHandle);
        }

        osDelay(50);
    }
}
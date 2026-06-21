#include "task_btn.h"
#include "osc_rtos.h"
/* Cau hinh mac dinh khi khoi dong */
OscConfig_t gConfig = {.vdivScale = 1.0f,
                       .timeDivMs = 5,
                       .showInfo = 1,
                       .selMode = SEL_VDIV,
                       .holdRun = OSC_RUN};

void StartTaskBtn(void const *argument) {
  for (;;) {
    /* Doc toan bo trang thai nut TRUOC khi chiem mutex
     * -> giam thoi gian giu mutex, tranh block task khac */
    uint8_t isSel = Btn_IsSelPressed();
    uint8_t isPlus = Btn_IsPlusPressed();
    uint8_t isMinus = Btn_IsMinusPressed();
    uint8_t isInfo = Btn_IsInfoPressed();
    uint8_t isHold = Btn_IsHoldPressed();

    if (isSel || isPlus || isMinus || isInfo || isHold) {
      osMutexWait(gConfigMutexHandle, osWaitForever);

      if (isSel)
        gConfig.selMode = Btn_GetNextSelMode(gConfig.selMode);
      if (isPlus)
        Btn_ApplyPlus(&gConfig);
      if (isMinus)
        Btn_ApplyMinus(&gConfig);
      if (isInfo)
        gConfig.showInfo = !gConfig.showInfo;
      if (isHold)
        gConfig.holdRun = !gConfig.holdRun;

      osMutexRelease(gConfigMutexHandle);
    }

    osDelay(50); /* Debounce + nhuong CPU */
  }
}

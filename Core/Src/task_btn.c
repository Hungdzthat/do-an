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

/* --- Implement missing Btn_ functions --- */
uint8_t Btn_IsSelPressed(void)   { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET; }
uint8_t Btn_IsPlusPressed(void)  { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET; }
uint8_t Btn_IsMinusPressed(void) { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET; }
uint8_t Btn_IsInfoPressed(void)  { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == GPIO_PIN_RESET; }
uint8_t Btn_IsHoldPressed(void)  { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_RESET; }

uint8_t Btn_GetNextSelMode(uint8_t currentMode) {
    if (currentMode == SEL_VDIV) return SEL_TIMEDIV;
    return SEL_VDIV;
}

void Btn_ApplyPlus(OscConfig_t *config) {
    if (config->selMode == SEL_VDIV) {
        config->vdivScale *= 1.2f;
    } else {
        if (config->timeDivMs < 100) config->timeDivMs += 5;
    }
}

void Btn_ApplyMinus(OscConfig_t *config) {
    if (config->selMode == SEL_VDIV) {
        config->vdivScale /= 1.2f;
    } else {
        if (config->timeDivMs > 5) config->timeDivMs -= 5;
    }
}


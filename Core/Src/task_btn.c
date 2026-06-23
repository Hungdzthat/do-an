#include "task_btn.h"
#include "osc_rtos.h"
#include "Button_signal.h"

extern TIM_HandleTypeDef htim3;

/* Default config at startup */
OscConfig_t gConfig = {
    .vdivScale = 1.0f,
    .timeDivMs = 5,
    .showInfo  = 1,
    .selMode   = SEL_VDIV,
    .holdRun   = OSC_RUN
};

void StartTaskBtn(void const *argument) {
  static uint8_t prevSel = 0, prevPlus = 0, prevMinus = 0, prevInfo = 0, prevHold = 0;

  for (;;) {
    /* Read all button states BEFORE taking mutex
     * -> minimize mutex hold time, avoid blocking other tasks */
    uint8_t curSel   = Btn_IsSelPressed();
    uint8_t curPlus  = Btn_IsPlusPressed();
    uint8_t curMinus = Btn_IsMinusPressed();
    uint8_t curInfo  = Btn_IsInfoPressed();
    uint8_t curHold  = Btn_IsHoldPressed();

    uint8_t trigSel   = (curSel && !prevSel);
    uint8_t trigPlus  = (curPlus && !prevPlus);
    uint8_t trigMinus = (curMinus && !prevMinus);
    uint8_t trigInfo  = (curInfo && !prevInfo);
    uint8_t trigHold  = (curHold && !prevHold);

    prevSel   = curSel;
    prevPlus  = curPlus;
    prevMinus = curMinus;
    prevInfo  = curInfo;
    prevHold  = curHold;

    if (trigSel || trigPlus || trigMinus || trigInfo || trigHold) {
      osMutexWait(gConfigMutexHandle, osWaitForever);

      if (trigSel)
        gConfig.selMode = Btn_GetNextSelMode(gConfig.selMode);
      if (trigPlus)
        Btn_ApplyPlus(&gConfig);
      if (trigMinus)
        Btn_ApplyMinus(&gConfig);
      if (trigInfo)
        gConfig.showInfo = !gConfig.showInfo;
      if (trigHold)
        gConfig.holdRun = (gConfig.holdRun == OSC_RUN) ? OSC_HOLD : OSC_RUN;

      osMutexRelease(gConfigMutexHandle);
    }

    osDelay(50); /* Debounce + yield CPU */
  }
}

/* --- Button GPIO read functions --- */
/* All buttons: GPIOA, active-low (pull-up configured in MX_GPIO_Init) */
uint8_t Btn_IsSelPressed(void)   { return HAL_GPIO_ReadPin(BTN_SEL_PORT,   BTN_SEL_PIN)   == GPIO_PIN_RESET; }
uint8_t Btn_IsPlusPressed(void)  { return HAL_GPIO_ReadPin(BTN_PLUS_PORT,  BTN_PLUS_PIN)  == GPIO_PIN_RESET; }
uint8_t Btn_IsMinusPressed(void) { return HAL_GPIO_ReadPin(BTN_MINUS_PORT, BTN_MINUS_PIN) == GPIO_PIN_RESET; }
uint8_t Btn_IsInfoPressed(void)  { return HAL_GPIO_ReadPin(BTN_INFO_PORT,  BTN_INFO_PIN)  == GPIO_PIN_RESET; }
uint8_t Btn_IsHoldPressed(void)  { return HAL_GPIO_ReadPin(BTN_HOLD_PORT,  BTN_HOLD_PIN)  == GPIO_PIN_RESET; }

SelMode_t Btn_GetNextSelMode(SelMode_t currentMode) {
    if (currentMode == SEL_VDIV) return SEL_TIMEDIV;
    return SEL_VDIV;
}

void Btn_ApplyPlus(OscConfig_t *config) {
    if (config->selMode == SEL_VDIV) {
        config->vdivScale *= 1.2f;
        if (config->vdivScale > 10.0f) config->vdivScale = 10.0f;
    } else {
        if (config->timeDivMs < 100) config->timeDivMs += 5;
        __HAL_TIM_SET_AUTORELOAD(&htim3, 100 * config->timeDivMs - 1);
    }
}

void Btn_ApplyMinus(OscConfig_t *config) {
    if (config->selMode == SEL_VDIV) {
        config->vdivScale /= 1.2f;
        if (config->vdivScale < 0.1f) config->vdivScale = 0.1f;
    } else {
        if (config->timeDivMs > 1) config->timeDivMs -= 1;
        __HAL_TIM_SET_AUTORELOAD(&htim3, 100 * config->timeDivMs - 1);
    }
}

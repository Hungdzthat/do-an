#include "task_btn.h"
#include "osc_rtos.h"

extern TIM_HandleTypeDef htim3;

OscConfig_t gConfig = {
    .vdivMv    = 1000,
    .timeDivUs = 500,
    .showInfo  = 1,
    .selMode   = SEL_VDIV,
    .holdRun   = OSC_RUN
};

static void update_tim3_arr(uint32_t timeDivUs) {
    HAL_TIM_Base_Stop(&htim3);
    __HAL_TIM_SET_AUTORELOAD(&htim3, (7 * timeDivUs) - 1);
    htim3.Instance->CNT = 0;
    HAL_TIM_Base_Start(&htim3);
}

void StartTaskBtn(void const *argument) {
  static uint8_t prevSel = 0, prevPlus = 0, prevMinus = 0, prevInfo = 0, prevHold = 0;

  for (;;) {
    uint8_t curSel = Btn_IsSelPressed(), curPlus = Btn_IsPlusPressed(),
            curMinus = Btn_IsMinusPressed(), curInfo = Btn_IsInfoPressed(),
            curHold = Btn_IsHoldPressed();

    uint8_t trigSel = curSel && !prevSel, trigPlus = curPlus && !prevPlus,
            trigMinus = curMinus && !prevMinus, trigInfo = curInfo && !prevInfo,
            trigHold = curHold && !prevHold;

    prevSel = curSel; prevPlus = curPlus; prevMinus = curMinus;
    prevInfo = curInfo; prevHold = curHold;

    if (trigSel || trigPlus || trigMinus || trigInfo || trigHold) {
      osMutexWait(gConfigMutexHandle, osWaitForever);
      if (trigSel) gConfig.selMode = Btn_GetNextSelMode(gConfig.selMode);
      if (trigPlus) Btn_ApplyPlus(&gConfig);
      if (trigMinus) Btn_ApplyMinus(&gConfig);
      if (trigInfo) gConfig.showInfo = !gConfig.showInfo;
      if (trigHold) gConfig.holdRun = (gConfig.holdRun == OSC_RUN) ? OSC_HOLD : OSC_RUN;
      osMutexRelease(gConfigMutexHandle);
    }
    osDelay(50);
  }
}

uint8_t Btn_IsSelPressed(void)   { return HAL_GPIO_ReadPin(BTN_SEL_PORT,   BTN_SEL_PIN)   == GPIO_PIN_RESET; }
uint8_t Btn_IsPlusPressed(void)  { return HAL_GPIO_ReadPin(BTN_PLUS_PORT,  BTN_PLUS_PIN)  == GPIO_PIN_RESET; }
uint8_t Btn_IsMinusPressed(void) { return HAL_GPIO_ReadPin(BTN_MINUS_PORT, BTN_MINUS_PIN) == GPIO_PIN_RESET; }
uint8_t Btn_IsInfoPressed(void)  { return HAL_GPIO_ReadPin(BTN_INFO_PORT,  BTN_INFO_PIN)  == GPIO_PIN_RESET; }
uint8_t Btn_IsHoldPressed(void)  { return HAL_GPIO_ReadPin(BTN_HOLD_PORT,  BTN_HOLD_PIN)  == GPIO_PIN_RESET; }

SelMode_t Btn_GetNextSelMode(SelMode_t mode) {
    return (mode == SEL_VDIV) ? SEL_TIMEDIV : SEL_VDIV;
}

void Btn_ApplyPlus(OscConfig_t *cfg) {
    if (cfg->selMode == SEL_VDIV) {
        if (cfg->vdivMv < 50000) cfg->vdivMv += 50;
    } else {
        if (cfg->timeDivUs < 100000) cfg->timeDivUs += 50;
        update_tim3_arr(cfg->timeDivUs);
    }
}

void Btn_ApplyMinus(OscConfig_t *cfg) {
    if (cfg->selMode == SEL_VDIV) {
        if (cfg->vdivMv > 50) cfg->vdivMv -= 50;
    } else {
        if (cfg->timeDivUs > 50) cfg->timeDivUs -= 50;
        update_tim3_arr(cfg->timeDivUs);
    }
}

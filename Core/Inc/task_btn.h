#ifndef TASK_BTN_H
#define TASK_BTN_H

#include "main.h"
#include "cmsis_os.h"
#include "osc_types.h"

#define BTN_MODE_PIN    GPIO_PIN_1
#define BTN_MODE_PORT   GPIOA

#define BTN_V_PIN       GPIO_PIN_2
#define BTN_V_PORT      GPIOA

#define BTN_TIME_PIN    GPIO_PIN_3
#define BTN_TIME_PORT   GPIOA

#define BTN_INFO_PIN    GPIO_PIN_4
#define BTN_INFO_PORT   GPIOA

/* Bien cau hinh - dinh nghia trong task_btn.c */
extern OscConfig_t gConfig;

// Mutex
extern osMutexId   gConfigMutexHandle;

void StartTaskBtn(void const *argument);

/* Prototypes thay cho Button_signal.h (để IDE hết báo đỏ) */
extern uint8_t Btn_IsSelPressed(void);
extern uint8_t Btn_IsPlusPressed(void);
extern uint8_t Btn_IsMinusPressed(void);
extern uint8_t Btn_IsInfoPressed(void);
extern uint8_t Btn_IsHoldPressed(void);

extern SelMode_t Btn_GetNextSelMode(SelMode_t current_mode);
extern void Btn_ApplyPlus(OscConfig_t *cfg);
extern void Btn_ApplyMinus(OscConfig_t *cfg);

#endif

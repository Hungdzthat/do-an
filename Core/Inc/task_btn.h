#ifndef TASK_BTN_H
#define TASK_BTN_H

#include "main.h"
#include "cmsis_os.h"
#include "osc_types.h"

/* Button GPIO mapping (matches MX_GPIO_Init in main.c)
 * Active-low with internal pull-up */
#define BTN_SEL_PIN     GPIO_PIN_8
#define BTN_SEL_PORT    GPIOA

#define BTN_PLUS_PIN    GPIO_PIN_12
#define BTN_PLUS_PORT   GPIOB

#define BTN_MINUS_PIN   GPIO_PIN_13
#define BTN_MINUS_PORT  GPIOB

#define BTN_INFO_PIN    GPIO_PIN_14
#define BTN_INFO_PORT   GPIOB

#define BTN_HOLD_PIN    GPIO_PIN_15
#define BTN_HOLD_PORT   GPIOB

/* Global config - defined in task_btn.c */
extern OscConfig_t gConfig;

/* Mutex */
extern osMutexId   gConfigMutexHandle;

void StartTaskBtn(void const *argument);

/* Button read functions */
extern uint8_t Btn_IsSelPressed(void);
extern uint8_t Btn_IsPlusPressed(void);
extern uint8_t Btn_IsMinusPressed(void);
extern uint8_t Btn_IsInfoPressed(void);
extern uint8_t Btn_IsHoldPressed(void);

extern SelMode_t Btn_GetNextSelMode(SelMode_t current_mode);
extern void Btn_ApplyPlus(OscConfig_t *cfg);
extern void Btn_ApplyMinus(OscConfig_t *cfg);

#endif

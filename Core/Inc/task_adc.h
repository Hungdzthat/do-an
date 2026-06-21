#ifndef TASK_ADC_H
#define TASK_ADC_H

#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "osc_types.h"

void StartTaskADC(void const *argument);

#endif /* TASK_ADC_H */

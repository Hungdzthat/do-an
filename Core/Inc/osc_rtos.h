#ifndef FREERTOS_OSC_H
#define FREERTOS_OSC_H

#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "osc_types.h"

//  Handle Task 
extern osThreadId myTask01Handle;
extern osThreadId myTask02Handle;
extern osThreadId myTask03Handle;
extern osThreadId myTask04Handle;

// Handle Mail Queue 
extern osMailQId myQueue01Handle;
extern osMailQId myQueue02Handle;

// Binary Semaphore 
extern SemaphoreHandle_t mySem01Handle;

// Handle Mutex
extern osMutexId gConfigMutexHandle;


#endif
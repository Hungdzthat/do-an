# STEP 1: DETAILED REVIEW - COMPLETE ✅

## Summary
- **Status:** 1 Race Condition Found & FIXED
- **Remaining:** All RTOS logic verified SAFE
- **Ready:** Proceed to Step 2 (Fix Bugs)

---

## Issues Found & Fixed

### 🔴 RACE CONDITION #1 - gConfig Access Without Mutex
**Severity:** HIGH - Data corruption risk

**Problem:**
- task_dsp.c line 84: Reads `gConfig.timeDivUs` without Mutex
- task_display.c lines 10, 64-66: Reads multiple gConfig fields without Mutex
- If TaskBtn updates gConfig while reading → partial/corrupted values

**Fix Applied:**
```c
/* task_dsp.c - dsp_calcFreq() */
osMutexWait(gConfigMutexHandle, osWaitForever);
uint32_t timeDivUs = gConfig.timeDivUs;
osMutexRelease(gConfigMutexHandle);

/* task_display.c - BuildWaveform() */
osMutexWait(gConfigMutexHandle, osWaitForever);
unsigned int vol_div_mv = (unsigned int)(gConfig.vdivMv);
osMutexRelease(gConfigMutexHandle);

/* task_display.c - StartTaskDisplay() */
osMutexWait(gConfigMutexHandle, osWaitForever);
unsigned int vol_div_mv = (unsigned int)(gConfig.vdivMv);
unsigned int time_div_us = (unsigned int)(gConfig.timeDivUs);
uint8_t selMode = gConfig.selMode;
uint8_t showInfo = gConfig.showInfo;
osMutexRelease(gConfigMutexHandle);
```

---

## Verified Safe ✅

### Semaphore (mySem01Handle)
- ✅ xSemaphoreCreateBinary() in main.c
- ✅ xSemaphoreGiveFromISR() in both Half & Full ISR callbacks
- ✅ xHigherPriorityTaskWoken used correctly
- ✅ portYIELD_FROM_ISR() present
- ✅ xSemaphoreTake() in task_adc.c

### Queue Synchronization
- ✅ Queue01 (ADCData_t, 2 slots): Alloc → Put → Get → Free
- ✅ Queue02 (DispData_t, 2 slots): Alloc(timeout=10ms) → Put → Get → Free
- ✅ osMailAlloc() with timeout in task_adc (5ms) and task_dsp (10ms)
- ✅ No deadlock risk - timeouts prevent blocking

### Mutex Protection
- ✅ gConfigMutex created properly
- ✅ TaskBtn: Wait → Write → Release ✓
- ✅ TaskDSP: Wait → Read holdRun + timeDivUs → Release ✓
- ✅ TaskDisplay: Wait → Read all gConfig → Release ✓

### Task Priority Order ✅
- TaskADC: AboveNormal (200) - Time-critical, correct
- TaskDSP: Normal (100) - Processing, correct
- TaskDisplay: BelowNormal (50) - Rendering, correct
- TaskBtn: Low (1) - Debounce 50ms, not critical ✓

### Stack Sizes ✅
- TOTAL_HEAP_SIZE: 10240 bytes (10KB)
- configMINIMAL_STACK_SIZE: 128 words
- TaskADC: 256 words (1KB)
- TaskDSP: 512 words (2KB)
- TaskDisplay: 512 words (2KB)
- TaskBtn: 256 words (1KB)
- Total used: ~6-7KB, ~3KB free ✓

### Button Edge Detection ✅
- ✅ Rising edge: (curX && !prevX) correct
- ✅ Static prev states for debounce
- ✅ Mutex held only during gConfig update
- ✅ osDelay(50) for debounce + CPU yield

### ST7735 DMA Synchronization ✅
- ✅ Static semaphore (xSemaphoreCreateBinaryStatic)
- ✅ xSemaphoreGiveFromISR() in DMA callback
- ✅ xSemaphoreTake() before DMA start
- ✅ Ping-pong buffering for scan-line rendering

### TIM3 ARR Update ✅
- ✅ HAL_TIM_Base_Stop() before update
- ✅ __HAL_TIM_SET_AUTORELOAD() with safe value
- ✅ htim3.Instance->CNT = 0 to reset counter
- ✅ HAL_TIM_Base_Start() to resume
- ✓ No glitch risk

### ISR Safety ✅
- ✅ ConvHalfCpltCallback: Unpack first half + Give semaphore
- ✅ ConvCpltCallback: Unpack second half + Give semaphore
- ✅ No blocking calls in ISR
- ✅ ISR duration: ~1-2ms (unpack 80 uint32_t)

---

## Minor Observations (Not Bugs)

### g_adc_half_flag - Unused
- Set in both ISR callbacks but never read in task_adc.c
- ADC_VAL_FINAL overwritten completely each ISR
- **Impact:** None, just redundant code
- **Action:** Could remove in cleanup, but low priority

---

## Conclusion

✅ **All critical RTOS logic is SAFE**
✅ **1 Race Condition FIXED**
✅ **All primitives properly synchronized**
✅ **No deadlock risks**
✅ **No memory corruption risks**

**Next Step:** STEP 2 - Fix any remaining bugs (if found)

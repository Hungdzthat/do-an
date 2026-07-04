#include "task_display.h"
#include "osc_rtos.h"
#include <stdio.h>
#include "st7735_dma.h"
#include "task_btn.h"

uint8_t waveY[160];

void BuildWaveform(DispData_t *pDisp) {
  osMutexWait(gConfigMutexHandle, osWaitForever);
  unsigned int vol_div_mv = (unsigned int)(gConfig.vdivMv);
  osMutexRelease(gConfigMutexHandle);

  /* Hệ số tỉ lệ trục Y */
  const float scale = ((float)vol_div_mv * 4.0f) / 1000.0f;
  const int trig = (int)pDisp->trigIdx;

  /* Ánh xạ 1-1 từ mẫu ADC sang pixel hiển thị (do tần số lấy mẫu thay đổi động theo timebase) */
  for (int x = 0; x < 160; x++) {
    int idx = (trig + x) % SAMPLE_SIZE;
    float adc = (float)pDisp->wave[idx];

    /* Tính tọa độ Y (Căn giữa tại y=64) */
    int y = 64 - (int)((adc - 2022.0f) / scale);
    if (y < 0) y = 0;
    if (y > 117) y = 117;
    waveY[x] = (uint8_t)y;
  }
}

void StartTaskDisplay(void const *argument) {
  osEvent evt;
  DispData_t *pDisp;

  for (int i = 0; i < 160; i++) {
    int v = (i % 32);
    if (v > 16) v = 32 - v;
    waveY[i] = (uint8_t)(48 + v);
  }
  ST7735_RenderFrame(waveY, 1000, 1000, 0, 0, 0, 0, 0, 0, 0);

  while (1) {
    evt = osMailGet(myQueue02Handle, osWaitForever);
    if (evt.status == osEventMail) {
      pDisp = (DispData_t *)evt.value.p;
      BuildWaveform(pDisp);

      osMutexWait(gConfigMutexHandle, osWaitForever);
      unsigned int vol_div_mv = (unsigned int)(gConfig.vdivMv);
      unsigned int time_div_us = (unsigned int)(gConfig.timeDivUs);
      uint8_t selMode = gConfig.selMode;
      uint8_t showInfo = gConfig.showInfo;
      osMutexRelease(gConfigMutexHandle);

      ST7735_RenderFrame(waveY, vol_div_mv, time_div_us, selMode, showInfo, 
                         pDisp->vrms, pDisp->freq, pDisp->vpp, pDisp->vdc, pDisp->duty);
      osMailFree(myQueue02Handle, pDisp);
    }
  }
}

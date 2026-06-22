#include "task_display.h"
#include "osc_rtos.h"
#include <stdio.h>
#include "st7735_dma.h"
#include "task_btn.h"

uint8_t waveY[160];

void BuildWaveform(DispData_t *pDisp) {
  /*
   * Read current scaling from gConfig
   */
  unsigned int vol_div_mv = (unsigned int)(gConfig.vdivScale * 1000.0f);
  unsigned int time_div_us = (unsigned int)(gConfig.timeDivMs * 1000);

  const float scale = ((float)vol_div_mv * 4096.0f) / (16.0f * (float)ADC_VREF_MV);
  const float step  = ((float)time_div_us * (float)ADC_FS_HZ) / (16.0f * 1000000.0f);

  int trig = pDisp->trigIdx;

  for (int x = 0; x < 160; x++) {
    float pos = trig + x * step;

    while (pos >= SAMPLE_SIZE)
      pos -= SAMPLE_SIZE;

    int i0 = (int)pos;
    int i1 = (i0 + 1) % SAMPLE_SIZE;

    float frac = pos - i0;

    float adc = pDisp->wave[i0] * (1.0f - frac) + pDisp->wave[i1] * frac;

    /* Centre at y=64 (midpoint of 128-px screen = 0 V reference) */
    int y = 64 - (int)((adc - 2048.0f) / scale);

    if (y < 0)
      y = 0;
    if (y > 127)
      y = 127;

    waveY[x] = (uint8_t)y;
  }
}

void StartTaskDisplay(void const *argument) {
  osEvent evt;
  DispData_t *pDisp;
  char buf[20];

  while (1) {
    /* Tối ưu RTOS: Block bằng osMailGet, không cần thiết dùng thêm osDelay */
    evt = osMailGet(myQueue02Handle, osWaitForever);

    if (evt.status == osEventMail) {
      pDisp = (DispData_t *)evt.value.p;

      /* Giữ nguyên logic hiển thị ban đầu của partner */
      snprintf(buf, sizeof(buf), "Vpp:%.2fV", pDisp->vpp);
      snprintf(buf, sizeof(buf), "%.2fVrms", pDisp->vrms);
      snprintf(buf, sizeof(buf), "%.1fHz", pDisp->freq);

      /* Render waveform to ST7735 */
      BuildWaveform(pDisp);
      unsigned int vol_div_mv = (unsigned int)(gConfig.vdivScale * 1000.0f);
      unsigned int time_div_us = (unsigned int)(gConfig.timeDivMs * 1000);
      ST7735_RenderFrame(waveY, vol_div_mv, time_div_us);

      osMailFree(myQueue02Handle, pDisp);
    }
  }
}

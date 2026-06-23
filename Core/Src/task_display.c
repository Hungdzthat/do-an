#include "task_display.h"
#include "osc_rtos.h"
#include <stdio.h>
#include "st7735_dma.h"
#include "task_btn.h"

uint8_t waveY[160];

void BuildWaveform(DispData_t *pDisp) {
  unsigned int vol_div_mv = (unsigned int)(gConfig.vdivMv);

  /* Calculate voltage scale (Y axis) */
    /* Hardware calibrated: 64 counts = 1V. Screen: 16 pixels = 1 division. 
       If setting is vol_div_mv (e.g. 1000mV = 1V), then 1 division should span 1V.
       Counts per division = (vol_div_mv / 1000) * 64. 
       Scale (counts per pixel) = Counts per division / 16 = vol_div_mv * 4 / 1000. */
    const float scale = ((float)vol_div_mv * 4.0f) / 1000.0f;

    int trig = (int)pDisp->trigIdx;
    trig -= 40; /* Shift wave to the right by 40 pixels so trigger edge is visible */
    if (trig < 0) trig += SAMPLE_SIZE;

    /* Process waveform points */
    for (int x = 0; x < 160; x++) {
      int idx = (trig + x) % SAMPLE_SIZE;
      float adc = (float)pDisp->wave[idx];

      /* Centre at y=64 (midpoint of 128-px screen = 0 V reference) 
       * Hardware calibrated 0V reference is 2022. */
      int y = 64 - (int)((adc - 2022.0f) / scale);

    if (y < 0)
      y = 0;
    if (y > 117)
      y = 117; /* Keep waveform out of label strip area */

    waveY[x] = (uint8_t)y;
  }
}

void StartTaskDisplay(void const *argument) {
  osEvent evt;
  DispData_t *pDisp;

  /* TEST FRAME: Draw a sine-like pattern to prove TFT is working */
  for (int i = 0; i < 160; i++) {
    /* Simple triangle wave for visual test */
    int v = (i % 32);
    if (v > 16) v = 32 - v;
    waveY[i] = (uint8_t)(48 + v); /* Range: 48..64 */
  }
  ST7735_RenderFrame(waveY, 1000, 1000, 0, 0, 0, 0, 0, 0);

  while (1) {
    evt = osMailGet(myQueue02Handle, osWaitForever);

    if (evt.status == osEventMail) {
      pDisp = (DispData_t *)evt.value.p;

      /* Build pixel array from ADC data */
      BuildWaveform(pDisp);

      /* Render to TFT */
      unsigned int vol_div_mv = (unsigned int)(gConfig.vdivMv);
      unsigned int time_div_us = (unsigned int)(gConfig.timeDivUs);
      ST7735_RenderFrame(waveY, vol_div_mv, time_div_us, gConfig.selMode, gConfig.showInfo, pDisp->vrms, pDisp->freq, pDisp->vpp, pDisp->vdc);

      osMailFree(myQueue02Handle, pDisp);
    }
  }
}

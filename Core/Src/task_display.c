#include "task_display.h"
#include "osc_rtos.h"
#include <stdio.h>
#include "st7735_dma.h"
#include "task_btn.h"

uint8_t waveY[160];

void BuildWaveform(DispData_t *pDisp) {
  unsigned int vol_div_mv = (unsigned int)(gConfig.vdivScale * 1000.0f);

  /* Calculate voltage scale (Y axis) */
    /* Hardware frontend attenuates and biases to 1.65V (2048 counts)
     * scale = (vol_div_mv * 8.0f) / 1000.0f */
    const float scale = ((float)vol_div_mv * 8.0f) / 1000.0f;

    int trig = (int)pDisp->trigIdx;
    trig -= 40; /* Shift wave to the right by 40 pixels so trigger edge is visible */
    if (trig < 0) trig += SAMPLE_SIZE;

    /* Process waveform points */
    for (int x = 0; x < 160; x++) {
      int idx = (trig + x) % SAMPLE_SIZE;
      float adc = (float)pDisp->wave[idx];

      /* Centre at y=64 (midpoint of 128-px screen = 0 V reference) */
      int y = 64 - (int)((adc - 2048.0f) / scale);

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
  ST7735_RenderFrame(waveY, 1000, 1000, 0);

  while (1) {
    evt = osMailGet(myQueue02Handle, osWaitForever);

    if (evt.status == osEventMail) {
      pDisp = (DispData_t *)evt.value.p;

      /* Build pixel array from ADC data */
      BuildWaveform(pDisp);

      /* Render to TFT */
      unsigned int vol_div_mv = (unsigned int)(gConfig.vdivScale * 1000.0f);
      unsigned int time_div_us = (unsigned int)(gConfig.timeDivUs);
      ST7735_RenderFrame(waveY, vol_div_mv, time_div_us, gConfig.selMode);

      osMailFree(myQueue02Handle, pDisp);
    }
  }
}

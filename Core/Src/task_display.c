#include "task_display.h"
#include "osc_rtos.h"
#include <stdio.h>
#include "st7735_dma.h"
#include "task_btn.h"

uint8_t waveY[160];

void BuildWaveform(DispData_t *pDisp) {
  unsigned int vol_div_mv = (unsigned int)(gConfig.vdivScale * 1000.0f);
  unsigned int time_div_us = (unsigned int)(gConfig.timeDivMs * 1000);

  const float scale = ((float)vol_div_mv * 4096.0f) / (16.0f * (float)ADC_VREF_MV);
  const float step  = ((float)time_div_us * (float)ADC_FS_HZ) / (16.0f * 1000000.0f);

  int trig = (int)pDisp->trigIdx;

  /* Ensure step is at least 1 pixel per sample to avoid infinite loops */
  float actual_step = step;
  if (actual_step < 0.1f) actual_step = 0.1f;

  for (int x = 0; x < 160; x++) {
    float pos = (float)trig + (float)x * actual_step;

    /* Wrap around the circular buffer */
    while (pos >= SAMPLE_SIZE)
      pos -= SAMPLE_SIZE;
    while (pos < 0)
      pos += SAMPLE_SIZE;

    int i0 = (int)pos;
    if (i0 >= SAMPLE_SIZE) i0 = SAMPLE_SIZE - 1;
    int i1 = (i0 + 1) % SAMPLE_SIZE;

    float frac = pos - (float)i0;

    /* Linear interpolation between adjacent samples */
    float adc = pDisp->wave[i0] * (1.0f - frac) + pDisp->wave[i1] * frac;

    /* Centre at y=64 (midpoint of 128-px screen = 0 V reference)
     * scale = how many ADC counts per 16 pixels */
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
  ST7735_RenderFrame(waveY, 1000, 1000);

  while (1) {
    evt = osMailGet(myQueue02Handle, osWaitForever);

    if (evt.status == osEventMail) {
      pDisp = (DispData_t *)evt.value.p;

      /* Build pixel array from ADC data */
      BuildWaveform(pDisp);

      /* Render to TFT */
      unsigned int vol_div_mv = (unsigned int)(gConfig.vdivScale * 1000.0f);
      unsigned int time_div_us = (unsigned int)(gConfig.timeDivMs * 1000);
      ST7735_RenderFrame(waveY, vol_div_mv, time_div_us);

      osMailFree(myQueue02Handle, pDisp);
    }
  }
}

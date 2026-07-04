#include "button.h"
#include "st7735_dma.h"   /* AFE_GAIN, ADC_VREF_MV, ADC_FS_HZ */

/* ---- Runtime scale values ---- */
uint16_t vol_div_mv  = 1000;   /* mV / div — default 1 V/div      */
uint16_t time_div_us = 500;    /* µs / div — default 500 µs/div   */

/* ---- UI state ---- */
uint8_t sel_mode    = 0;       /* 0 = Vol/div,  1 = Time/div      */
uint8_t show_info   = 0;       /* 0 = hidden,   1 = overlay on    */
uint8_t hold_active = 0;       /* 0 = running,  1 = frozen        */

/* ---- Measured signal parameters ---- */
SignalParams_t sigParams = {0};

/* ---- Scale limits ---- */
#define VOL_MIN   100u     /* mV/div minimum    */
#define VOL_MAX   10000u   /* mV/div maximum    */
#define VOL_STEP  100u     /* mV/div increment  */

#define TIME_MIN  50u      /* µs/div minimum    */
#define TIME_MAX  5000u    /* µs/div maximum    */
#define TIME_STEP 50u      /* µs/div increment  */

/* ---- Debounce ---- */
#define DEBOUNCE_MS 200u

static uint32_t lastPress[5] = {0};   /* PA8..PA12 timestamps */

/* =========================================================================
 *  Button_Init  — configure PA8–PA12 as inputs with internal pull-up
 * ========================================================================= */
void Button_Init(void) {
  /* GPIOA clock is already enabled by MX_GPIO_Init */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin  = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10
                        | GPIO_PIN_11 | GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* =========================================================================
 *  ScanButtons  — poll all 5 buttons with debounce (called every main-loop)
 * ========================================================================= */

/* helper: returns 1 on a new falling-edge press, 0 otherwise */
static uint8_t BtnPressed(GPIO_TypeDef *port, uint16_t pin, int idx) {
  if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) {   /* active LOW */
    uint32_t now = HAL_GetTick();
    if (now - lastPress[idx] >= DEBOUNCE_MS) {
      lastPress[idx] = now;
      return 1;
    }
  }
  return 0;
}

void ScanButtons(void) {
  /* PA8  — Sel: toggle Vol/div ↔ Time/div */
  if (BtnPressed(GPIOA, GPIO_PIN_8, 0)) {
    sel_mode ^= 1;
  }

  /* PA9  — Up (+) */
  if (BtnPressed(GPIOA, GPIO_PIN_9, 1)) {
    if (sel_mode == 0) {
      if (vol_div_mv + VOL_STEP <= VOL_MAX)
        vol_div_mv += VOL_STEP;
    } else {
      if (time_div_us + TIME_STEP <= TIME_MAX)
        time_div_us += TIME_STEP;
    }
  }

  /* PA10 — Down (−) */
  if (BtnPressed(GPIOA, GPIO_PIN_10, 2)) {
    if (sel_mode == 0) {
      if (vol_div_mv >= VOL_MIN + VOL_STEP)
        vol_div_mv -= VOL_STEP;
    } else {
      if (time_div_us >= TIME_MIN + TIME_STEP)
        time_div_us -= TIME_STEP;
    }
  }

  /* PA11 — Info: toggle overlay */
  if (BtnPressed(GPIOA, GPIO_PIN_11, 3)) {
    show_info ^= 1;
  }

  /* PA12 — Hold: toggle waveform freeze */
  if (BtnPressed(GPIOA, GPIO_PIN_12, 4)) {
    hold_active ^= 1;
  }
}

/* =========================================================================
 *  ComputeSignalParams  — measure Vpp, Vrms, Freq, Duty from ADC buffer
 *
 *  adc[] : ADC_VAL_FINAL (12-bit, 0–4095)
 *  len   : number of samples (typically 640)
 * ========================================================================= */
void ComputeSignalParams(uint16_t *adc, int len) {
  if (len < 4) return;

  /* ================================================================
   * 1. Single pass: vmin, vmax, DC mean  (overflow-safe with float)
   * ================================================================ */
  uint16_t vmin = 4095, vmax = 0;
  float    sum  = 0.0f;

  for (int i = 0; i < len; i++) {
    uint16_t v = adc[i];
    if (v < vmin) vmin = v;
    if (v > vmax) vmax = v;
    sum += (float)v;
  }

  float mean = sum / (float)len;   /* DC level in ADC counts */

  /* Vpp in mV — real voltage at probe tip
   *   ADC Vpp  = (vmax-vmin) × VREF / 4096
   *   Real Vpp = ADC Vpp × AFE_GAIN  (reverse the 1/20 front-end attenuation)
   *   max: 4095 × 3300 × 20 = 270 270 000  →  fits uint32_t              */
  sigParams.vpp_mv = ((uint32_t)(vmax - vmin) * (uint32_t)ADC_VREF_MV
                      * (uint32_t)AFE_GAIN) / 4096u;

  /* ================================================================
   * 2. Vrms — AC RMS (subtract DC mean before squaring)
   *    Vrms_adc = sqrt( sum((v - mean)²) / N )
   *    Vrms_mV  = Vrms_adc × VREF / 4096 × AFE_GAIN
   * ================================================================ */
  {
    float ac_sum_sq = 0.0f;
    for (int i = 0; i < len; i++) {
      float ac = (float)adc[i] - mean;
      ac_sum_sq += ac * ac;
    }
    float rms_adc = 0.0f;
    float mean_sq = ac_sum_sq / (float)len;
    if (mean_sq > 0.0f) {
      /* Newton's method sqrt — 15 iterations for full float precision */
      rms_adc = mean_sq;
      for (int i = 0; i < 15; i++)
        rms_adc = 0.5f * (rms_adc + mean_sq / rms_adc);
    }
    sigParams.vrms_mv = (uint32_t)(rms_adc * (float)ADC_VREF_MV
                                   * (float)AFE_GAIN / 4096.0f);
  }

  /* ================================================================
   * 3. Frequency & Duty
   *
   * The buffer contains interleaved ADC1 (even idx) + ADC2 (odd idx).
   * Gaps are NOT uniform: ADC1→ADC2 = 500 ns, ADC2→ADC1(next) ≈ 26.3 µs.
   * Fix: use ONLY the even-indexed ADC1 samples (len/2 samples).
   * They are triggered uniformly by TIM3 at f_TIM3 = 37 333 Hz.
   * ================================================================ */
  {
    /* Build an ADC1-only view (every 2nd sample, starting at index 0) */
    int n1 = len / 2;                /* number of ADC1 samples = 320 */
    uint16_t mid = (vmax + vmin) / 2;

    int crossings   = 0;
    int first_cross = -1;
    int last_cross  = -1;

    for (int i = 1; i < n1; i++) {
      uint16_t prev = adc[(i-1) * 2];   /* even indices = ADC1 */
      uint16_t curr = adc[ i    * 2];

      /* Rising-edge crossing */
      if (prev < mid && curr >= mid) {
        crossings++;
        if (first_cross < 0) first_cross = i;
        last_cross = i;
      }
    }

    if (crossings >= 2 && last_cross > first_cross) {
      /* Period in ADC1-samples = span / (crossings − 1) */
      float period_samples = (float)(last_cross - first_cross)
                             / (float)(crossings - 1);
      /* Freq = f_TIM3 / period_samples
       * f_TIM3 = ADC_FS_HZ / 2 (only ADC1 samples used, uniform rate) */
      sigParams.freq_hz = (uint32_t)((float)(ADC_FS_HZ) / 2.0f
                                     / period_samples + 0.5f);

      /* Duty: count ADC1 samples above mid within first full period */
      int span        = last_cross - first_cross;
      int above       = 0;
      for (int i = first_cross; i < last_cross; i++) {
        if (adc[i * 2] >= mid) above++;
      }
      sigParams.duty = (uint8_t)((uint32_t)above * 100u / (uint32_t)span);
    } else {
      /* Not enough crossings — DC or out-of-range signal */
      sigParams.freq_hz = 0;
      sigParams.duty    = 0;
    }
  }
}


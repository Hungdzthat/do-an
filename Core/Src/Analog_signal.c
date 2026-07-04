#include "Analog_signal.h"

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern TIM_HandleTypeDef htim3;

uint32_t ADC_VAL[ADC_BUFFER_SIZE];
uint16_t ADC_VAL_FINAL[ADC_BUFFER_SIZE * 2];

/* =========================================================================
 * Buffer state machine — eliminates ALL race conditions between DMA
 * callbacks and the main-loop reader.
 *
 *  ADC_BUF_EMPTY   (0): main has consumed the data; callbacks may write
 *  ADC_BUF_HALF    (1): first half written by half-callback; waiting for 2nd
 *  ADC_BUF_READY   (2): full snapshot available; main may read
 *  ADC_BUF_READING (3): main is currently reading; callbacks must NOT write
 *
 * Transition diagram:
 *
 *   [EMPTY] --(half-cplt)--> [HALF] --(full-cplt)--> [READY]
 *   [READY] --(main reads)-> [READING] --(main done)-> [EMPTY]
 *
 * Any callback that fires while state == READING is silently skipped.
 * The next DMA cycle starts fresh from EMPTY.
 * ========================================================================= */
#define ADC_BUF_EMPTY   0u
#define ADC_BUF_HALF    1u
#define ADC_BUF_READY   2u
#define ADC_BUF_READING 3u

volatile uint8_t adc_data_ready = 0;   /* exposed for main: !=0 means READY  */
static  volatile uint8_t buf_state = ADC_BUF_EMPTY;

/* --------------------------------------------------------------------------
 * Analog_Signal_Init
 * -------------------------------------------------------------------------- */
void Analog_Signal_Init(void) {
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_ADCEx_Calibration_Start(&hadc2);

  HAL_ADC_Start(&hadc2);
  HAL_ADCEx_MultiModeStart_DMA(&hadc1, ADC_VAL, ADC_BUFFER_SIZE);
  HAL_TIM_Base_Start(&htim3);
}

/* --------------------------------------------------------------------------
 * Inline copy helper
 * One 32-bit DMA word = [15:0] ADC1 result | [31:16] ADC2 result
 * -------------------------------------------------------------------------- */
static inline void CopyHalf(int src, int dst, int count) {
  for (int i = 0; i < count; i++) {
    ADC_VAL_FINAL[dst + 2*i]   = (uint16_t)( ADC_VAL[src + i]        & 0xFFFF);
    ADC_VAL_FINAL[dst + 2*i+1] = (uint16_t)((ADC_VAL[src + i] >> 16) & 0xFFFF);
  }
}

/* --------------------------------------------------------------------------
 * Half-transfer callback — fires when DMA finishes ADC_VAL[0..159].
 * DMA is now writing [160..319], so [0..159] is SAFE to copy.
 * -------------------------------------------------------------------------- */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance != ADC1) return;

  if (buf_state == ADC_BUF_EMPTY) {
    CopyHalf(0, 0, ADC_BUFFER_SIZE / 2);  /* ADC_VAL[0..159] → FINAL[0..319] */
    buf_state = ADC_BUF_HALF;
  }
  /* If READING or READY: skip this cycle entirely — main still has the buffer */
}

/* --------------------------------------------------------------------------
 * Full-transfer callback — fires when DMA finishes ADC_VAL[160..319].
 * DMA has wrapped to [0], so [160..319] is SAFE to copy.
 * -------------------------------------------------------------------------- */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance != ADC1) return;

  if (buf_state == ADC_BUF_HALF) {
    CopyHalf(ADC_BUFFER_SIZE/2, ADC_BUFFER_SIZE, ADC_BUFFER_SIZE/2); /* [160..319] → FINAL[320..639] */
    buf_state    = ADC_BUF_READY;
    adc_data_ready = 1;   /* signal main loop */
  }
  /* If READING: skip — main is consuming the previous snapshot */
  /* If EMPTY: half-callback was skipped (main was reading), restart next cycle */
  else if (buf_state == ADC_BUF_EMPTY) {
    /* full fired but half was skipped → treat as empty, next half will restart */
  }
}

/* --------------------------------------------------------------------------
 * ADC_MarkReading / ADC_MarkDone — called by main loop
 *
 *   ADC_MarkReading(): transitions READY → READING
 *                      returns 1 if data is fresh, 0 if nothing ready
 *   ADC_MarkDone():    transitions READING → EMPTY
 * -------------------------------------------------------------------------- */
uint8_t ADC_MarkReading(void) {
  if (buf_state == ADC_BUF_READY) {
    buf_state      = ADC_BUF_READING;
    adc_data_ready = 0;
    return 1;
  }
  return 0;
}

void ADC_MarkDone(void) {
  buf_state = ADC_BUF_EMPTY;
}
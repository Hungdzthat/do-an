#ifndef INC_ANALOG_SIGNAL_H_
#define INC_ANALOG_SIGNAL_H_

#include "main.h"
#include "osc_types.h"

/* DMA transfer size for half a buffer. Since dual mode puts 2 samples in 1 uint32_t,
 * we need SAMPLE_SIZE/2 transfers to get SAMPLE_SIZE uint16_t samples.
 * SAMPLE_SIZE = 320 -> ADC_BUFFER_SIZE = 160 */
#define ADC_BUFFER_SIZE  (SAMPLE_SIZE / 2)   /* 160 */

/* Double buffer for DMA circular mode: size is ADC_BUFFER_SIZE * 2 = 320 uint32_t
 * Half-complete ISR -> first 160, Full-complete ISR -> second 160 */
extern uint32_t  ADC_VAL[ADC_BUFFER_SIZE * 2];

/* The final unpacked array, just like original task_Quyet code */
extern uint16_t  ADC_VAL_FINAL[SAMPLE_SIZE];

/* Flag to indicate which half of the buffer is ready (0 = first half, 1 = second half) */
extern volatile uint8_t g_adc_half_flag;

void Analog_Signal_Init(void);

#endif /* INC_ANALOG_SIGNAL_H_ */

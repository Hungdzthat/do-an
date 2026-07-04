#ifndef INC_ANALOG_SIGNAL_H_
#define INC_ANALOG_SIGNAL_H_

#include "main.h"

#define ADC_BUFFER_SIZE 320

extern volatile uint8_t adc_data_ready;  /* 1 = fresh buffer ready to read */

void Analog_Signal_Init(void);
uint8_t ADC_MarkReading(void);   /* claim buffer: READY→READING, returns 1 if fresh */
void    ADC_MarkDone(void);      /* release buffer: READING→EMPTY                   */
#endif /* INC_ANALOG_SIGNAL_H_ */
#include "Analog_signal.h"

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern TIM_HandleTypeDef htim3;

#define ADC_BUFFER_SIZE 320

uint32_t ADC_VAL[ADC_BUFFER_SIZE];
uint16_t ADC_VAL_FINAL[ADC_BUFFER_SIZE*2];

volatile int adc_ready = 0;

void Analog_Signal_Init(){
	HAL_ADC_Start(&hadc2);
	HAL_ADCEx_MultiModeStart_DMA(&hadc1, ADC_VAL, ADC_BUFFER_SIZE);
	HAL_TIM_Base_Start(&htim3);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
	if(hadc->Instance == ADC1){
		for(int i = 0; i < ADC_BUFFER_SIZE; i++){
			ADC_VAL_FINAL[2*i] = (uint16_t)(ADC_VAL[i] & 0xFFFF);

			ADC_VAL_FINAL[2*i + 1] = (uint16_t)((ADC_VAL[i] >> 16) & 0xFFFF);
		}
	}

}


#ifndef OSC_TYPES_H
#define OSC_TYPES_H

#include <stdint.h>

#define SAMPLE_SIZE  128

#define MODE_TANG   0
#define MODE_GIAM   1

typedef struct {
    uint16_t data[SAMPLE_SIZE];
    uint32_t time;
} ADCData_t;

typedef struct {
    uint16_t wave[SAMPLE_SIZE];
    float    vpp;
    float    vrms;
    float    freq;
    uint8_t  trigIdx;
} DispData_t;

typedef struct {
	  float vdivScale;
	  uint16_t timeDivMs;   
    uint8_t  showInfo;    
    uint8_t  mode;       
} OscConfig_t;

#endif

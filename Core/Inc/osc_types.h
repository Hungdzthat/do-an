#ifndef OSC_TYPES_H
#define OSC_TYPES_H

#include <stdint.h>

#define SAMPLE_SIZE  160

typedef enum {
    SEL_VDIV    = 0,
    SEL_TIMEDIV = 1
} SelMode_t;

typedef enum {
    OSC_RUN  = 0,
    OSC_HOLD = 1
} HoldRun_t;

/* ADC data: TaskADC -> TaskDSP (via Queue01) */
typedef struct {
    uint16_t data[SAMPLE_SIZE];
    uint32_t time;
} ADCData_t;

/* Display data: TaskDSP -> TaskDisplay (via Queue02) */
typedef struct {
    uint16_t wave[SAMPLE_SIZE];
    float    vpp;
    float    vrms;
    float    freq;
    float    duty;
    float    vdc;
    uint16_t trigIdx;   /* was uint8_t - overflows for index > 255 */
} DispData_t;

/* Oscilloscope config - protected by gConfigMutex */
typedef struct {
    uint32_t  vdivMv;
    uint32_t  timeDivUs;
    uint8_t   showInfo;
    SelMode_t selMode;
    HoldRun_t holdRun;
} OscConfig_t;

#endif /* OSC_TYPES_H */

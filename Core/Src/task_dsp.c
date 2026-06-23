#include "task_dsp.h"
#include "osc_rtos.h"
#include "string.h"
#include "st7735_dma.h"
#include "task_btn.h"

/* ---- DSP helper functions ---- */

float dsp_calcVpp(const uint16_t *buf) {
    uint16_t vmax = 0, vmin = 4095;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        if (buf[i] > vmax) vmax = buf[i];
        if (buf[i] < vmin) vmin = buf[i];
    }
    /* Hardware calibrated: 64 ADC counts = 1V */
    return (float)(vmax - vmin) / 64.0f;
}

float dsp_calcVrms(const uint16_t *buf) {
    float sum = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++)
        sum += (float)buf[i];
    float mean = sum / (float)SAMPLE_SIZE;

    float sum_sq = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        float diff = (float)buf[i] - mean;
        sum_sq += diff * diff;
    }
    float rms_counts = 0;
    if (sum_sq > 0) {
        /* Newton's method sqrt - avoid math.h */
        float x = sum_sq / (float)SAMPLE_SIZE;
        rms_counts = x;
        for (int j = 0; j < 10; j++)
            rms_counts = 0.5f * (rms_counts + x / rms_counts);
    }
    /* Hardware calibrated: 64 ADC counts = 1V */
    return rms_counts / 64.0f;
}

float dsp_calcVdc(const uint16_t *buf) {
    float sum = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++)
        sum += (float)buf[i];
    float mean = sum / (float)SAMPLE_SIZE;
    /* Hardware calibrated: 2022 is 0V reference. 64 counts = 1V. */
    return (mean - 2022.0f) / 64.0f;
}

float dsp_calcFreq(const uint16_t *buf) {
    uint16_t vmax = 0, vmin = 4095;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        if (buf[i] > vmax) vmax = buf[i];
        if (buf[i] < vmin) vmin = buf[i];
    }
    /* Require at least ~0.8V VPP to calculate frequency reliably */
    if ((vmax - vmin) < 50) return 0.0f;

    uint16_t mid = (vmax + vmin) / 2;
    uint16_t hyst = (vmax - vmin) / 8;
    if (hyst < 5) hyst = 5;

    int crossings = 0;
    int first_cross = -1;
    int last_cross = -1;
    int state = (buf[0] > mid) ? 1 : 0;

    for (int i = 1; i < SAMPLE_SIZE; i++) {
        if (state == 0 && buf[i] > (mid + hyst)) {
            state = 1;
            if (first_cross < 0) first_cross = i;
            last_cross = i;
            crossings++;
        } else if (state == 1 && buf[i] < (mid - hyst)) {
            state = 0;
        }
    }

    if (crossings < 2)
        return 0.0f;

    float period_samples = (float)(last_cross - first_cross) / (float)(crossings - 1);
    float sample_rate_hz = 16.0f * 1000000.0f / (float)gConfig.timeDivUs;
    return sample_rate_hz / period_samples;
}

float dsp_calcDuty(const uint16_t *buf) {
    uint16_t vmax = 0, vmin = 4095;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        if (buf[i] > vmax) vmax = buf[i];
        if (buf[i] < vmin) vmin = buf[i];
    }
    /* If amplitude is very small, it's not a valid pulse */
    if ((vmax - vmin) < 50) return 0.0f;

    uint16_t mid = (vmax + vmin) / 2;
    uint16_t hyst = (vmax - vmin) / 8;
    if (hyst < 5) hyst = 5;

    int crossings = 0;
    int first_cross = -1;
    int last_cross = -1;
    int state = (buf[0] > mid) ? 1 : 0;

    for (int i = 1; i < SAMPLE_SIZE; i++) {
        if (state == 0 && buf[i] > (mid + hyst)) {
            state = 1;
            if (first_cross < 0) first_cross = i;
            last_cross = i;
            crossings++;
        } else if (state == 1 && buf[i] < (mid - hyst)) {
            state = 0;
        }
    }

    if (crossings < 2) return 0.0f;

    int high_samples = 0;
    int total_samples = 0;
    /* Count high samples only within complete periods */
    for (int i = first_cross; i < last_cross; i++) {
        if (buf[i] >= mid) high_samples++;
        total_samples++;
    }

    if (total_samples == 0) return 0.0f;
    return ((float)high_samples / (float)total_samples) * 100.0f;
}

uint16_t dsp_findTrig(const uint16_t *buf) {
    uint16_t vmax = 0, vmin = 4095;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        if (buf[i] > vmax) vmax = buf[i];
        if (buf[i] < vmin) vmin = buf[i];
    }
    uint16_t mid = (vmax + vmin) / 2;

    for (int i = 1; i < (SAMPLE_SIZE / 2); i++) {
        if (buf[i - 1] < mid && buf[i] >= mid) {
            return (uint16_t)i;
        }
    }
    return 0;
}

/* ---- Task DSP ---- */

void StartTaskDSP(void const *argument)
{
    osEvent     evt;
    ADCData_t  *pIn;
    DispData_t *pOut;

    for(;;)
    {
        evt = osMailGet(myQueue01Handle, osWaitForever);

        if(evt.status == osEventMail)
        {
            pIn  = (ADCData_t*)evt.value.p;

            osMutexWait(gConfigMutexHandle, osWaitForever);
            uint8_t hold = (gConfig.holdRun == OSC_HOLD);
            osMutexRelease(gConfigMutexHandle);

            if (!hold) {
                /* Use timeout to prevent deadlock if display is busy */
                pOut = (DispData_t*)osMailAlloc(myQueue02Handle, 10);

                if(pOut != NULL)
                {
                    memcpy(pOut->wave, pIn->data, SAMPLE_SIZE * sizeof(uint16_t));
                    pOut->vpp     = dsp_calcVpp(pIn->data);
                    pOut->vrms    = dsp_calcVrms(pIn->data);
                    pOut->vdc     = dsp_calcVdc(pIn->data);
                    pOut->freq    = dsp_calcFreq(pIn->data);
                    pOut->duty    = dsp_calcDuty(pIn->data);
                    pOut->trigIdx = dsp_findTrig(pIn->data);

                    osMailPut(myQueue02Handle, pOut);
                }
            }
            /* Always free input regardless of output allocation */
            osMailFree(myQueue01Handle, pIn);
        }
    }
}

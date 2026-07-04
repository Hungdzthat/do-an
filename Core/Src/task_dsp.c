#include "task_dsp.h"
#include "osc_rtos.h"
#include "string.h"
#include "st7735_dma.h"
#include "task_btn.h"

/* ---- DSP helper functions ---- */

/* Hàm sắp xếp nổi bọt hỗ trợ tính toán thông số */
static void sort_array(uint16_t *arr, int size) {
    for (int i = 0; i < size - 1; i++)
        for (int j = 0; j < size - i - 1; j++)
            if (arr[j] > arr[j + 1]) {
                uint16_t temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
}

/* Bộ lọc nhiễu dạng tam giác giúp làm mịn các sóng sine/triangle */
static void smooth_filter(uint16_t *buf, int window) {
    if (window < 3) return;
    int halfW = window / 2;
    uint16_t temp[SAMPLE_SIZE];
    for (int i = 0; i < SAMPLE_SIZE; i++) temp[i] = buf[i];

    for (int i = halfW; i < SAMPLE_SIZE - halfW; i++) {
        uint32_t wsum = 0;
        uint32_t wdiv = 0;
        for (int j = -halfW; j <= halfW; j++) {
            uint32_t w = (uint32_t)(halfW + 1 - (j < 0 ? -j : j));  /* triangular weight */
            wsum += w * temp[i + j];
            wdiv += w;
        }
        buf[i] = (uint16_t)(wsum / wdiv);
    }
}

float dsp_calcVpp(const uint16_t *buf) {
    uint16_t sorted[SAMPLE_SIZE];
    for (int i = 0; i < SAMPLE_SIZE; i++) sorted[i] = buf[i];
    sort_array(sorted, SAMPLE_SIZE);
    /* Use 15th and 85th percentile instead of 10th and 90th for better noise immunity */
    return (float)(sorted[SAMPLE_SIZE * 85 / 100] - sorted[SAMPLE_SIZE * 15 / 100]) / 64.0f;
}

float dsp_calcVrms(const uint16_t *buf) {
    uint16_t sorted[SAMPLE_SIZE];
    for (int i = 0; i < SAMPLE_SIZE; i++) sorted[i] = buf[i];
    sort_array(sorted, SAMPLE_SIZE);
    uint16_t median = sorted[SAMPLE_SIZE / 2];
    
    /* Use trimmed data (15-85 percentile) to reduce outlier effects */
    uint16_t lower = sorted[SAMPLE_SIZE * 15 / 100];
    uint16_t upper = sorted[SAMPLE_SIZE * 85 / 100];
    
    float sum_sq = 0;
    int count = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        if (buf[i] >= lower && buf[i] <= upper) {
            float diff = (float)buf[i] - (float)median;
            sum_sq += diff * diff;
            count++;
        }
    }
    
    float rms = 0;
    if (sum_sq > 0 && count > 0) {
        float x = sum_sq / (float)count;
        rms = x;
        for (int j = 0; j < 10; j++)
            rms = 0.5f * (rms + x / rms);
    }
    return rms / 64.0f;
}

float dsp_calcVdc(const uint16_t *buf) {
    uint16_t sorted[SAMPLE_SIZE];
    for (int i = 0; i < SAMPLE_SIZE; i++) sorted[i] = buf[i];
    sort_array(sorted, SAMPLE_SIZE);
    return (float)(sorted[SAMPLE_SIZE / 2] - 2022.0f) / 64.0f;
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
    uint16_t hyst = (vmax - vmin) / 5;  /* Increased from /8 to /5 for better noise immunity */
    if (hyst < 15) hyst = 15;  /* Increased minimum hysteresis from 5 to 15 */

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
    
    /* CRITICAL: Read gConfig.timeDivUs with Mutex protection */
    osMutexWait(gConfigMutexHandle, osWaitForever);
    uint32_t timeDivUs = gConfig.timeDivUs;
    osMutexRelease(gConfigMutexHandle);
    
    float sample_rate_hz = 16.0f * 1000000.0f / (float)timeDivUs;
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
    uint16_t hyst = (vmax - vmin) / 5;  /* Increased from /8 to /5 for better noise immunity */
    if (hyst < 15) hyst = 15;  /* Increased minimum hysteresis from 5 to 15 */

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
    /* Find min/max voltage */
    uint16_t vmax = 0, vmin = 4095;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        if (buf[i] > vmax) vmax = buf[i];
        if (buf[i] < vmin) vmin = buf[i];
    }
    
    /* Schmitt trigger hysteresis - Increased for noise immunity */
    uint16_t mid = (vmax + vmin) / 2;
    uint16_t hyst = (vmax - vmin) / 5;  /* Increased from /8 to /5 */
    if (hyst < 15) hyst = 15;  /* Increased minimum hysteresis from 5 to 15 */
    
    /* Find rising edge with Schmitt trigger (anti-noise) + edge confirmation */
    int state = (buf[0] > mid) ? 1 : 0;  /* 0=low, 1=high */
    
    for (int i = 1; i < SAMPLE_SIZE - 2; i++) {
        if (state == 0 && buf[i] > (mid + hyst)) {
            /* Rising edge detected - confirm by checking next 2 samples stay high */
            if (buf[i + 1] > (mid + hyst) && buf[i + 2] > (mid + hyst)) {
                return (uint16_t)i;  /* Confirmed rising edge */
            }
        } else if (state == 1 && buf[i] < (mid - hyst)) {
            /* Falling edge - update state only */
            state = 0;
        }
    }
    
    return 0;  /* No rising edge found */
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
                pOut = (DispData_t*)osMailAlloc(myQueue02Handle, 10);

                if(pOut != NULL)
                {
                    memcpy(pOut->wave, pIn->data, SAMPLE_SIZE * sizeof(uint16_t));
                    
                    float raw_vpp  = dsp_calcVpp(pOut->wave);
                    float raw_vrms = dsp_calcVrms(pOut->wave);
                    float raw_vdc  = dsp_calcVdc(pOut->wave);
                    float raw_freq = dsp_calcFreq(pOut->wave);
                    float raw_duty = dsp_calcDuty(pOut->wave);

                    /* Bộ lọc đệ quy IIR bậc 1 giúp ổn định các số đo hiển thị */
                    static float f_vpp = -1.0f;
                    static float f_vrms = -1.0f;
                    static float f_vdc = 999.0f;
                    static float f_freq = -1.0f;
                    static float f_duty = -1.0f;

                    if (f_vpp < 0.0f) {
                        f_vpp  = raw_vpp;
                        f_vrms = raw_vrms;
                        f_vdc  = raw_vdc;
                        f_freq = raw_freq;
                        f_duty = raw_duty;
                    } else {
                        const float alpha = 0.15f; /* 15% mới, 85% cũ */
                        f_vpp  = alpha * raw_vpp  + (1.0f - alpha) * f_vpp;
                        f_vrms = alpha * raw_vrms + (1.0f - alpha) * f_vrms;
                        f_vdc  = alpha * raw_vdc  + (1.0f - alpha) * f_vdc;
                        f_duty = alpha * raw_duty + (1.0f - alpha) * f_duty;

                        /* Nhảy nhanh tần số khi cắm/rút que đo */
                        if ((raw_freq == 0.0f && f_freq < 10.0f) || (raw_freq > 0.0f && f_freq == 0.0f)) {
                            f_freq = raw_freq;
                        } else {
                            f_freq = alpha * raw_freq + (1.0f - alpha) * f_freq;
                        }
                    }

                    pOut->vpp     = f_vpp;
                    pOut->vrms    = f_vrms;
                    pOut->vdc     = f_vdc;
                    pOut->freq    = f_freq;
                    pOut->duty    = f_duty;
                    pOut->trigIdx = dsp_findTrig(pOut->wave);
                    
                    /* Nhận diện sóng vuông để bỏ qua bộ lọc làm mịn, bảo toàn cạnh đứng sắc nét */
                    {
                        uint16_t vmax = 0, vmin = 4095;
                        uint16_t max_diff = 0;
                        for (int i = 0; i < SAMPLE_SIZE; i++) {
                            uint16_t val = pOut->wave[i];
                            if (val > vmax) vmax = val;
                            if (val < vmin) vmin = val;
                            if (i < SAMPLE_SIZE - 1) {
                                uint16_t next_val = pOut->wave[i + 1];
                                uint16_t diff = (next_val > val) ? (next_val - val) : (val - next_val);
                                if (diff > max_diff) max_diff = diff;
                            }
                        }
                        uint16_t amplitude = vmax - vmin;
                        /* Lọc làm mịn khi biên độ đủ lớn và biến thiên mượt (sine/triangle) */
                        if (amplitude > 50 && max_diff < (amplitude * 4 / 10)) {
                            smooth_filter(pOut->wave, 5);
                        }
                    }

                    osMailPut(myQueue02Handle, pOut);
                }
            }
            /* Always free input regardless of output allocation */
            osMailFree(myQueue01Handle, pIn);
        }
    }
}

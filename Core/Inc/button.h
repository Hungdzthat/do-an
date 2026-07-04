#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include "main.h"

/* ---- Runtime scale values (adjustable via buttons) ---- */
extern uint16_t vol_div_mv;   /* mV / div  — default 1000       */
extern uint16_t time_div_us;  /* µs / div  — default 500        */

/* ---- UI state flags ---- */
extern uint8_t sel_mode;      /* 0 = Vol/div,  1 = Time/div     */
extern uint8_t show_info;     /* 0 = hidden,   1 = overlay on   */
extern uint8_t hold_active;   /* 0 = running,  1 = frozen       */

/* ---- Measured signal parameters (computed by ComputeSignalParams) ---- */
typedef struct {
  uint16_t vpp_mv;    /* peak-to-peak voltage in mV  */
  uint16_t vrms_mv;   /* RMS voltage in mV           */
  uint8_t  duty;      /* duty cycle 0–100 %          */
  uint32_t freq_hz;   /* frequency in Hz             */
} SignalParams_t;

extern SignalParams_t sigParams;

/* ---- API ---- */
void Button_Init(void);                              /* GPIO init PA8–PA12   */
void ScanButtons(void);                              /* poll + debounce      */
void ComputeSignalParams(uint16_t *adc, int len);   /* measure Vpp/Vrms/F/D */

#endif /* INC_BUTTON_H_ */

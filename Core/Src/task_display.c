#include "task_display.h"
#include "osc_rtos.h"
#include <stdio.h>

void StartTaskDisplay(void const *argument) {
  osEvent evt;
  DispData_t *pDisp;
  char buf[20];

  while (1) {
    /* Tối ưu RTOS: Block bằng osMailGet, không cần thiết dùng thêm osDelay */
    evt = osMailGet(myQueue02Handle, osWaitForever);

    if (evt.status == osEventMail) {
      pDisp = (DispData_t *)evt.value.p;

      /* Giữ nguyên logic hiển thị ban đầu của partner */
      snprintf(buf, sizeof(buf), "Vpp:%.2fV", pDisp->vpp);
      snprintf(buf, sizeof(buf), "%.2fVrms", pDisp->vrms);
      snprintf(buf, sizeof(buf), "%.1fHz", pDisp->freq);

      osMailFree(myQueue02Handle, pDisp);
    }
  }
}

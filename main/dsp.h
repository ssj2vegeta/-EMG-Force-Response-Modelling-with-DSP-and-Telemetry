#ifndef DSP_H
#define DSP_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void dsp_init(void);
void dsp_task(void *params);
QueueHandle_t dsp_get_feature_queue(void);

#endif


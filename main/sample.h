#ifndef SAMPLE_H
#define SAMPLE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void sampling_init(void);
QueueHandle_t sampling_get_queue(void);

#endif

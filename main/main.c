#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "dsps_biquad.h"
#include "dsp.h"
#include "network.h"
#include "sample.h"

void app_main(void)
{
    sampling_init();
    dsp_init();
    network_init();

    xTaskCreatePinnedToCore(dsp_task, "dsp_task", 4096, NULL, 5, NULL, 1);
}
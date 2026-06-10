#include "esp_adc/adc_oneshot.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <math.h>

static adc_oneshot_unit_handle_t adc_handle;
static QueueHandle_t sample_queue;

static void IRAM_ATTR sampling_isr(void *args)
{
    // fake sine wave simulating EMG signal
    static int t = 0;
    int sample = (int)(2048 + 1000 * sinf(2 * M_PI * 20 * t / 1000.0f));
    t++;
    // adc_oneshot_read(adc_handle, ADC_CHANNEL_6, &sample);

    xQueueSendFromISR(sample_queue, &sample, NULL);
}

void sampling_init(void)
{
    // ADC setup
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_6, &chan_config);

    // queue
    sample_queue = xQueueCreate(4096, sizeof(int));

    // timer
    const esp_timer_create_args_t timer_args = {
        .callback = sampling_isr,
        .name = "sampling_timer"
    };
    esp_timer_handle_t timer;
    esp_timer_create(&timer_args, &timer);
    esp_timer_start_periodic(timer, 1000);
}

QueueHandle_t sampling_get_queue(void)
{
    return sample_queue;
}
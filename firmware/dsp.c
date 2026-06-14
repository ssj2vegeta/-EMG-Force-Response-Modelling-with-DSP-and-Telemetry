
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sample.h"
#include "dsp.h"
#include "network.h"


static QueueHandle_t feature_queue;

void dsp_init(void)
{
    feature_queue = xQueueCreate(32, sizeof(float));
}

QueueHandle_t dsp_get_feature_queue(void)
{
    return feature_queue;
}

void dsp_task(void *params)
{
    QueueHandle_t sample_queue = sampling_get_queue();
    int raw;

    float running_mean = 0.0f;
    float sum_sq = 0.0f;
    float window[250] = {0};
    int window_idx = 0;

    while (true)
    {
        xQueueReceive(sample_queue, &raw, portMAX_DELAY);

        // cast to float
        float sample = (float)raw;

        // DC removal
        running_mean += (sample - running_mean) / 1000.0f;
        float dc_removed = sample - running_mean;

        // Rectification
        float rectified = fabsf(dc_removed);

        // RMS sliding window
        sum_sq -= window[window_idx] * window[window_idx];
        window[window_idx] = rectified;
        sum_sq += rectified * rectified;
        window_idx = (window_idx + 1) % 250;
        float rms = sqrtf(sum_sq / 250.0f);

        xQueueSend(feature_queue, &rms, 0);
    }
}

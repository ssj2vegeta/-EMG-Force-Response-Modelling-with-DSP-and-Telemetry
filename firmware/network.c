#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "dsp.h"

static const char *TAG = "NETWORK";
static EventGroupHandle_t wifi_events;
static esp_netif_t *esp_netif;
static const int CONNECTED = BIT0;
static const int DISCONNECTED = BIT1;
static bool attempt_reconnect = false;
static int disconnection_err_count = 0;
static esp_mqtt_client_handle_t client;

static void event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data);
static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data);
static void publish_task(void *param);

void wifi_connect_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
}

static void event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case WIFI_EVENT_STA_CONNECTED:
        disconnection_err_count = 0;
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
    {
        wifi_event_sta_disconnected_t *disc = event_data;
        ESP_LOGW(TAG, "Disconnected: reason %d", disc->reason);

        if (attempt_reconnect)
        {
            if (disc->reason == WIFI_REASON_NO_AP_FOUND ||
                disc->reason == WIFI_REASON_ASSOC_LEAVE ||
                disc->reason == WIFI_REASON_AUTH_EXPIRE)
            {
                if (disconnection_err_count++ < 5)
                {
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    esp_wifi_connect();
                    break;
                }
            }
        }
        xEventGroupSetBits(wifi_events, DISCONNECTED);
        break;
    }

    case IP_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_events, CONNECTED);
        break;
    }
}

esp_err_t wifi_connect_sta(char *ssid, char *pass, int timeout)
{
    attempt_reconnect = true;
    wifi_events = xEventGroupCreate();
    esp_netif = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t result = xEventGroupWaitBits(wifi_events, CONNECTED | DISCONNECTED, true, false, pdMS_TO_TICKS(timeout));
    return (result == CONNECTED) ? ESP_OK : ESP_FAIL;
}

void wifi_disconnect(void)
{
    attempt_reconnect = false;
    esp_wifi_stop();
    esp_netif_destroy(esp_netif);
}

void network_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_connect_init();
    ESP_ERROR_CHECK(wifi_connect_sta("Haziq", "October2005", 20000));

    esp_mqtt_client_config_t config = {
        .broker.address.uri = "mqtt://test.mosquitto.org:1883",
        .session.last_will = {
            .topic   = "emg/esp32_01/status",
            .msg     = "offline",
            .msg_len = strlen("offline"),
            .qos     = 1,
        }
    };

    client = esp_mqtt_client_init(&config);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    xTaskCreatePinnedToCore(publish_task, "publish_task", 4096, NULL, 4, NULL, 0);
}

static void mqtt_event_handler(void *arg, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT disconnected");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        break;
    default:
        break;
    }
}

static void publish_task(void *param)
{
    QueueHandle_t feature_queue = dsp_get_feature_queue();
    float rms;
    char message[64];

    while (true)
    {
        xQueueReceive(feature_queue, &rms, portMAX_DELAY);
        snprintf(message, sizeof(message), "{\"rms\": %.4f}", rms);
        esp_mqtt_client_publish(client, "emg/esp32_01/features", message, 0, 1, 0);
        vTaskDelay(pdMS_TO_TICKS(250));  // publish 4 times per second max not to overload mqtt
    }
}
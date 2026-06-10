idf_component_register(
        SRCS
        "main.c"
        "sample.c"
        "dsp.c"
        "network.c"
        INCLUDE_DIRS "."
        REQUIRES
        esp_wifi
        esp_event
        nvs_flash
        esp_adc
        esp_timer
        mqtt
)
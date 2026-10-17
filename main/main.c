/**
 * @file main.c
 * @brief ESP32 application entrypoint. Delegates all wiring to app_core so
 *        this file stays trivial and the logic stays testable elsewhere.
 */
#include "app_core.h"

#include "esp_log.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "sensor-pipeline starting");
    int rc = app_core_start();
    if (rc != 0) {
        ESP_LOGE(TAG, "app_core_start failed: %d", rc);
    }
}

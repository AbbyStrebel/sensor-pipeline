/**
 * @file reporter.c
 * @brief Consumer task: receives samples, filters them, logs the result.
 */
#include "reporter.h"

#include "sample.h"

#include "processor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

static const char *TAG = "reporter";

/** Stack depth (in words) for the reporter task. */
#define REPORTER_STACK_WORDS 2048

/** Priority of the reporter task. Lower than the sampler so the producer is
 *  never starved; the reporter catches up while the sampler sleeps. */
#define REPORTER_PRIORITY 4

static QueueHandle_t      s_in_queue;
static reporter_config_t *s_config;

/**
 * @brief Read the current window size under the config mutex.
 *
 * Shows the mutex pattern: take the lock, touch shared state, give it back,
 * holding it for as short a time as possible.
 */
static size_t read_window(void)
{
    /* portMAX_DELAY means the take blocks until acquired, so it does not fail
     * in practice. We still check the return and fall back to the current
     * config value's safe default (1 == passthrough) to keep the function
     * total even if the RTOS config ever changes to allow timed takes. */
    size_t w = 1;
    if (xSemaphoreTake(s_config->lock, portMAX_DELAY) == pdTRUE) {
        w = s_config->window;
        xSemaphoreGive(s_config->lock);
    } else {
        ESP_LOGW(TAG, "config lock take failed; using passthrough window");
    }
    return w;
}

static void reporter_task(void *arg)
{
    (void)arg;

    processor_filter_t filter;
    size_t             current_window = read_window();
    processor_filter_init(&filter, current_window);

    for (;;) {
        sample_t s;

        /* Block until a sample arrives. portMAX_DELAY = wait forever; while
         * blocked, this task consumes zero CPU and the scheduler runs others.
         * This is the heart of event-driven RTOS design: don't poll, block. */
        if (xQueueReceive(s_in_queue, &s, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        /* If the shared window changed, re-init the filter. */
        size_t w = read_window();
        if (w != current_window) {
            current_window = w;
            processor_filter_init(&filter, current_window);
            ESP_LOGI(TAG, "window changed to %u", (unsigned)current_window);
        }

        /* Delegate the actual math to the host-tested pure logic (ADR-0001). */
        float smoothed = 0.0F;
        processor_filter_push(&filter, s.value, &smoothed);

        ESP_LOGI(TAG, "seq=%u raw=%.3f smoothed=%.3f", (unsigned)s.seq, (double)s.value,
                 (double)smoothed);
    }
}

int reporter_start(QueueHandle_t in_queue, reporter_config_t *config)
{
    if (in_queue == NULL || config == NULL) {
        ESP_LOGE(TAG, "null arg");
        return 1;
    }
    s_in_queue = in_queue;
    s_config   = config;

    BaseType_t ok =
        xTaskCreate(reporter_task, "reporter", REPORTER_STACK_WORDS, NULL, REPORTER_PRIORITY, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "failed to create reporter task");
        return 1;
    }

    ESP_LOGI(TAG, "reporter started");
    return 0;
}

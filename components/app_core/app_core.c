/**
 * @file app_core.c
 * @brief Composition root: creates the shared queue and mutex-protected config,
 *        then starts the sampler (producer) and reporter (consumer) tasks.
 *
 * Keeping this wiring in one place (instead of in app_main or inside the tasks)
 * makes the data flow obvious and keeps each task ignorant of the other — they
 * only know about the queue between them.
 */
#include "app_core.h"

#include "reporter.h"
#include "sample.h"
#include "sampler.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_log.h"

static const char *TAG = "app_core";

/** How many samples the queue can hold before the producer starts dropping. */
#define QUEUE_LENGTH 8

/** Default moving-average window the reporter starts with. */
#define DEFAULT_WINDOW 8

/* Statically owned shared objects. Static storage (not stack) because these
 * must outlive app_core_start() — the tasks use them for the program's life. */
static reporter_config_t s_config;

int app_core_start(void)
{
    /* Create the queue that carries samples from sampler -> reporter. Each slot
     * is sizeof(sample_t); FreeRTOS allocates the backing storage. */
    QueueHandle_t queue = xQueueCreate(QUEUE_LENGTH, sizeof(sample_t));
    if (queue == NULL) {
        ESP_LOGE(TAG, "failed to create queue");
        return 1;
    }

    /* Create the mutex guarding the shared config. A FreeRTOS mutex has
     * priority inheritance, which avoids priority inversion. */
    s_config.lock = xSemaphoreCreateMutex();
    if (s_config.lock == NULL) {
        ESP_LOGE(TAG, "failed to create config mutex");
        return 1;
    }
    s_config.window = DEFAULT_WINDOW;

    /* Start the consumer first so it is ready and blocked on the (empty) queue
     * before the producer begins sending. */
    int rc = reporter_start(queue, &s_config);
    if (rc != 0) {
        return rc;
    }

    rc = sampler_start(queue);
    if (rc != 0) {
        return rc;
    }

    ESP_LOGI(TAG, "pipeline running (queue_len=%d, window=%d)", QUEUE_LENGTH, DEFAULT_WINDOW);
    return 0;
}

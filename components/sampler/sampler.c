/**
 * @file sampler.c
 * @brief Producer task: generates a sample every SAMPLER_PERIOD_MS and sends it
 *        to the output queue.
 */
#include "sampler.h"

#include "sample.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include <math.h>

static const char *TAG = "sampler";

/** How often the sampler produces a sample. */
#define SAMPLER_PERIOD_MS 100

/** Stack depth (in words) for the sampler task. */
#define SAMPLER_STACK_WORDS 2048

/** Priority of the sampler task (higher number = higher priority). */
#define SAMPLER_PRIORITY 5

/* The queue this task sends to. Set once in sampler_start() before the task
 * runs, then only read by the task — so no locking needed. */
static QueueHandle_t s_out_queue;

/**
 * @brief The task function. Runs forever; never returns.
 *
 * A FreeRTOS task is just a C function with signature `void fn(void *)` that
 * loops forever. It must NOT return — if it ever needs to stop, it calls
 * vTaskDelete(NULL) instead.
 */
static void sampler_task(void *arg)
{
    (void)arg; /* unused */

    uint32_t seq = 0;

    /* vTaskDelayUntil needs a reference "last wake time", seeded with now. */
    TickType_t       last_wake   = xTaskGetTickCount();
    const TickType_t period_ticks = pdMS_TO_TICKS(SAMPLER_PERIOD_MS);

    for (;;) {
        /* Generate a fake reading: a slow sine wave plus a little offset, so
         * the smoothed output is visibly different from the raw input. A real
         * driver would read a sensor here instead. */
        float value = sinf((float)seq * 0.1F) + 1.0F;

        sample_t s = {.seq = seq, .value = value};

        /* Send a COPY of `s` into the queue. The last arg is how long to block
         * if the queue is full; 0 = don't block, just try. We log if it fails
         * (queue full = the consumer isn't keeping up). */
        if (xQueueSend(s_out_queue, &s, 0) != pdTRUE) {
            ESP_LOGW(TAG, "queue full, dropped sample seq=%u", (unsigned)seq);
        }

        seq++;

        /* Sleep until the next period boundary. vTaskDelayUntil (vs vTaskDelay)
         * keeps a FIXED cadence: it accounts for the time the loop body took,
         * so we tick every 100ms regardless of work done. This is the correct
         * primitive for periodic tasks. While this task sleeps, the scheduler
         * runs other tasks (or idles). */
        vTaskDelayUntil(&last_wake, period_ticks);
    }
}

int sampler_start(QueueHandle_t out_queue)
{
    if (out_queue == NULL) {
        ESP_LOGE(TAG, "out_queue is NULL");
        return 1;
    }
    s_out_queue = out_queue;

    /* Create the task. The scheduler will start running it. Returns pdPASS on
     * success, or pdFAIL if there wasn't enough heap for the task's stack. */
    BaseType_t ok = xTaskCreate(sampler_task, "sampler", SAMPLER_STACK_WORDS,
                                NULL, SAMPLER_PRIORITY, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "failed to create sampler task");
        return 1;
    }

    ESP_LOGI(TAG, "sampler started (period=%dms)", SAMPLER_PERIOD_MS);
    return 0;
}

/**
 * @file reporter.h
 * @brief Consumer task: drains samples from a queue, smooths them with the
 *        (host-tested) processor filter, and logs the result.
 */
#ifndef REPORTER_H
#define REPORTER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Shared, mutable configuration read by the reporter.
 *
 * Demonstrates protecting shared state with a mutex: another task (or a future
 * console command) could change `window` at runtime, so every access is guarded
 * by `lock`.
 */
typedef struct {
    SemaphoreHandle_t lock;   /**< mutex guarding the fields below */
    size_t            window; /**< moving-average window size      */
} reporter_config_t;

/**
 * @brief Create and start the reporter task.
 *
 * @param in_queue  queue to receive samples from (must not be NULL).
 * @param config    shared config; its `lock` must already be created. Must not
 *                  be NULL.
 * @return 0 on success, non-zero if the task could not be created.
 */
int reporter_start(QueueHandle_t in_queue, reporter_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* REPORTER_H */

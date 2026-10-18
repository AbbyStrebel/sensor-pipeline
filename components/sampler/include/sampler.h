/**
 * @file sampler.h
 * @brief Timer-driven sampling task (producer).
 *
 * Wakes on a fixed period, generates one @ref sample_t, and sends it to the
 * queue it was given. All real "work" is trivial here on purpose — per
 * ADR-0001, logic lives in the host-tested `processor`, not in the task.
 */
#ifndef SAMPLER_H
#define SAMPLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and start the sampler task.
 *
 * @param out_queue  queue to send produced samples to (must be created by the
 *                   caller and sized for @ref sample_t items). Must not be NULL.
 * @return 0 on success, non-zero if the task could not be created.
 */
int sampler_start(QueueHandle_t out_queue);

#ifdef __cplusplus
}
#endif

#endif /* SAMPLER_H */

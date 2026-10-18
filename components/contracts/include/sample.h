/**
 * @file sample.h
 * @brief The message type passed from the sampler task to the reporter task
 *        through a FreeRTOS queue.
 *
 * A FreeRTOS queue stores fixed-size items by COPY. So we define a small plain
 * struct here; both the producer (sampler) and consumer (reporter) include this
 * header and agree on the layout.
 */
#ifndef SAMPLE_H
#define SAMPLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** One raw sample produced by the sampler. */
typedef struct {
    uint32_t seq;   /**< monotonically increasing sequence number */
    float    value; /**< the raw (unfiltered) sample value         */
} sample_t;

#ifdef __cplusplus
}
#endif

#endif /* SAMPLE_H */

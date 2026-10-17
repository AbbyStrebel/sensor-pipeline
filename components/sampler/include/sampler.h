/**
 * @file sampler.h
 * @brief Timer-driven sampling task. Owns the source and pushes raw samples
 *        onto a FreeRTOS queue for the processing/reporting pipeline.
 *
 * Phase 0: stub. Phase 2 wires up a real FreeRTOS task + queue.
 */
#ifndef SAMPLER_H
#define SAMPLER_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Start the sampler task. Returns 0 on success. */
int sampler_start(void);

#ifdef __cplusplus
}
#endif

#endif /* SAMPLER_H */

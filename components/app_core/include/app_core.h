/**
 * @file app_core.h
 * @brief Composition root: creates shared queues/mutexes and starts the
 *        sampler + reporter tasks. Keeps wiring out of app_main.
 */
#ifndef APP_CORE_H
#define APP_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Build the pipeline and start all tasks. Returns 0 on success. */
int app_core_start(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CORE_H */

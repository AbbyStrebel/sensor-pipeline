/**
 * @file reporter.h
 * @brief Drains processed samples from the pipeline queue and outputs them
 *        (UART in Phase 2, network in a later phase).
 *
 * Phase 0: stub.
 */
#ifndef REPORTER_H
#define REPORTER_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Start the reporter task. Returns 0 on success. */
int reporter_start(void);

#ifdef __cplusplus
}
#endif

#endif /* REPORTER_H */

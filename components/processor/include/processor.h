/**
 * @file processor.h
 * @brief Pure sample-processing logic. NO ESP-IDF / hardware dependencies.
 *
 * This component is deliberately free of any FreeRTOS or hardware includes so
 * it can be compiled and unit-tested on the host (x86) target and run under
 * valgrind / AddressSanitizer. All hardware interaction lives in the
 * sampler/reporter components, which call into this logic.
 */
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum window size for the moving-average filter. */
#define PROCESSOR_MAX_WINDOW 32U

/** Status codes returned by processor functions. */
typedef enum {
    PROCESSOR_OK = 0,
    PROCESSOR_ERR_NULL_ARG,
    PROCESSOR_ERR_BAD_WINDOW,
} processor_status_t;

/**
 * @brief Moving-average filter state.
 *
 * Fixed-capacity ring buffer so it needs no dynamic allocation — suitable for
 * use inside a FreeRTOS task with a bounded stack.
 *
 * @warning The window size is IMMUTABLE after @ref processor_filter_init. The
 * ring-buffer bookkeeping (head wrap via `% window`, overwrite subtraction from
 * `running_sum`) assumes `window` never changes for the life of the filter. To
 * change the window, call @ref processor_filter_init again to fully re-init
 * (this discards accumulated history). Do NOT mutate `window` in place.
 */
typedef struct {
    float  samples[PROCESSOR_MAX_WINDOW]; /**< ring buffer of recent samples */
    size_t window;                        /**< active window size (1..MAX)   */
    size_t count;                         /**< samples seen (saturates)      */
    size_t head;                          /**< next write index              */
    double running_sum;                   /**< sum of in-window samples      */
} processor_filter_t;

/**
 * @brief Initialise a moving-average filter.
 * @param f       filter to initialise (must not be NULL)
 * @param window  window size, 1..PROCESSOR_MAX_WINDOW
 * @return PROCESSOR_OK on success, error code otherwise.
 */
processor_status_t processor_filter_init(processor_filter_t *f, size_t window);

/**
 * @brief Push one sample and get the current moving average.
 * @param f    initialised filter (must not be NULL)
 * @param in   new sample value
 * @param out  receives the moving average over the current window (must not be NULL)
 * @return PROCESSOR_OK on success, error code otherwise.
 */
processor_status_t processor_filter_push(processor_filter_t *f, float in, float *out);

#ifdef __cplusplus
}
#endif

#endif /* PROCESSOR_H */

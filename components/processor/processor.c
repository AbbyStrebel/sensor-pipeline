/**
 * @file processor.c
 * @brief Implementation of the pure moving-average filter logic.
 */
#include "processor.h"

processor_status_t processor_filter_init(processor_filter_t *f, size_t window)
{
    if (f == NULL) {
        return PROCESSOR_ERR_NULL_ARG;
    }
    if (window == 0U || window > PROCESSOR_MAX_WINDOW) {
        return PROCESSOR_ERR_BAD_WINDOW;
    }

    for (size_t i = 0; i < PROCESSOR_MAX_WINDOW; ++i) {
        f->samples[i] = 0.0F;
    }
    f->window      = window;
    f->count       = 0U;
    f->head        = 0U;
    f->running_sum = 0.0;

    return PROCESSOR_OK;
}

processor_status_t processor_filter_push(processor_filter_t *f, float in, float *out)
{
    if (f == NULL || out == NULL) {
        return PROCESSOR_ERR_NULL_ARG;
    }

    /* If the window is full, subtract the value we are about to overwrite. */
    if (f->count == f->window) {
        f->running_sum -= (double)f->samples[f->head];
    } else {
        f->count += 1U;
    }

    f->samples[f->head] = in;
    f->running_sum += (double)in;
    f->head = (f->head + 1U) % f->window;

    *out = (float)(f->running_sum / (double)f->count);
    return PROCESSOR_OK;
}

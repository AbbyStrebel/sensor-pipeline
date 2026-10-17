/**
 * @file app_core.c
 * @brief Phase 0 stub. Wiring of queues/tasks lands in Phase 2.
 */
#include "app_core.h"

#include "reporter.h"
#include "sampler.h"

int app_core_start(void)
{
    int rc = sampler_start();
    if (rc != 0) {
        return rc;
    }
    return reporter_start();
}

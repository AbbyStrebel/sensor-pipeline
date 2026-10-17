/**
 * @file test_processor.c
 * @brief Host-side Unity tests for the pure processor logic.
 *
 * Runs on the linux target (x86) so it executes under valgrind / ASan in CI.
 */
#include "processor.h"

#include "unity.h"

#include <math.h>
#include <stdlib.h>

void setUp(void) {}
void tearDown(void) {}

TEST_CASE("init rejects NULL and bad windows", "[processor]")
{
    processor_filter_t f;
    TEST_ASSERT_EQUAL(PROCESSOR_ERR_NULL_ARG, processor_filter_init(NULL, 4));
    TEST_ASSERT_EQUAL(PROCESSOR_ERR_BAD_WINDOW, processor_filter_init(&f, 0));
    TEST_ASSERT_EQUAL(PROCESSOR_ERR_BAD_WINDOW,
                      processor_filter_init(&f, PROCESSOR_MAX_WINDOW + 1U));
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_init(&f, 4));
}

TEST_CASE("push rejects NULL args", "[processor]")
{
    processor_filter_t f;
    float              out = 0.0F;
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_init(&f, 4));
    TEST_ASSERT_EQUAL(PROCESSOR_ERR_NULL_ARG, processor_filter_push(NULL, 1.0F, &out));
    TEST_ASSERT_EQUAL(PROCESSOR_ERR_NULL_ARG, processor_filter_push(&f, 1.0F, NULL));
}

TEST_CASE("average before window fills uses count seen so far", "[processor]")
{
    processor_filter_t f;
    float              out = 0.0F;
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_init(&f, 4));

    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_push(&f, 2.0F, &out));
    TEST_ASSERT_FLOAT_WITHIN(1e-5F, 2.0F, out); /* avg of {2} */

    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_push(&f, 4.0F, &out));
    TEST_ASSERT_FLOAT_WITHIN(1e-5F, 3.0F, out); /* avg of {2,4} */
}

TEST_CASE("steady-state average slides over the window", "[processor]")
{
    processor_filter_t f;
    float              out = 0.0F;
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_init(&f, 3));

    const float in[] = {1.0F, 2.0F, 3.0F, 4.0F, 5.0F};
    for (size_t i = 0; i < sizeof(in) / sizeof(in[0]); ++i) {
        TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_push(&f, in[i], &out));
    }
    /* last 3 samples = {3,4,5} -> 4.0 */
    TEST_ASSERT_FLOAT_WITHIN(1e-5F, 4.0F, out);
}

TEST_CASE("window of 1 is a passthrough", "[processor]")
{
    processor_filter_t f;
    float              out = 0.0F;
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_init(&f, 1));
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_push(&f, 42.0F, &out));
    TEST_ASSERT_FLOAT_WITHIN(1e-5F, 42.0F, out);
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_push(&f, -7.0F, &out));
    TEST_ASSERT_FLOAT_WITHIN(1e-5F, -7.0F, out);
}

void app_main(void)
{
    UNITY_BEGIN();
    unity_run_all_tests();
    int failures = UNITY_END();

    /* On the linux target, returning from app_main() hands control back to the
     * simulated FreeRTOS scheduler, which then spins forever in the idle task
     * and the process never terminates. Explicitly exit with the failure count
     * as the status code so the test binary is CI-friendly (0 = pass). */
    exit(failures);
}

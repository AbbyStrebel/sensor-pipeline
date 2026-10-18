/**
 * @file test_processor.c
 * @brief Host-side unit tests for the pure processor logic.
 *
 * Built as a plain native (gcc) binary via the Makefile in this directory —
 * NOT through the ESP-IDF linux target. The `processor` component has zero IDF
 * dependencies, so compiling it natively lets the tests:
 *   - build in well under a second (no IDF component tree),
 *   - produce working gcov coverage data (the IDF linux-target runtime
 *     intercepts filesystem calls and prevents .gcda files being written),
 *   - run cleanly under valgrind.
 *
 * Uses vanilla Unity (RUN_TEST), so it is portable to any host toolchain.
 */
#include "processor.h"

#include "unity.h"

#include <math.h>
#include <stddef.h>

void setUp(void) {}
void tearDown(void) {}

static void test_init_rejects_null_and_bad_windows(void)
{
    processor_filter_t f;
    TEST_ASSERT_EQUAL(PROCESSOR_ERR_NULL_ARG, processor_filter_init(NULL, 4));
    TEST_ASSERT_EQUAL(PROCESSOR_ERR_BAD_WINDOW, processor_filter_init(&f, 0));
    TEST_ASSERT_EQUAL(PROCESSOR_ERR_BAD_WINDOW,
                      processor_filter_init(&f, PROCESSOR_MAX_WINDOW + 1U));
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_init(&f, 4));
}

static void test_push_rejects_null_args(void)
{
    processor_filter_t f;
    float              out = 0.0F;
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_init(&f, 4));
    TEST_ASSERT_EQUAL(PROCESSOR_ERR_NULL_ARG, processor_filter_push(NULL, 1.0F, &out));
    TEST_ASSERT_EQUAL(PROCESSOR_ERR_NULL_ARG, processor_filter_push(&f, 1.0F, NULL));
}

static void test_average_before_window_fills(void)
{
    processor_filter_t f;
    float              out = 0.0F;
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_init(&f, 4));

    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_push(&f, 2.0F, &out));
    TEST_ASSERT_FLOAT_WITHIN(1e-5F, 2.0F, out); /* avg of {2} */

    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_push(&f, 4.0F, &out));
    TEST_ASSERT_FLOAT_WITHIN(1e-5F, 3.0F, out); /* avg of {2,4} */
}

static void test_steady_state_slides_over_window(void)
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

static void test_window_of_one_is_passthrough(void)
{
    processor_filter_t f;
    float              out = 0.0F;
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_init(&f, 1));
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_push(&f, 42.0F, &out));
    TEST_ASSERT_FLOAT_WITHIN(1e-5F, 42.0F, out);
    TEST_ASSERT_EQUAL(PROCESSOR_OK, processor_filter_push(&f, -7.0F, &out));
    TEST_ASSERT_FLOAT_WITHIN(1e-5F, -7.0F, out);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_rejects_null_and_bad_windows);
    RUN_TEST(test_push_rejects_null_args);
    RUN_TEST(test_average_before_window_fills);
    RUN_TEST(test_steady_state_slides_over_window);
    RUN_TEST(test_window_of_one_is_passthrough);
    return UNITY_END();
}

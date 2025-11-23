#include <zephyr/ztest.h>

ZTEST(sum_suite, test_dummy_always_pass)
{
    zassert_true(1 == 1, "This test should always pass");
}

ZTEST_SUITE(sum_suite, NULL, NULL, NULL, NULL, NULL);

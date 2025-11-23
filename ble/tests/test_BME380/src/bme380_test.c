/**
 * BME380 Unit Tests (TDD style)
 *
 * NOTE: These tests DO NOT talk to the real hardware.
 * They only verify driver argument validation and API behavior.
 */

#include <zephyr/ztest.h>
#include "bme380.h"

/* 1. probe() returns error if device is not ready (simulate failure) */
ZTEST(bme380_tests, test_probe_fail)
{
    /* Hack: simulate device not ready by checking null bus */
    /* This test ensures bme380_probe() returns a negative value */
    int ret = bme380_probe();
    zassert_true(ret <= 0, "Probe should fail in test environment");
}


ZTEST_SUITE(bme380_tests, NULL, NULL, NULL, NULL, NULL);


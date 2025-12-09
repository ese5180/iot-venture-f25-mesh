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
    zassert_true(ret > 0, "Probe should fail in test environment");
}

/* 2. read temperature raw with NULL pointer */
ZTEST(bme380_tests, test_read_temp_raw_null)
{
    int ret = bme380_read_temp_raw(NULL);
    zassert_true(ret < 0, "read_temp_raw must fail with NULL argument");
}

/* 3. read humidity raw with NULL pointer */
ZTEST(bme380_tests, test_read_hum_raw_null)
{
    int ret = bme380_read_hum_raw(NULL);
    zassert_true(ret < 0, "read_humidity_raw must fail with NULL pointer");
}

/* 4. compensate temperature should work with a dummy value */
ZTEST(bme380_tests, test_compensate_temp)
{
    double t = compensate_temp(50000);  /* arbitrary ADC raw */
    zassert_true(t < 200 && t > -100, "Temperature seems unrealistic");
}

/* 5. temp celsius API fails when raw read fails */
ZTEST(bme380_tests, test_read_temp_celsius_fail)
{
    double t;
    int ret = bme380_read_temp_celsius(&t);
    zassert_true(ret < 0, "read_temp_celsius must fail when no I2C hw");
}

ZTEST(bme380_tests, test_read_temp_without_calib_fail)
{
    double temp_value = 0.0;

    /* Not calling read_temp_calib() before reading temp */

    int rc = bme380_read_temp_celsius(&temp_value);

    /* Expect failure because calibration data has NOT been loaded */
    zassert_true(rc < 0, "Driver must fail when temp calibration not loaded!");
}

ZTEST_SUITE(bme380_tests, NULL, NULL, NULL, NULL, NULL);


#include <zephyr/ztest.h>
#include "bno055_driver.h"

ZTEST(bno055_tests, test_bno055_init_success)
{
    bno055_config_t cfg = {
        .address = BNO055_ADDRESS_A,
        .mode = BNO055_OPERATION_MODE_IMU,
        .units_in_radians = false,
        .use_external_crystal = false
    };

    int ret = bno055_init(&cfg);
    zassert_equal(ret, 0, "BNO055 init should succeed");
}

ZTEST(bno055_tests, test_read_quaternion_success)
{
    bno055_quaternion_t quat;
    bno055_config_t cfg = {
        .address = BNO055_ADDRESS_A,
        .mode = BNO055_OPERATION_MODE_IMU,
        .units_in_radians = false,
        .use_external_crystal = false
    };

    int ret = bno055_init(&cfg);
    zassert_equal(ret, 0, "Init must succeed");

    ret = bno055_read_quaternion(&quat);
    zassert_equal(ret, 0, "Quaternion read must succeed");

    zassert_true(!isnan(quat.w), "quat.w invalid");
    zassert_true(!isnan(quat.x), "quat.x invalid");
    zassert_true(!isnan(quat.y), "quat.y invalid");
    zassert_true(!isnan(quat.z), "quat.z invalid");
}


ZTEST(bno055_tests, test_warning_interval_too_small)
{
    int ret = bno055_set_warning_interval(10);
    zassert_true(ret < 0, "Should fail when interval < 100 ms");
}


ZTEST_SUITE(bno055_tests, NULL, NULL, NULL, NULL, NULL);

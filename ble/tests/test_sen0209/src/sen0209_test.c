#include <zephyr/ztest.h>
#include "sen0209.h"

ZTEST_SUITE(sen0209_tests, NULL, NULL, NULL, NULL, NULL);

/* 1. Test initialization with valid config */
ZTEST(sen0209_tests, test_init_valid)
{
    struct sen0209_instance inst;
    struct sen0209_config cfg = {
        .adc_channel = 1,
        .threshold = 500,
        .sample_interval_ms = 10
    };

    int ret = sen0209_init(&inst, &cfg);
    zassert_equal(ret, 0, "Init should succeed");
    zassert_equal(inst.adc_channel, cfg.adc_channel, "Channel mismatch");
    zassert_equal(inst.threshold, cfg.threshold, "Threshold mismatch");
}

/* 2. Test initialization fails with NULL */
ZTEST(sen0209_tests, test_init_null)
{
    struct sen0209_instance inst;
    int ret = sen0209_init(&inst, NULL);
    zassert_not_equal(ret, 0, "Init must fail with NULL config");
}

/* 3. Test set/get threshold */
ZTEST(sen0209_tests, test_threshold_set_get)
{
    struct sen0209_instance inst;
    struct sen0209_config cfg = {.adc_channel=1,.threshold=400,.sample_interval_ms=10};

    sen0209_init(&inst, &cfg);
    sen0209_set_threshold(&inst, 600);

    uint16_t t = sen0209_get_threshold(&inst);
    zassert_equal(t, 600, "Threshold get/set mismatch");
}

/* 4. Test read_raw() null pointer */
ZTEST(sen0209_tests, test_read_raw_null)
{
    struct sen0209_instance inst;
    struct sen0209_config cfg = {.adc_channel=1,.threshold=400,.sample_interval_ms=10};
    sen0209_init(&inst, &cfg);

    int ret = sen0209_read_raw(&inst, NULL);
    zassert_not_equal(ret, 0, "read_raw must fail with NULL");
}

/* 5. Test get_data() null pointer */
ZTEST(sen0209_tests, test_get_data_null)
{
    struct sen0209_instance inst;
    struct sen0209_config cfg = {.adc_channel=1,.threshold=400,.sample_interval_ms=10};
    sen0209_init(&inst, &cfg);

    int ret = sen0209_get_data(&inst, NULL);
    zassert_not_equal(ret, 0, "get_data must fail with NULL");
}

/* 6. Test init fails if sample interval is zero */
ZTEST(sen0209_tests, test_init_interval_zero_fail)
{
    struct sen0209_instance inst;

    struct sen0209_config cfg = {
        .adc_channel = 1,
        .threshold = 400,
        .sample_interval_ms = 0   /* illegal value */
    };

    int ret = sen0209_init(&inst, &cfg);
    zassert_true(ret < 0, "Init must fail when sample_interval_ms = 0");
}

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>

#include "bme380.h"
#include "sen0209.h"

LOG_MODULE_REGISTER(bme_app, LOG_LEVEL_INF);

#define BT_UUID_SVC_VAL  BT_UUID_128_ENCODE(0x12345678,0x1234,0x1234,0x1234,0x1234567890ab)//8-4-4-4-12 to get 128 bit
#define BT_UUID_TMP_VAL  BT_UUID_128_ENCODE(0xabcdef01,0x1234,0x1234,0x1234,0x1234567890ab)//Temperature UUID
#define BT_UUID_HUMIDITY_VAL  BT_UUID_128_ENCODE(0xabcdef11,0x1234,0x1234,0x1234,0x1234567890ab)//Humidity UUID
#define BT_UUID_PRESSURE_VAL  BT_UUID_128_ENCODE(0xabcdef21,0x1234,0x1234,0x1234,0x1234567890ab)//Pressure UUID

#define ADC_DEVICE_NODE       DT_NODELABEL(adc)
#define ADC_RESOLUTION        12
#define ADC_GAIN              ADC_GAIN_1_6
#define ADC_REFERENCE         ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME  ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)

#define VIBRATION_THRESHOLD   500    // 振动阈值，根据实际情况调整
#define DETECTION_COUNT       20     // 检测次数阈值
#define SAMPLE_INTERVAL_MS    100    // 采样间隔（毫秒）
#define MAX_PRESSURE_N        1000    // 最大压力值（牛顿）

static struct bt_uuid_128 svc_uuid  = BT_UUID_INIT_128(BT_UUID_SVC_VAL);// 16 Byte
static struct bt_uuid_128 temp_uuid = BT_UUID_INIT_128(BT_UUID_TMP_VAL);
static struct bt_uuid_128 humidity_uuid = BT_UUID_INIT_128(BT_UUID_HUMIDITY_VAL);
static struct bt_uuid_128 pressure_uuid = BT_UUID_INIT_128(BT_UUID_PRESSURE_VAL);//For pressure sensor

static uint8_t notify_enabled;

// const struct bt_data ad[] = {
//     BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
//     BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_SVC_VAL),
// };

/**ADC */
static const struct device *adc_dev;
static int16_t sample_buf[2];  // 存储两个通道的数据
static int vibration_count_left = 0;   // 左侧振动计数 (通道0)
static int vibration_count_right = 0;  // 右侧振动计数 (通道1)
static bool grinding_left = false;   // 左侧磨牙检测标志
static bool grinding_right = false;  // 右侧磨牙检测标志
static int grinding_duration_left = 0;   // 左侧磨牙持续时间（次数）
static int grinding_duration_right = 0;  // 右侧磨牙持续时间（次数）
static int last_grinding_time_left_ms = 0;   // 上次左侧磨牙时长（毫秒）
static int last_grinding_time_right_ms = 0;  // 上次右侧磨牙时长（毫秒）
static int total_grinding_time_left_ms = 0;   // 左侧累计磨牙时长（毫秒）
static int total_grinding_time_right_ms = 0;  // 右侧累计磨牙时长（毫秒）
static int grinding_episodes_left = 0;   // 左侧磨牙次数
static int grinding_episodes_right = 0;  // 右侧磨牙次数
static int grinding_total_count = 0;     // 总磨牙次数
// 通道0配置 (AIN0 = P0.04) - 左侧传感器
static struct adc_channel_cfg ch0_cfg = {
    .gain             = ADC_GAIN,
    .reference        = ADC_REFERENCE,
    .acquisition_time = ADC_ACQUISITION_TIME,
    .channel_id       = 0,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
    .input_positive   = SAADC_CH_PSELP_PSELP_AnalogInput0, // AIN0 = P0.04
#endif
};

// 通道1配置 (AIN1 = P0.05) - 右侧传感器
static struct adc_channel_cfg ch1_cfg = {
    .gain             = ADC_GAIN,
    .reference        = ADC_REFERENCE,
    .acquisition_time = ADC_ACQUISITION_TIME,
    .channel_id       = 1,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
    .input_positive   = SAADC_CH_PSELP_PSELP_AnalogInput1, // AIN1 = P0.05
#endif
};

static struct adc_sequence seq = {
    .buffer       = sample_buf,
    .buffer_size  = sizeof(sample_buf),
    .resolution   = ADC_RESOLUTION,
    .channels     = BIT(0) | BIT(1), 
};



//0100 means notify enable
static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

//temperature->attrs[2], humidity->attrs[5], pressure->attrs[8]
BT_GATT_SERVICE_DEFINE(bme_svc,
    BT_GATT_PRIMARY_SERVICE(&svc_uuid),
    /**Temperature */
    BT_GATT_CHARACTERISTIC(&temp_uuid.uuid,
        BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    // /**Humidity */
    BT_GATT_CHARACTERISTIC(&humidity_uuid.uuid,
        BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    // /**Pressure */
    BT_GATT_CHARACTERISTIC(&pressure_uuid.uuid,
        BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static void ble_start(void)
{
    int err = bt_enable(NULL);
    if (err) { LOG_ERR("bt_enable failed (%d)", err); return; }
    LOG_INF("Bluetooth enabled");
    //It can be found and is set to be BLE Mode 
    const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_SVC_VAL),
    };

    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising start failed (%d)", err);
    } else {
        LOG_INF("Advertising started");
    }
}

// static int adv_start(void)
// {
//     int err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
//     if (err && err != -EALREADY) {
//         LOG_ERR("Advertising start failed (%d)", err);
//         return err;
//     }
//     LOG_INF("Advertising started (or already running)");
//     return 0;
// }

// /* =========== Connect callback =========== */ // >>> NEW
// static void connected(struct bt_conn *conn, uint8_t err)
// {
//     if (err) {
//         LOG_ERR("Connection failed (err %u)", err);
//         return;
//     }
//     char addr[BT_ADDR_LE_STR_LEN];
//     bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
//     LOG_INF("Connected: %s", addr);

//     /* 继续广播，允许第二台设备连接 */
//     int ret = adv_start();
//     if (ret) {
//         LOG_WRN("Re-start adv after connect failed: %d", ret);
//     }
// }

// /* =========== Disconnect Callback =========== */                     // >>> NEW
// static void disconnected(struct bt_conn *conn, uint8_t reason)
// {
//     char addr[BT_ADDR_LE_STR_LEN];
//     bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
//     LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

//     /* 恢复广播 */
//     int ret = adv_start();
//     if (ret) {
//         LOG_WRN("Re-start adv after disconnect failed: %d", ret);
//     }
// }

// /* 在静态区注册回调 */                                                 // >>> NEW
// BT_CONN_CB_DEFINE(conn_cbs) = {
//     .connected = connected,
//     .disconnected = disconnected,
// };

// /* =========== 初始化 BLE：enable 后就启动一次广播 =========== */        // >>> CHANGED
// static void ble_start(void)
// {
//     int err = bt_enable(NULL);
//     if (err) {
//         LOG_ERR("bt_enable failed (%d)", err);
//         return;
//     }
//     LOG_INF("Bluetooth enabled");

//     (void)adv_start();   // 第一次启动广播
// }

/**ADC Functions */
static int convert_raw_to_pressure(int16_t raw_value)
{
    int pressure;
    pressure = (int)(raw_value * 0.1);
    if (pressure > MAX_PRESSURE_N)  pressure = MAX_PRESSURE_N;
    if (pressure < 0)   pressure = 0;
    return pressure;
}

static int adc_init(void)
{
    int err;

    adc_dev = DEVICE_DT_GET(ADC_DEVICE_NODE);
    if (!device_is_ready(adc_dev)) {
        LOG_INF("ADC device not ready\n");
        return -ENODEV;
    }

    err = adc_channel_setup(adc_dev, &ch0_cfg);
    if (err) {
        LOG_INF("adc_channel_setup ch0 failed: %d\n", err);
        return err;
    }

    err = adc_channel_setup(adc_dev, &ch1_cfg);
    if (err) {
        LOG_INF("adc_channel_setup ch1 failed: %d\n", err);
        return err;
    }
    return 0;
}

int main(void)
{
    int rc;
    int pressure_left, pressure_right;

    rc = bme380_probe();
    if (rc) { LOG_ERR("I2C/BME not ready (%d)", rc); return rc; }
    rc = read_temp_calib();
    if (rc) { LOG_ERR("Temp calib read fail (%d)", rc); return rc; }
    rc = read_hum_calib();
    if (rc) { LOG_ERR("Humidity calib read fail (%d)", rc); return rc; }
    rc = set_ctrl_hum();
    if (rc) { LOG_ERR("ctrl_meas hum fail (%d)", rc); return rc; }
    rc = set_ctrl_meas();
    if (rc) { LOG_ERR("ctrl_meas fail (%d)", rc); return rc; }

    rc = adc_init();
    if (rc) { LOG_ERR("ADC init failed: (%d)", rc); return rc; }

    ble_start();

	while(1){
        double temp_c, humidity;

        rc = bme380_read_temp_celsius(&temp_c);
        rc = bme380_read_humidity(&humidity);
        rc = adc_read(adc_dev, &seq);//ADC

        char str_temp[16], str_hum[16], str_press[22];
        /**ADC got Functions*/
        if (rc) {
            LOG_ERR("adc_read error: (%d)", rc);
        }else{
            pressure_left = convert_raw_to_pressure(sample_buf[0]);
            pressure_right = convert_raw_to_pressure(sample_buf[1]);
            // 检测左侧振动
            if (sample_buf[0] > VIBRATION_THRESHOLD)    vibration_count_left++;
            else    vibration_count_left = 0;  
            // 检测右侧振动
            if (sample_buf[1] > VIBRATION_THRESHOLD)    vibration_count_right++;
            else    vibration_count_right = 0;                        
            // 左侧磨牙检测
            if (vibration_count_left >= DETECTION_COUNT && !grinding_left) {
                grinding_left = true;
                grinding_duration_left = 0;
                grinding_episodes_left++;//左侧磨牙次数
                LOG_INF(">>> 左侧检测到磨牙！(第%d次) <<<", grinding_episodes_left);
            } else if (vibration_count_left < DETECTION_COUNT && grinding_left) {
                grinding_left = false;
                last_grinding_time_left_ms = grinding_duration_left * SAMPLE_INTERVAL_MS;
                total_grinding_time_left_ms += last_grinding_time_left_ms;
                
                int sec = last_grinding_time_left_ms / 1000;
                int ms = last_grinding_time_left_ms % 1000;
                int total_sec = total_grinding_time_left_ms / 1000;
                int total_ms = total_grinding_time_left_ms % 1000;
                
                LOG_INF(">>> 左侧磨牙停止 - 持续: %d.%03d秒 (累计: %d.%03d秒) <<<", 
                       sec, ms, total_sec, total_ms);
                grinding_duration_left = 0;
            }
            // 右侧磨牙检测
            if (vibration_count_right >= DETECTION_COUNT && !grinding_right) {
                grinding_right = true;
                grinding_duration_right = 0;
                grinding_episodes_right++;//右侧磨牙次数
                LOG_INF(">>> 右侧检测到磨牙！(第%d次) <<<", grinding_episodes_right);
            } else if (vibration_count_right < DETECTION_COUNT && grinding_right) {
                grinding_right = false;
                last_grinding_time_right_ms = grinding_duration_right * SAMPLE_INTERVAL_MS;
                total_grinding_time_right_ms += last_grinding_time_right_ms;
                
                int sec = last_grinding_time_right_ms / 1000;
                int ms = last_grinding_time_right_ms % 1000;
                int total_sec = total_grinding_time_right_ms / 1000;
                int total_ms = total_grinding_time_right_ms % 1000;
                
                LOG_INF(">>> 右侧磨牙停止 - 持续: %d.%03d秒 (累计: %d.%03d秒) <<<", 
                       sec, ms, total_sec, total_ms);
                grinding_duration_right = 0;
            }
            // 如果正在磨牙，增加持续时间计数
            if (grinding_left) {
                grinding_duration_left++;
            }
            if (grinding_right) {
                grinding_duration_right++;
            }
            grinding_total_count = grinding_episodes_left + grinding_episodes_right;//total count for bruxism
        }

        if(notify_enabled){
            int len = snprintf(str_temp, sizeof(str_temp), "T=%.2f", temp_c);
            bt_gatt_notify(NULL, &bme_svc.attrs[2], str_temp, len); //temperature->attrs[2], humidity->attrs[5], pressure->attrs[8]
            LOG_INF("T = %.2f C", temp_c);

            len = snprintf(str_hum, sizeof(str_hum), "RH=%.2f", humidity);
            bt_gatt_notify(NULL, &bme_svc.attrs[5], str_hum, len); //temperature->attrs[2], humidity->attrs[5], pressure->attrs[8]
            LOG_INF("RH = %.2f %%", humidity);

            len = snprintf(str_press, sizeof(str_press), "left=%d, right=%d", grinding_episodes_left, grinding_episodes_right);
            bt_gatt_notify(NULL, &bme_svc.attrs[8], str_press, len); //temperature->attrs[2], humidity->attrs[5], pressure->attrs[8]
            LOG_INF("Total Count = %d times", grinding_total_count);
            LOG_INF("Pressure_L = %d N", pressure_left);
            LOG_INF("Pressure_R = %d N", pressure_right);
        }
		k_msleep(SAMPLE_INTERVAL_MS);
	}
    return 0;
}

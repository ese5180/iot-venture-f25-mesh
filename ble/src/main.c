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
#include <zephyr/sys/byteorder.h>

#include "bme380.h"

LOG_MODULE_REGISTER(bme_app, LOG_LEVEL_INF);

#define BT_UUID_SVC_VAL  BT_UUID_128_ENCODE(0x12345678,0x1234,0x1234,0x1234,0x1234567890ab)//8-4-4-4-12 to get 128 bit
#define BT_UUID_TMP_VAL  BT_UUID_128_ENCODE(0xabcdef01,0x1234,0x1234,0x1234,0x1234567890ab)//Temperature UUID
// #define BT_UUID_PRESSURE_VAL  BT_UUID_128_ENCODE(0xabcdef11,0x1234,0x1234,0x1234,0x1234567890ab)//Pressure UUID
#define BT_UUID_HUMIDITY_VAL  BT_UUID_128_ENCODE(0xabcdef21,0x1234,0x1234,0x1234,0x1234567890ab)//Humidity UUID

static struct bt_uuid_128 svc_uuid  = BT_UUID_INIT_128(BT_UUID_SVC_VAL);// 16 Byte
static struct bt_uuid_128 temp_uuid = BT_UUID_INIT_128(BT_UUID_TMP_VAL);
// static struct bt_uuid_128 pressure_uuid = BT_UUID_INIT_128(BT_UUID_PRESSURE_VAL);
static struct bt_uuid_128 humidity_uuid = BT_UUID_INIT_128(BT_UUID_HUMIDITY_VAL);

static uint8_t notify_enabled;

//0100 means notify enable
static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}


BT_GATT_SERVICE_DEFINE(bme_svc,
    BT_GATT_PRIMARY_SERVICE(&svc_uuid),
    /**Temperature */
    BT_GATT_CHARACTERISTIC(&temp_uuid.uuid,
        BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    // /**Pressure */
    // BT_GATT_CHARACTERISTIC(&pressure_uuid.uuid,
    //     BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    // BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    // /**Humidity */
    BT_GATT_CHARACTERISTIC(&humidity_uuid.uuid,
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


int main(void)
{
    int rc;

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

    ble_start();

	while(1){
        double temp_c, humidity;
        rc = bme380_read_temp_celsius(&temp_c);
        rc = bme380_read_humidity(&humidity);
        char str_temp[8], str_hum[8];
        if(notify_enabled){
            snprintf(str_temp, sizeof(str_temp), "%.2f", temp_c);
            bt_gatt_notify(NULL, &bme_svc.attrs[2], str_temp, sizeof(str_temp)); //temperature->attrs[2], pressure->attrs[5], humidity->attrs[8]
            LOG_INF("T = %.2f C", temp_c);

            snprintf(str_hum, sizeof(str_hum), "%.2f", humidity);
            bt_gatt_notify(NULL, &bme_svc.attrs[5], str_hum, sizeof(str_hum)); //temperature->attrs[2], pressure->attrs[5], humidity->attrs[8]
            LOG_INF("RH = %.2f %%", humidity);
        }
		k_msleep(2000);
	}
    return 0;
}

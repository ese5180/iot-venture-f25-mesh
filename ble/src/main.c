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

/* The devicetree node identifier for the "led0" alias. */
// #define LED0_NODE DT_ALIAS(led0)

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

LOG_MODULE_REGISTER(bme_app, LOG_LEVEL_INF);

#define MYBME_NODE DT_NODELABEL(mybme)
static const struct i2c_dt_spec bme = I2C_DT_SPEC_GET(MYBME_NODE);

#define REG_CTRL_MEAS 0xF4
#define REG_TEMP_MSB 0xFA
#define REG_CALIB_T1 0x88

static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;

static int read_temp_calib(void){
	uint8_t buf[6];
	int rc = i2c_burst_read_dt(&bme, REG_CALIB_T1, buf, sizeof(buf));
	if(rc) return rc;

    dig_T1 = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    dig_T2 = (int16_t)((uint16_t)buf[2] | ((uint16_t)buf[3] << 8));
    dig_T3 = (int16_t)((uint16_t)buf[4] | ((uint16_t)buf[5] << 8));
    return 0;
}

static int set_ctrl_meas(void)
{
    uint8_t tx[2] = { REG_CTRL_MEAS, 0x27 };
    return i2c_write_dt(&bme, tx, sizeof(tx));
}

static double compensate_temp(int32_t adc_T)
{
    double var1 = ((adc_T / 16384.0) - (dig_T1 / 1024.0)) * dig_T2;
    double var2 = (((adc_T / 131072.0) - (dig_T1 / 8192.0)) *
                   ((adc_T / 131072.0) - (dig_T1 / 8192.0))) * dig_T3;
    double t_fine = var1 + var2;
    return t_fine / 5120.0;
}

int main(void)
{
    if (!device_is_ready(bme.bus)) {
        LOG_ERR("I2C bus not ready");
        return;
    }

    if (read_temp_calib()) {
        LOG_ERR("Read calib failed");
        return;
    }

    if (set_ctrl_meas()) {
        LOG_ERR("CTRL_MEAS write failed");
        return;
    }

	while(1){
		uint8_t raw[3];
		int rc = i2c_burst_read_dt(&bme, REG_TEMP_MSB, raw, sizeof(raw));
		if(rc){
			LOG_ERR("Temp read failed (%d)", rc);
		}
		else{
            int32_t adc_T = ((int32_t)raw[0] << 12) |
                            ((int32_t)raw[1] << 4)  |
                            ((int32_t)raw[2] >> 4);
            double temp_c = compensate_temp(adc_T);
            LOG_INF("T = %.2f C", temp_c);
		}
		k_msleep(2000);
	}
}

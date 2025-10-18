#include <zephyr/kernel.h>
#include "bme380.h"

#define REG_CTRL_MEAS 0xF4
#define REG_TEMP_MSB 0xFA
#define REG_CALIB_T1 0x88

static const struct i2c_dt_spec bme = I2C_DT_SPEC_GET(MYBME_NODE);

static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;

int bme380_probe(void)
{
    return device_is_ready(bme.bus) ? 0 : -ENODEV;
}

/*Get raw value of temperature*/
int read_temp_calib(void){
	uint8_t buf[6];
	int rc = i2c_burst_read_dt(&bme, REG_CALIB_T1, buf, sizeof(buf));
	if(rc) return rc;

    dig_T1 = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    dig_T2 = (int16_t)((uint16_t)buf[2] | ((uint16_t)buf[3] << 8));
    dig_T3 = (int16_t)((uint16_t)buf[4] | ((uint16_t)buf[5] << 8));
    return 0;
}

/*Set status 001 001 11*/
int set_ctrl_meas(void)
{
    uint8_t tx[2] = { REG_CTRL_MEAS, 0x27 };// x1  x1  normal
    return i2c_write_dt(&bme, tx, sizeof(tx));
}

/***Detect Temperature and Measure Temperature *****/

/*Compensation formula(Got from datasheet)*/
double compensate_temp(int32_t adc_T)
{
    double var1 = ((adc_T / 16384.0) - (dig_T1 / 1024.0)) * dig_T2;
    double var2 = (((adc_T / 131072.0) - (dig_T1 / 8192.0)) *
                   ((adc_T / 131072.0) - (dig_T1 / 8192.0))) * dig_T3;
    double t_fine = var1 + var2;
    return t_fine / 5120.0;
}

/*Read raw temperature and store value into adc_T*/
int bme380_read_temp_raw(int32_t *adc_T) {
    uint8_t raw[3];
    int rc = i2c_burst_read_dt(&bme, REG_TEMP_MSB, raw, sizeof(raw));
    if (rc) return rc;
    *adc_T = ((int32_t)raw[0] << 12) |
             ((int32_t)raw[1] << 4)  |
             ((int32_t)raw[2] >> 4);
    return 0;
}

/*Raw value to accurate value(from compemsation formula)*/
int bme380_read_temp_celsius(double *celsius) {
    int32_t adc_T;
    int rc = bme380_read_temp_raw(&adc_T);
    if (rc) return rc;
    *celsius = compensate_temp(adc_T);
    return 0;
}
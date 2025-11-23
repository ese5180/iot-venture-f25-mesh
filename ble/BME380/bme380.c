#include <zephyr/kernel.h>
#include "bme380.h"

/**Temperature */
#define REG_CTRL_MEAS 0xF4
#define REG_TEMP_MSB 0xFA
#define REG_CALIB_T1 0x88

/**Humidity */
#define REG_CTRL_HUM  0xF2 //Set after REG_CTRL_MEAS has set
#define REG_HUM_MSB   0xFD
#define REG_CALIB_H1  0xA1
#define REG_CALIB_H2  0xE1


static const struct i2c_dt_spec bme = I2C_DT_SPEC_GET(MYBME_NODE);

static int32_t t_fine;

static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;

static uint8_t  dig_H1, dig_H3;
static int16_t  dig_H2, dig_H4, dig_H5;
static int8_t   dig_H6;

int bme380_probe(void)
{
    return device_is_ready(bme.bus) ? 0 : -ENODEV;
}

/******************* Temperature ********************/
/*Get temperature calibration data*/
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
    double T, var1, var2;
    var1 = (((double)adc_T / 16384.0) - ((double)dig_T1 / 1024.0)) * (double)dig_T2;
    var2 = ((((double)adc_T / 131072.0) - ((double)dig_T1 / 8192.0)) *
                   (((double)adc_T / 131072.0) - ((double)dig_T1 / 8192.0))) * (double)dig_T3;
    t_fine = (int32_t)(var1 + var2);
    T = (var1 + var2)/5120.0;
    return T;
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



/*************************Humidity******************************/
/*Get humidity calibration data*/
int read_hum_calib(void){
    uint8_t buf1[1];
    uint8_t buf2[7];
    int rc;
    
    rc = i2c_burst_read_dt(&bme, REG_CALIB_H1, buf1, sizeof(buf1));
    if(rc) return rc;
    dig_H1 = buf1[0];

    rc = i2c_burst_read_dt(&bme, REG_CALIB_H2, buf2, sizeof(buf2));
    if(rc) return rc;

    dig_H2 = (int16_t)((buf2[1] << 8) | buf2[0]);
    dig_H3 = buf2[2];
    dig_H4 = (int16_t)((buf2[3] << 4) | (buf2[4] & 0x0F));
    dig_H5 = (int16_t)((buf2[4] >> 4) | (buf2[5] << 4));
    dig_H6 = (int8_t)buf2[6];

    return 0;
}

/** Humidity oversampling setting(x1) */
int set_ctrl_hum(void){
    uint8_t tx[2] = { REG_CTRL_HUM, 0x01 };
    return i2c_write_dt(&bme, tx, sizeof(tx));
}

/** Read raw humidity */
int bme380_read_hum_raw(int32_t *adc_H){
    uint8_t raw[2];
    int rc = i2c_burst_read_dt(&bme, REG_HUM_MSB, raw, sizeof(raw));
    if(rc) return rc;
    *adc_H = ((int32_t)raw[0] << 8) | raw[1];
    return 0;
}

/** Compensation formula and get final value */
int bme380_read_humidity(double *humidity){
    int32_t adc_H;
    int rc = bme380_read_hum_raw(&adc_H);
    if(rc) return rc;

    double var_H;

    var_H = (double)t_fine - 76800.0;
    var_H = (adc_H - ((double)dig_H4 * 64.0 + ((double)dig_H5 / 16384.0) * var_H)) *
            ((double)dig_H2 / 65536.0 * (1.0 + ((double)dig_H6 / 67108864.0) * var_H *(1.0 + ((double)dig_H3 / 67108864.0) * var_H)));
    var_H = var_H * (1.0 - (double)dig_H1 * var_H / 524288.0);
    if(var_H > 100.0)   var_H = 100.0;
    else if(var_H < 0.0)    var_H = 0.0;
    *humidity = var_H;
    return 0;
}
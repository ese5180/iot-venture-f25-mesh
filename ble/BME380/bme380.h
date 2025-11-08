#pragma once
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>

#define MYBME_NODE DT_NODELABEL(mybme)

int bme380_probe(void);

int read_temp_calib(void);
int set_ctrl_meas(void);
double compensate_temp(int32_t adc_T);
int bme380_read_temp_raw(int32_t *adc_T);
int bme380_read_temp_celsius(double *celsius);

int read_hum_calib(void);
int set_ctrl_hum(void);
int bme380_read_hum_raw(int32_t *adc_H);
int bme380_read_humidity(double *humidity);

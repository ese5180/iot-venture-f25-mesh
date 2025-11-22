/**
 * @file sen0209.c
 * @brief Driver implementation for SEN0209 Piezo Vibration Sensor
 * 
 * This file implements the driver functions for the DFRobot SEN0209
 * Flexible Piezo Film Vibration Sensor with multi-instance support.
 * Multiple sensors can be used simultaneously on different ADC channels.
 * 
 * Hardware Connection:
 * - VCC: 5V power supply
 * - GND: Ground
 * - Signal: Connected to ADC input (e.g., P0.05/AIN1, P0.06/AIN2, etc.)
 * 
 * The sensor generates lower voltage values when vibration is detected.
 */

#include "sen0209.h"
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <hal/nrf_saadc.h>

/* ADC Configuration Constants */
#define ADC_DEVICE_NODE       DT_NODELABEL(adc)
#define ADC_RESOLUTION        12                /**< 12-bit ADC resolution (0-4095) */
#define ADC_GAIN              ADC_GAIN_1_6      /**< ADC gain setting */
#define ADC_REFERENCE         ADC_REF_INTERNAL  /**< Use internal reference voltage */
#define ADC_ACQUISITION_TIME  ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)  /**< 10us acquisition time */

/**
 * @brief Helper function to get ADC input configuration based on channel
 * 
 * @param channel ADC channel number (0-7)
 * @return ADC input configuration value
 */
static uint8_t get_adc_input_config(uint8_t channel)
{
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	switch (channel) {
		case 0: return SAADC_CH_PSELP_PSELP_AnalogInput0;
		case 1: return SAADC_CH_PSELP_PSELP_AnalogInput1;
		case 2: return SAADC_CH_PSELP_PSELP_AnalogInput2;
		case 3: return SAADC_CH_PSELP_PSELP_AnalogInput3;
		case 4: return SAADC_CH_PSELP_PSELP_AnalogInput4;
		case 5: return SAADC_CH_PSELP_PSELP_AnalogInput5;
		case 6: return SAADC_CH_PSELP_PSELP_AnalogInput6;
		case 7: return SAADC_CH_PSELP_PSELP_AnalogInput7;
		default: return SAADC_CH_PSELP_PSELP_AnalogInput1;
	}
#else
	return 0;
#endif
}

/**
 * @brief Initialize a SEN0209 vibration sensor instance
 * 
 * This function performs the following:
 * 1. Validates the configuration parameters
 * 2. Gets the ADC device handle
 * 3. Configures the ADC channel for this instance
 * 4. Stores configuration for later use
 * 
 * @param instance Pointer to sensor instance structure
 * @param config Pointer to sensor configuration structure
 * @return 0 on success, negative errno code on failure
 *         -EINVAL: Invalid configuration or instance pointer
 *         -ENODEV: ADC device not ready
 *         Other: ADC channel setup error code
 */
int sen0209_init(struct sen0209_instance *instance, const struct sen0209_config *config)
{
	int err;

	/* Validate input parameters */
	if (instance == NULL || config == NULL) {
		printk("SEN0209: Invalid instance or config parameter\n");
		return -EINVAL;
	}

	// todo
	/* Validate sample interval */
	// if (config->sample_interval_ms == 0) {
    //     return -EINVAL;
    // }

	/* Store configuration parameters in instance */
	instance->adc_channel = config->adc_channel;
	instance->threshold = config->threshold;

	/* Get ADC device handle */
	instance->adc_dev = DEVICE_DT_GET(ADC_DEVICE_NODE);
	if (!device_is_ready(instance->adc_dev)) {
		printk("SEN0209: ADC device not ready\n");
		return -ENODEV;
	}

	/* Configure ADC channel for this instance */
	instance->channel_cfg.gain = ADC_GAIN;
	instance->channel_cfg.reference = ADC_REFERENCE;
	instance->channel_cfg.acquisition_time = ADC_ACQUISITION_TIME;
	instance->channel_cfg.channel_id = config->adc_channel;
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	instance->channel_cfg.input_positive = get_adc_input_config(config->adc_channel);
#endif

	/* Setup ADC sequence for this instance */
	instance->sequence.buffer = &instance->sample_buffer;
	instance->sequence.buffer_size = sizeof(instance->sample_buffer);
	instance->sequence.resolution = ADC_RESOLUTION;
	instance->sequence.channels = BIT(config->adc_channel);

	/* Setup ADC channel with specified configuration */
	err = adc_channel_setup(instance->adc_dev, &instance->channel_cfg);
	if (err) {
		printk("SEN0209: ADC channel setup failed with error %d\n", err);
		return err;
	}

	/* Print initialization success message */
	printk("SEN0209: Sensor initialized successfully\n");
	printk("  - ADC Channel: %d\n", config->adc_channel);
	printk("  - Threshold: %d\n", config->threshold);
	printk("  - Sample Interval: %d ms\n", config->sample_interval_ms);

	return 0;
}

/**
 * @brief Read raw ADC value from sensor instance
 * 
 * Performs a blocking ADC read operation and returns the raw
 * digital value. Lower values indicate stronger vibration.
 * 
 * @param instance Pointer to sensor instance
 * @param value Pointer to store the raw ADC reading
 * @return 0 on success, negative errno code on failure
 *         -EINVAL: Invalid instance or value pointer
 *         -ENODEV: ADC device not initialized
 *         Other: ADC read error code
 */
int sen0209_read_raw(struct sen0209_instance *instance, int16_t *value)
{
	int err;

	/* Validate input parameters */
	if (instance == NULL || value == NULL) {
		return -EINVAL;
	}

	/* Check if ADC device is initialized */
	if (instance->adc_dev == NULL) {
		return -ENODEV;
	}

	/* Perform ADC read */
	err = adc_read(instance->adc_dev, &instance->sequence);
	if (err) {
		printk("SEN0209: ADC read failed with error %d\n", err);
		return err;
	}

	/* Copy result from sample buffer */
	*value = instance->sample_buffer;
	return 0;
}

/**
 * @brief Check if vibration is detected on sensor instance
 * 
 * Reads the sensor and compares the value against the threshold.
 * Vibration is detected when ADC value drops below threshold
 * (piezo generates voltage when bent).
 * 
 * @param instance Pointer to sensor instance
 * @param detected Pointer to store detection result
 * @return 0 on success, negative errno code on failure
 */
int sen0209_check_vibration(struct sen0209_instance *instance, bool *detected)
{
	int err;
	int16_t value;

	/* Validate input parameters */
	if (instance == NULL || detected == NULL) {
		return -EINVAL;
	}

	/* Read raw ADC value */
	err = sen0209_read_raw(instance, &value);
	if (err) {
		return err;
	}

	/* Compare against threshold - vibration detected when value < threshold */
	*detected = (value < instance->threshold);

	return 0;
}

/**
 * @brief Get complete sensor data from instance
 * 
 * Reads the sensor and populates a data structure with both
 * the raw ADC value and the vibration detection status.
 * 
 * @param instance Pointer to sensor instance
 * @param data Pointer to structure to store sensor data
 * @return 0 on success, negative errno code on failure
 *         -EINVAL: Invalid instance or data pointer
 *         Other: Error from sen0209_read_raw()
 */
int sen0209_get_data(struct sen0209_instance *instance, struct sen0209_data *data)
{
	int err;

	/* Validate input parameters */
	if (instance == NULL || data == NULL) {
		return -EINVAL;
	}

	/* Read raw ADC value */
	err = sen0209_read_raw(instance, &data->raw_value);
	if (err) {
		return err;
	}

	/* Determine vibration status based on threshold */
	data->vibration_detected = (data->raw_value < instance->threshold);

	return 0;
}

/**
 * @brief Set vibration detection threshold for sensor instance
 * 
 * Updates the threshold value used for vibration detection.
 * The sensor ADC value decreases when vibration occurs, so
 * values below this threshold indicate vibration.
 * 
 * Typical range: 400-600 depending on sensitivity requirements
 * - Lower threshold = less sensitive (harder impacts needed)
 * - Higher threshold = more sensitive (detects lighter touches)
 * 
 * @param instance Pointer to sensor instance
 * @param threshold New threshold value (0-4095 for 12-bit ADC)
 */
void sen0209_set_threshold(struct sen0209_instance *instance, uint16_t threshold)
{
	if (instance != NULL) {
		instance->threshold = threshold;
		printk("SEN0209 (Ch%d): Threshold updated to %d\n", 
		       instance->adc_channel, threshold);
	}
}

/**
 * @brief Get current vibration detection threshold from instance
 * 
 * Returns the currently configured threshold value used for
 * vibration detection.
 * 
 * @param instance Pointer to sensor instance
 * @return Current threshold value, or 0 if instance is NULL
 */
uint16_t sen0209_get_threshold(struct sen0209_instance *instance)
{
	if (instance != NULL) {
		return instance->threshold;
	}
	return 0;
}
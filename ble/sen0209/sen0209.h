/**
 * @file sen0209.h
 * @brief Driver header file for SEN0209 Piezo Vibration Sensor
 * 
 * This file contains the API declarations for the DFRobot SEN0209 
 * Flexible Piezo Film Vibration Sensor driver with multi-instance support.
 * 
 * The sensor detects vibration, flexibility, impact and touch through
 * a piezoelectric film that generates voltage when bent or deflected.
 */

#ifndef SEN0209_H
#define SEN0209_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief SEN0209 sensor instance structure
 * 
 * Each sensor instance maintains its own configuration and state.
 * This allows multiple sensors to be used simultaneously.
 */
struct sen0209_instance {
	const struct device *adc_dev;       /**< ADC device pointer */
	struct adc_channel_cfg channel_cfg; /**< ADC channel configuration */
	struct adc_sequence sequence;       /**< ADC sequence configuration */
	int16_t sample_buffer;              /**< Sample buffer for this instance */
	uint16_t threshold;                 /**< Vibration threshold for this instance */
	uint8_t adc_channel;                /**< ADC channel number */
};

/**
 * @brief SEN0209 sensor configuration structure
 * 
 * This structure holds the configuration parameters for initializing
 * the vibration sensor.
 */
struct sen0209_config {
	uint8_t adc_channel;         /**< ADC channel number (e.g., 0 for AIN0, 1 for AIN1) */
	uint16_t threshold;          /**< Vibration detection threshold value */
	uint16_t sample_interval_ms; /**< Sampling interval in milliseconds */
};

/**
 * @brief SEN0209 sensor data structure
 * 
 * This structure contains the sensor reading data including
 * raw ADC value and vibration detection status.
 */
struct sen0209_data {
	int16_t raw_value;           /**< Raw ADC value (0-4095 for 12-bit) */
	bool vibration_detected;     /**< True if vibration is detected */
};

/**
 * @brief Initialize a SEN0209 vibration sensor instance
 * 
 * This function initializes the ADC peripheral and configures the
 * sensor with the specified parameters. Each sensor instance can
 * be configured independently.
 * 
 * @param instance Pointer to sensor instance structure
 * @param config Pointer to sensor configuration structure
 * @return 0 on success, negative errno code on failure
 */
int sen0209_init(struct sen0209_instance *instance, const struct sen0209_config *config);

/**
 * @brief Read raw ADC value from sensor instance
 * 
 * Performs a single ADC conversion and returns the raw digital value.
 * 
 * @param instance Pointer to sensor instance
 * @param value Pointer to store the raw ADC reading
 * @return 0 on success, negative errno code on failure
 */
int sen0209_read_raw(struct sen0209_instance *instance, int16_t *value);

/**
 * @brief Check if vibration is detected on sensor instance
 * 
 * Reads the sensor and compares against threshold to determine
 * if vibration is present.
 * 
 * @param instance Pointer to sensor instance
 * @param detected Pointer to store detection result (true if vibration detected)
 * @return 0 on success, negative errno code on failure
 */
int sen0209_check_vibration(struct sen0209_instance *instance, bool *detected);

/**
 * @brief Get complete sensor data from instance
 * 
 * Reads the sensor and returns both raw value and vibration status
 * in a single structure.
 * 
 * @param instance Pointer to sensor instance
 * @param data Pointer to structure to store sensor data
 * @return 0 on success, negative errno code on failure
 */
int sen0209_get_data(struct sen0209_instance *instance, struct sen0209_data *data);

/**
 * @brief Set vibration detection threshold for sensor instance
 * 
 * Updates the threshold value used for vibration detection.
 * Values below this threshold are considered as vibration.
 * 
 * @param instance Pointer to sensor instance
 * @param threshold New threshold value (0-4095)
 */
void sen0209_set_threshold(struct sen0209_instance *instance, uint16_t threshold);

/**
 * @brief Get current vibration detection threshold from instance
 * 
 * @param instance Pointer to sensor instance
 * @return Current threshold value
 */
uint16_t sen0209_get_threshold(struct sen0209_instance *instance);

#endif /* SEN0209_H */
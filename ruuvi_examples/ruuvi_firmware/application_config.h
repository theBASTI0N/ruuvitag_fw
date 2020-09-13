#ifndef APPLICATION_CONFIG_H
#define APPLICATION_CONFIG_H

#include "bme280.h"
#include "lis2dh12.h"
// Milliseconds before new button press is accepted. Applies both to rising and falling edge
#define DEBOUNCE_THRESHOLD 100u
// Milliseconds before switch is checked again
#define APPLICATION_SWITCH_INTERVAL 10000u
// Milliseconds until new batteryreading is taken on radio interrupt.
// Use cached value otherse.
#define APPLICATION_BATTERY_INTERVAL 10000u
// Milliseconds to hold down the button before reset
#define BUTTON_RESET_TIME 3000u
// Milliseconds after NFC field detection to reset
#define NFC_RESET_DELAY   10000u

// 1, 2, 4, 8, 16.
// Oversampling increases current consumption, but lowers noise.
// IIR lowers noise, but slows step response.
#define BME280_HUMIDITY_OVERSAMPLING    BME280_OVERSAMPLING_1
#define BME280_TEMPERATURE_OVERSAMPLING BME280_OVERSAMPLING_1
#define BME280_PRESSURE_OVERSAMPLING    BME280_OVERSAMPLING_1
#define BME280_IIR                      BME280_IIR_16
#define BME280_DELAY                    BME280_STANDBY_1000_MS

#define LIS2DH12_SCALE              LIS2DH12_SCALE2G
#define LIS2DH12_RESOLUTION         LIS2DH12_RES10BIT
#define LIS2DH12_SAMPLERATE_RAWv2   LIS2DH12_RATE_10
#define LIS2DH12_SAMPLERATE_RAWv1   LIS2DH12_RATE_1

// mg, scaled to bits by driver
#define LIS2DH12_ACTIVITY_THRESHOLD 64

#endif
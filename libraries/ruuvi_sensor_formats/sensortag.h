#ifndef SENSORTAG_H
#define SENSORTAG_H


#include <stdbool.h>
#include <stdint.h>
#include "bme280.h"
#include "lis2dh12.h"

/*
0:   uint8_t     format;          // (0x03 = realtime sensor readings base64)
1:   uint8_t     humidity;        // one lsb is 0.5%
2-3: uint16_t    temperature;     // Signed 8.8 fixed-point notation.
4-5: uint16_t    pressure;        // (-50kPa)
6-7:   int16_t   acceleration_x;  // mg
8-9:   int16_t   acceleration_y;  // mg
10-11: int16_t   acceleration_z;  // mg
12-13: int16_t   vbat;            // mv
*/
#define SENSOR_TAG_DATA_FORMAT          0x03				  /**< raw binary, includes acceleration */
#define SENSORTAG_ENCODED_DATA_LENGTH   14            /**< 14 bytes  */

#define RAW_FORMAT_2                    0x05          /**< Proposal, please see https://f.ruuvi.com/t/proposed-next-high-precision-data-format/692 */
#define SW_DOOR_CLOSED                  0x15          /**< Variation of RAWv2, 15 is door closed */
#define SW_DOOR_OPEN                    0x16          /**< Variation of RAWv2, 16 is door open */ 
#define RAW_2_ENCODED_DATA_LENGTH       24

#define WEATHER_STATION_URL_FORMAT      0x02				  /**< Base64 */
#define WEATHER_STATION_URL_ID_FORMAT   0x04				  /**< Base64, with ID byte */

#define EDDYSTONE_URL_MAX_LENGTH 17
#define URL_PAYLOAD_LENGTH 9

#define URL_BASE_MAX_LENGTH (EDDYSTONE_URL_MAX_LENGTH - URL_PAYLOAD_LENGTH)

// Invalid values for data
#define TEMPERATURE_INVALID       -0x8000
#define HUMIDITY_INVALID          0xFFFF
#define PRESSURE_INVALID          0xFFFF
#define ACCELERATION_INVALID      -0x8000
#define RAW2_TEMPERATURE_INVALID  TEMPERATURE_INVALID
#define RAW2_HUMIDITY_INVALID     HUMIDITY_INVALID
#define RAW2_PRESSURE_INVALID     PRESSURE_INVALID
#define RAW2_ACCELERATION_INVALID ACCELERATION_INVALID
#define RAW1_TEMPERATURE_INVALID  0
#define RAW1_HUMIDITY_INVALID     0
#define RAW1_PRESSURE_INVALID     0
#define RAW1_ACCELERATION_INVALID 0

// Sensor values
typedef struct 
{
uint8_t     format;              // 0x00 ... 0x09 for official Ruuvi applications
uint32_t    humidity;            // one lsb is 1/1024%
int32_t     temperature;         // 1/100 C
uint32_t    pressure;            // Pascals (pa) / 256
int16_t     accX;                // Milli-g (mg)
int16_t     accY;
int16_t     accZ;
uint16_t    vbat;                // mv
}ruuvi_sensor_t;

/**
 *  Parses data into Ruuvi data format scale
 *  @param *data pointer to ruuvi_sensor_t object
 *  @param raw_t raw temperature as given by BME280, I.E 2-complement int32_t in celcius * 100, -2134 = 21.34
 *  @param raw_p raw pressure as given by BME280, uint32_t, multiplied by 256
 *  @param acceleration along 3 axes in milliG, X Y Z. 
 */
void parseSensorData(ruuvi_sensor_t* data, int32_t raw_t, uint32_t raw_p, uint32_t raw_h, uint16_t vbat, int32_t acc[3]);

/**
 *  Parses sensor values into RAWv1
 *  @param char* data_buffer character array with length of 14 bytes
 */
void encodeToRawFormat3(uint8_t* data_buffer, const ruuvi_sensor_t* const data);

/**
 *  Parses sensor values into RAWv2
 *  @param data_buffer uint8_t array with length of 24 bytes
 *  @param environmental  Environmental data as data comes from BME280, i.e. uint32_t pressure, int32_t temperature, uint32_t humidity
 *  @param acceleration 3 x int16_t having acceleration along X-Y-Z axes in MG. Low pass and last sample are allowed DSP operations
 *  @param acceleration_events counter of acceleration events. Events are configured by application, "value exceeds 1.1 G" recommended.
 *  @param vbatt Voltage of battery in millivolts
 */
void encodeToRawFormat5(uint8_t* data_buffer,  const ruuvi_sensor_t* const data, uint16_t acceleration_events, int8_t tx_pwr);


/**
 *  Slight addition to the above
 *  @param data_buffer uint8_t array with length of 24 bytes
 *  @param sw, if true door is open
 */
void encodeToSWRawFormat5(uint8_t* data_buffer,  const ruuvi_sensor_t* const data, uint16_t acceleration_events, int8_t tx_pwr, bool sw);


/**
 *  Encodes sensor data into given char* url. The base url must have the base of url written by caller.
 *  For example, url = {'r' 'u' 'u' '.' 'v' 'i' '/' '#' '0' '0' '0' '0' '0' '0' '0' '0' '0'}
 *  The URL may have a length of 18 bytes, 8 of which is consumed by payload. 
 *  
 *  @param url pointer to character array with max length of 18
 *  @param base_length length of base url. The payload will be written starting at index at base length
 *  @param data pointer to ruuvi_sensor_t which contains current sensor data to be encoded
 */
void encodeToUrlDataFromat(char* url, uint8_t base_length, ruuvi_sensor_t* data);

#endif

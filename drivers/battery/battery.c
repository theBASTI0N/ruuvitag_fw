/* Copyright (c) 2016 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include "nrf_drv_saadc.h"
#include "battery.h"
#include "boards.h" //For reverse voltage protection define
#define NRF_LOG_MODULE_NAME "ADC"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS  600  //!< Reference voltage (in milli volts) used by ADC while doing conversion.
#define ADC_RES_10BIT                  1024 //!< Maximum digital value for 10-bit ADC conversion.
#define ADC_PRE_SCALING_COMPENSATION   6    //!< The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.
#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE) \
    ((((ADC_VALUE) *ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_10BIT) * ADC_PRE_SCALING_COMPENSATION)

static nrf_saadc_value_t adc_buf;           //!< Buffer used for storing ADC values.
static uint8_t battery_is_init = 0;

/**@brief Function handling events from 'nrf_drv_saadc.c'.
 *
 * @param[in] p_evt SAADC event.
 */
static void saadc_event_handler(nrf_drv_saadc_evt_t const * p_evt)
{

    if (p_evt->type == NRF_DRV_SAADC_EVT_DONE)
    {
      NRF_LOG_DEBUG("ADC done \r\n");
    }
}


void battery_voltage_init(void)
{
    ret_code_t err_code = nrf_drv_saadc_init(NULL, saadc_event_handler);

    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);
    err_code = nrf_drv_saadc_channel_init(0, &config);
    APP_ERROR_CHECK(err_code);

    battery_is_init = 1;
}


uint16_t getBattery(void)
{
    ret_code_t err_code;
    if(!battery_is_init)
    {
      battery_voltage_init();
    }

    if (!nrf_drv_saadc_is_busy())
    {
        err_code = nrf_drv_saadc_sample_convert(NRF_SAADC_INPUT_VDD, &adc_buf);
        APP_ERROR_CHECK(err_code);

    }
    return ADC_RESULT_IN_MILLI_VOLTS(adc_buf) + REVERSE_PROT_VOLT_DROP_MILLIVOLTS;
}

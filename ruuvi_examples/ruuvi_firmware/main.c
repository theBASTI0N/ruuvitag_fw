/** RuuviTag firmware  */
// Version 2.5.3 August 29, 2019. Send valid data from LIS2DH12 / BME280 even if only one is present. 
// Version 2.5.2 August 15, 2019. Add invalid values to encoded data when real data is not available
// Version 2.5.1 July 15, 2019. Start up in RAWv2-fast mode, use apple guidelines for advertising rates. 
// Version 2.4.1 January 07, 2019 modes now  RAWv1(RED), RAWv2_FAST(GREEN) and RAWv2_SLOW(GREEN) preserved in flash. (URL removed) 
//               long button hold or NFC triggers reset.
//               short button or NFC  triggers become_connectable i.e. fast_advertising  BUT
//               default is always non-connectable, non-scannable (see bluetooth_application_config.h) 
//               accelerometer (lis2dh12) reewritten as pre STM
//               Only get battery voltage from ADC after radio has been quiet
// Version 2.2.3 August 01, 2018; 
//  Rewrite initalization continue even if there are failures and announce failure status by led

/* Copyright (c) 2015 Nordic Semiconductor. All Rights Reserved. 
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in 
 * NORDIC SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 * Licensees are granted free, non-transferable use of the information. 
 NO WARRANTY of ANY KIND is provided. This heading must NOT be removed from the file. */

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

// Nordic SDK
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_radio_notification.h"
#include "nordic_common.h"
#include "softdevice_handler.h"
#include "app_scheduler.h"
#include "app_timer_appsh.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"

#define NRF_LOG_MODULE_NAME "MAIN"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

// BSP  Board Support Package : 
#include "bsp.h"

// Drivers
#include "flash.h"
#include "lis2dh12.h"
#include "lis2dh12_acceleration_handler.h"
#include "bme280.h"
#include "battery.h"
#include "bluetooth_core.h"
#include "eddystone.h"
#include "pin_interrupt.h"
#include "nfc.h"
#include "nfc_t2t_lib.h"
#include "rtc.h"
#include "application_config.h"

// Libraries
#include "base64.h"
#include "sensortag.h"

// Init
#include "init.h"

// Configuration
#include "bluetooth_config.h"      // including  REVision, intervals, APP_TX_POWER

// Constants
#define DEAD_BEEF               0xDEADBEEF    //!< Value used as error code on stack dump, can be used to identify stack location on stack unwind.

// ID for main loop timer.
APP_TIMER_DEF(main_timer_id);                 // Creates timer id for our program.
APP_TIMER_DEF(reset_timer_id);                 // Creates timer id for our program.

static uint16_t init_status = 0;   // combined status of all initalizations.  Zero when all are complete if no errors occured.
static uint8_t NFC_message[100];   // NFC message buffer has 4 records, up to 128 bytes each minus some overhead for NFC NDEF data keeping. 
static size_t NFC_message_length = sizeof(NFC_message);
#define LOG_FAILED_INIT             0x0002
#define ACCELEROMETER_FAILED_INIT   0x0004
#define TEMP_HUM_PRESS_FAILED_INIT  0x0008
#define NFC_FAILED_INIT             0x0010
#define BLE_FAILED_INIT             0x0020
#define TIMER_FAILED_INIT           0x0040
#define RTC_FAILED_INIT             0x0080
#define PIN_ENA_FAILED_INIT         0x0200
#define ACCEL_INT_FAILED_INIT       0x0400
#define ACC_INT_FAILED_INIT         0x0800
#define SW_INT_FAILED_INIT          0x1600
#define BATTERY_MIN_V                 2000  //!< Millivolts, this value should be high enough to not trigger error on a tag on a freezer
#define BATTERY_FAILED_INIT         0x1000
#define BUTTON_FAILED_INIT          0x2000
#define BME_FAILED_INIT             0x4000

// File and record for app mode
#define FDS_FILE_ID 1
#define FDS_RECORD_ID 1

// define unconfusing macros for LEDs
#define RED_LED_ON    nrf_gpio_pin_clear(LED_RED)
#define RED_LED_OFF   nrf_gpio_pin_set(LED_RED)
#define GREEN_LED_ON  nrf_gpio_pin_clear(LED_GREEN)
#define GREEN_LED_OFF nrf_gpio_pin_set(LED_GREEN)

static uint8_t data_buffer[RAWv2_DATA_LENGTH] = { 0 };
static bool bme280_available = false;          // Flag for sensors available
static bool lis2dh12_available = false;        // Flag for sensors available
static bool fast_advertising = true;           // Connectable mode
static uint64_t fast_advertising_start = 0;    // Timestamp of when tag became connectable
static uint64_t debounce = 0;                  // Flag for avoiding double presses
static uint64_t sw_debounce = 0;               // Flag for avoiding accidental read of reed switch
static uint16_t acceleration_events = 0;       // Number of times accelerometer has triggered
static volatile uint16_t vbat = 0;             // Update in interrupt after radio activity.
static uint64_t last_battery_measurement = 0;  // Timestamp of VBat update.
static volatile bool pressed = false;          // Debounce flag
static volatile bool open = false;             // True if door is open

// Possible types of switch
#define NO 0
#define NC 1

// Must be UINT32_T as flash storage operated in 4-byte chunks
// Will get loaded from flash, this is default.
static uint32_t switch_type __attribute__ ((aligned (4))) = NO;

// Prototype declaration
static void main_timer_handler(void * p_context);

/**
 * Tag enters connectable mode. Main loop timer will close the connectable mode after 20 seconds.
 *
 * Parameters are unused.
 */
static void become_connectable(void* data, uint16_t length)
{
  fast_advertising_start = millis();
  fast_advertising = true;
  bluetooth_configure_advertising_interval(ADVERTISING_INTERVAL_STARTUP);
  bluetooth_configure_advertisement_type(STARTUP_ADVERTISEMENT_TYPE);
  bluetooth_apply_configuration();
}

/**
 * Stores current mode to flash, given in parameters.
 *
 * Data is address of the switch_type.
 * length is length of the address, not data.
 *
 */
static void store_type(void* data, uint16_t length)
{
  // Point the record directly to word-aligned tag mode rather than data pointer passed as context.
  ret_code_t err_code = flash_record_set(FDS_FILE_ID, FDS_RECORD_ID, sizeof(switch_type), &switch_type);
  if(err_code)
  {
   NRF_LOG_WARNING("Error in flash write %X\r\n", err_code);
  }
  else
  {
    size_t flash_space_remaining;
    flash_free_size_get(&flash_space_remaining);
    NRF_LOG_INFO("Stored mode in flash, Largest continuous space remaining %d bytes\r\n", flash_space_remaining);
    if(4000 > flash_space_remaining)
    {
      NRF_LOG_INFO("Flash space is almost used, running gc\r\n")
      flash_gc_run();
      flash_free_size_get(&flash_space_remaining);
      NRF_LOG_INFO("Continuous space remaining after gc %d bytes\r\n", flash_space_remaining);
    }
  }
}

/**
 * Reboots tag. Enters bootloader as button is pressed on boot
 */
static void reboot(void* p_context)
{
  // Reboot if we've not registered a button press, OR
  // if we have registered a button press and the button is still pressed (debounce)
  if(!pressed || (pressed && !(nrf_gpio_pin_read(BUTTON_1))))
  {
    NRF_LOG_WARNING("Rebooting\r\n")
    NVIC_SystemReset();
  }
  pressed = false;
}

/**@brief Function for handling button events.
 * Schedulers call to handler.
 * Use scheduler, do not use peripherals in interrupt context (SPI/Flash writes won't complete.)
 *
 * @param message. A struct containing data about the event. 
 *                 message.payload[0] has pin number (4)
 *                 message.payload[1] has pin state, 1 if high, 0 if low.
 */
ret_code_t button_press_handler(const ruuvi_standard_message_t message)
{
  // Debounce
  if(false == message.payload[1] && ((millis() - debounce) > DEBOUNCE_THRESHOLD) && !pressed)
  {
    NRF_LOG_INFO("Button pressed\r\n");
    GREEN_LED_ON;
    RED_LED_ON;
    // Start timer to reboot tag.
    app_timer_start(reset_timer_id, APP_TIMER_TICKS(BUTTON_RESET_TIME, RUUVITAG_APP_TIMER_PRESCALER), NULL);
    pressed = true;
  }
  if(true == message.payload[1] &&  pressed)
  {
     NRF_LOG_INFO("Button released\r\n");
     pressed = false;
     // Cancel reset
     app_timer_stop(reset_timer_id);

     // Clear leds
     GREEN_LED_OFF;
     RED_LED_OFF;

     // Update mode
     switch_type = !switch_type;

     //Enter connectable mode if allowed by configuration.
     if(APP_GATT_PROFILE_ENABLED)
     {
       app_sched_event_put (NULL, 0, become_connectable);
     }

     // Schedule store mode to flash
     app_sched_event_put (NULL, 0, store_type);
  }

  debounce = millis();
  return ENDPOINT_SUCCESS;
}

/**
  * @brief on a NC Reed switch signal will be high when near magnet,
  * on a NO the signal would be low when near magnet
  * The magnet is assumed to be installed on the door frame.
  **/
ret_code_t sw_handler(const ruuvi_standard_message_t message)
{
  NRF_LOG_INFO("SW-HANDLER %d\r\n", nrf_gpio_pin_read(SW));
 
  if( (millis() - sw_debounce > DEBOUNCE_THRESHOLD) && !open)
  {
    NRF_LOG_INFO("Door Opened\r\n");
    open = true;
  }
  else if( (millis() - sw_debounce > DEBOUNCE_THRESHOLD) && open)
  {
    NRF_LOG_INFO("Door Closed\r\n");
    open = false;
  }
  if(switch_type){
    RED_LED_ON;
  }
  else{GREEN_LED_ON;}

  sw_debounce = millis();
  return ENDPOINT_SUCCESS;
}

/**
 * Work around NFC data corruption bug by reinitializing NFC data after field has been lost.
 * Call this function outside of interrupt context.
 */
static void reinit_nfc(void* data, uint16_t length)
{
  init_nfc();
}

/**@brief Function for handling NFC events.
 * Schedulers call to handler.
 */
void app_nfc_callback(void* p_context, nfc_t2t_event_t event, const uint8_t* p_data, size_t data_length)
{
  NRF_LOG_INFO("NFC\r\n");
  switch (event)
  {
    case NFC_T2T_EVENT_FIELD_ON:
      // NFC activation causes tag to hang sometimes. Fix this by resetting tag after a delay on NFC read.
      NRF_LOG_INFO("NFC Field detected \r\n");
      app_timer_start(reset_timer_id, APP_TIMER_TICKS(NFC_RESET_DELAY, RUUVITAG_APP_TIMER_PRESCALER), NULL);
      break;

    case NFC_T2T_EVENT_FIELD_OFF:
      NRF_LOG_INFO("NFC Field lost \r\n");
      app_sched_event_put (NULL, 0, reinit_nfc);
      if(APP_GATT_PROFILE_ENABLED)
      {
        app_sched_event_put (NULL, 0, become_connectable);
      }
      break;

    case NFC_T2T_EVENT_DATA_READ:
      NRF_LOG_INFO("Data read\r\n");
      break;
    
    default:
      break;
  }
}


/**@brief Function for doing power management.
 */
static void power_manage(void)
{
  // Clear both leds before sleep if not indicating button press
  if (nrf_gpio_pin_read(BUTTON_1))
  {
  GREEN_LED_OFF;
  RED_LED_OFF;
  }
  uint32_t err_code = sd_app_evt_wait();
  APP_ERROR_CHECK(err_code);
}


static void updateAdvertisement(void)
{
  bluetooth_set_manufacturer_data(data_buffer, RAWv2_DATA_LENGTH);
}

void switch_check(void){
  // Conduct initial check of reed switch to set state of door. 
  if(switch_type){
    if(nrf_gpio_pin_read(SW)){
    NRF_LOG_INFO("NC High, Door Closed\r\n");
    open = false;
    }
    else{
      NRF_LOG_INFO("NC Low, Door Open\r\n");
      open = true;}
  }
  else{
    if(nrf_gpio_pin_read(SW)){
      NRF_LOG_INFO("NO High, Door Open\r\n");
      open = true;}
    else{
      NRF_LOG_INFO("NO Low, Door Closed\r\n");
      open = false;}
  }
  return;
}

static void main_sensor_task(void* p_data, uint16_t length)
{
  // Signal mode by led color.
  if(open){
    RED_LED_ON;
  }
  else{GREEN_LED_ON;}

  ruuvi_sensor_t data = { .accX = ACCELERATION_INVALID,
                          .accY = ACCELERATION_INVALID,
                          .accZ = ACCELERATION_INVALID,
                          .humidity = HUMIDITY_INVALID,
                          .pressure = PRESSURE_INVALID,
                          .temperature = TEMPERATURE_INVALID,
                          .vbat = vbat
                        };
  lis2dh12_sensor_buffer_t buffer;

  if (fast_advertising && ((millis() - fast_advertising_start) > ADVERTISING_STARTUP_PERIOD))
  {
    fast_advertising = false;
    bluetooth_configure_advertisement_type(APPLICATION_ADVERTISEMENT_TYPE);

    bluetooth_configure_advertising_interval(ADVERTISING_INTERVAL_RAW);
    bluetooth_apply_configuration();
  }

  if (bme280_available)
  {
    // Get raw environmental data.
    bme280_read_measurements();
    data.temperature = bme280_get_temperature();
    data.pressure    = bme280_get_pressure();
    data.humidity    = bme280_get_humidity();
  }
  // If only temperature sensor is present.
  else
  {
    int32_t temp;                                        // variable to hold temp reading
    (void)sd_temp_get(&temp);                            // get new temperature
    temp *= 25;                                          // SD returns temp * 4. Ruuvi format expects temp * 100. 4*25 = 100.
    data.temperature = temp;
  }

  if(lis2dh12_available)
  {
    // Get accelerometer data.
    lis2dh12_read_samples(&buffer, 1);
    data.accX = buffer.sensor.x;
    data.accY = buffer.sensor.y;
    data.accZ = buffer.sensor.z;
  }

  if(millis() - sw_debounce > APPLICATION_SWITCH_INTERVAL){
    switch_check();
  }

  encodeToSWRawFormat5(data_buffer, &data, acceleration_events, BLE_TX_POWER, open);

  updateAdvertisement();
  watchdog_feed();
  
}

/**@brief Timeout handler for the repeated timer
 */
static void main_timer_handler(void * p_context)
{
  app_sched_event_put (NULL, 0, main_sensor_task);
}


/**
 * @brief Handle interrupt from lis2dh12.
 * Never do long actions, such as sensor reads in interrupt context.
 * Using peripherals in interrupt is also risky,
 * as peripherals might require interrupts for their function.
 *
 *  @param message Ruuvi message, with source, destination, type and 8 byte payload. Ignore for now.
 **/
ret_code_t lis2dh12_int2_handler(const ruuvi_standard_message_t message)
{
  NRF_LOG_DEBUG("Accelerometer interrupt to pin 2\r\n");
  acceleration_events++;
  /*
  app_sched_event_put ((void*)(&message),
                       sizeof(message),
                       lis2dh12_scheduler_event_handler);
  */
  return NRF_SUCCESS;
}

/**
 * Task to run on radio activity
 * This function is in interrupt context, avoid long processing or using peripherals.
 *
 * parameter active: True if radio is going to be active after event, false if radio was turned off (after tx/rx)
 */
static void on_radio_evt(bool active)
{
  // If radio is turned off (was active) and enough time has passed since last measurement
  if(false == active && millis() - last_battery_measurement > APPLICATION_BATTERY_INTERVAL)
  {
    vbat = getBattery();
    last_battery_measurement = millis();
  }
}

/**  This is where it all starts ++++++++++++++++++++++++++++++++++++++++++ 
 main is entered as a result of one of SEVERAL events:
  - Normal startup from press of reset button.
  - Battery inserted.
  - After DFU (Device Firmware Upgrade) at manufacturing Quality Assurance or user DFU.
  - WatchDogTimer expiration and its interrupt handler didn't feed new value.
  - Some error occured and
  - Spontenous unknown reset.
 All subsystems are initalized and any failures are noted and available later in init_status
 Since some events occur after tag is deployed and no one can see the LEDs the system continues operating.

 After initalizition (including setting up interrupts)
    we loop here calling app_sched_execute and sd_app_evt_wait 
*/
int main(void)
{
   // LEDs first (they're easy and cannot fail)  drivers/init/init.c
  init_leds();
  RED_LED_ON;

  if( init_log() ) { init_status |=LOG_FAILED_INIT; }
  else { NRF_LOG_INFO("LOG initialized \r\n"); } // subsequent initializations assume log is working
  
  // Switch initialization cannot fail under any reasonable circumstance.
  init_sw();

  // start watchdog now in case program hangs up.
  // watchdog_default_handler logs error and resets the tag.
  init_watchdog(NULL);

  // Battery voltage initialization cannot fail under any reasonable circumstance.
  battery_voltage_init(); 
  vbat = getBattery();

  if( vbat < BATTERY_MIN_V ) { init_status |=BATTERY_FAILED_INIT; }
  else NRF_LOG_INFO("BATTERY initalized \r\n"); 

  if(init_lis2dh12() == NRF_SUCCESS )
  {
    lis2dh12_available = true;
    NRF_LOG_INFO("Accelerometer initialized \r\n");  
  }
  else { init_status |= ACC_INT_FAILED_INIT; }

  if(init_bme280() == NRF_SUCCESS )
  {
    bme280_available = true;
    NRF_LOG_INFO("BME initialized \r\n");  
  } 
  else { init_status |= TEMP_HUM_PRESS_FAILED_INIT; }


  // Init NFC ASAP in case we're waking from deep sleep via NFC (todo)
  // outputs ID:DEVICEID ,MAC:DEVICEADDR, SW:REVision
  set_nfc_callback(app_nfc_callback);
  if( init_nfc() ) { init_status |= NFC_FAILED_INIT; } 
  else { NRF_LOG_INFO("NFC init \r\n"); }

  pin_interrupt_init(); 

  if( pin_interrupt_enable(BSP_BUTTON_0, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIO_PIN_PULLUP, button_press_handler) ) 
  {
    init_status |= BUTTON_FAILED_INIT;
  }

  //Checks state of the switch
  switch_check();
  
  if (pin_interrupt_enable(SW, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIO_PIN_PULLUP, sw_handler) )
    {
      init_status |= SW_INT_FAILED_INIT;
    }

  // Initialize BLE Stack. Starts LFCLK required for timer operation.
  if( init_ble() ) { init_status |= BLE_FAILED_INIT; }
  bluetooth_configure_advertisement_type(STARTUP_ADVERTISEMENT_TYPE);
  bluetooth_tx_power_set(BLE_TX_POWER);
  bluetooth_configure_advertising_interval(ADVERTISING_INTERVAL_STARTUP);

  // Priorities 2 and 3 are after SD timing critical events. 
  // 6, 7 after SD non-critical events.
  // Triggers ADC, so use 3. 
  ble_radio_notification_init(3,
                              NRF_RADIO_NOTIFICATION_DISTANCE_800US,
                              on_radio_evt);

  // If GATT is enabled BLE init inits peer manager which uses flash.
  // BLE init should handle insufficient space gracefully (i.e. erase flash and proceed). 
  // Flash must be initialized after softdevice. 
  if(flash_init())
  {
    NRF_LOG_ERROR("Failed to init flash \r\n");
  }
  size_t flash_space_remaining = 0;
  flash_free_size_get(&flash_space_remaining);
  NRF_LOG_INFO("Largest continuous space remaining %d bytes\r\n", flash_space_remaining);
  if(4000 > flash_space_remaining)
  {
    NRF_LOG_INFO("Flash space is almost used, running gc\r\n")
    flash_gc_run();
    flash_free_size_get(&flash_space_remaining);
    NRF_LOG_INFO("Continuous space remaining after gc %d bytes\r\n", flash_space_remaining);
  }
  else if (flash_record_get(FDS_FILE_ID, FDS_RECORD_ID, sizeof(switch_type), &switch_type))
  {
   NRF_LOG_INFO("Did not find mode in flash, is this first boot? \r\n");
  }
  else 
  {
    NRF_LOG_INFO("Loaded mode %d from flash\r\n", switch_type);
  }

  if( init_rtc() ) { init_status |= RTC_FAILED_INIT; }
  else { NRF_LOG_INFO("RTC initialized \r\n"); }

  // Configure lis2dh12
  if (lis2dh12_available)    
  {
    lis2dh12_reset(); // Clear memory.
    
    // Enable Low-To-Hi rising edge trigger interrupt on nRF52 to detect acceleration events.
    if (pin_interrupt_enable(INT_ACC2_PIN, NRF_GPIOTE_POLARITY_LOTOHI, NRF_GPIO_PIN_NOPULL, lis2dh12_int2_handler) )
    {
      init_status |= ACC_INT_FAILED_INIT;
    }
    
    nrf_delay_ms(10); // Wait for LIS reboot.
    // Enable XYZ axes.
    lis2dh12_enable();
    lis2dh12_set_scale(LIS2DH12_SCALE);
    lis2dh12_set_sample_rate(LIS2DH12_SAMPLERATE_RAWv1);
    lis2dh12_set_resolution(LIS2DH12_RESOLUTION);

    lis2dh12_set_activity_interrupt_pin_2(LIS2DH12_ACTIVITY_THRESHOLD);
    NRF_LOG_INFO("Accelerometer configuration done \r\n");
  }
  if(bme280_available)
  {
    // oversampling must be set for each used sensor.
    bme280_set_oversampling_hum  (BME280_HUMIDITY_OVERSAMPLING);
    bme280_set_oversampling_temp (BME280_TEMPERATURE_OVERSAMPLING);
    bme280_set_oversampling_press(BME280_PRESSURE_OVERSAMPLING);
    bme280_set_iir(BME280_IIR);
    bme280_set_interval(BME280_DELAY);
    bme280_set_mode(BME280_MODE_NORMAL);
    NRF_LOG_INFO("BME280 configuration done \r\n");
  }
  
  // Initialize repeated timer for sensor read and single-shot timer for button reset
  if( init_timer(main_timer_id, APP_TIMER_MODE_REPEATED, MAIN_LOOP_INTERVAL_RAW, main_timer_handler) )
  {
    init_status |= TIMER_FAILED_INIT;
  }
  
  if( init_timer(reset_timer_id, APP_TIMER_MODE_SINGLE_SHOT, BUTTON_RESET_TIME, reboot) )
  {
    init_status |= TIMER_FAILED_INIT;
  }
  // Init starts timers, stop the reset
  app_timer_stop(reset_timer_id);

  // Log errors, add a note to NFC, blink RED to visually indicate the problem
  if (init_status)
  { 
    snprintf((char* )NFC_message, NFC_message_length, "Error: %X", init_status);
    NRF_LOG_WARNING (" -- Initialization error :  %X \r\n", init_status);
    for ( int16_t i=0; i<13; i++)
    { 
      RED_LED_ON;
      nrf_delay_ms(500u);
      RED_LED_OFF;
      nrf_delay_ms(500u); 
    }
  }
  
  // Turn green led on if model+ with no errors.
  // Power manage turns led off
  if (!init_status)
  {
    GREEN_LED_ON;
  }

  // Turn off red led, leave green on to signal model+ without errors
  RED_LED_OFF;

  // Wait for sensors to take first sample
  nrf_delay_ms(1000);
  // Get first sample from sensors, set fast advertising start counter
  fast_advertising_start = millis();
  app_sched_event_put (NULL, 0, main_sensor_task);
  app_sched_execute();

  // Start advertising 
  bluetooth_advertising_start(); 
  NRF_LOG_INFO("Advertising started\r\n");

  // Enter main loop. Executes tasks scheduled by timers and interrupts.
  for (;;)
  {
    app_sched_execute();
    // Sleep until next event.
    power_manage();
  }
}

// 01/07/19 2.4.1 add FDS_CRC_ENABLED; Default turn off GATT (see bluetooth_application_config.h ) 
//                include SDK overrides; WDT reload 6 minutes
// This is included in drivers/init/sdk_config.h
#ifndef SDK_APPLICATION_CONFIG_H
#define SDK_APPLICATION_CONFIG_H
#include "bluetooth_application_config.h"


// Define any application-specific sdk configuration overrides here
#define PWM_ENABLED     0
#define PWM0_ENABLED    0
#define APP_PWM_ENABLED 0
#define PPI_ENABLED     1
#define TIMER_ENABLED   1
#define TIMER0_ENABLED  1
#define TIMER1_ENABLED  1
#define TIMER2_ENABLED  1
#define TIMER3_ENABLED  1
#define TIMER4_ENABLED  0  // Required by NFC
#define NFC_HAL_ENABLED 1
#define FDS_CRC_ENABLED 1  // Driver checks if value is defined, comment this line out to disable CRC
#if FDS_CRC_ENABLED
  #define CRC16_ENABLED   FDS_CRC_ENABLED  
  #define CRC32_ENABLED   FDS_CRC_ENABLED  
#endif

#define NRF_LOG_ENABLED 1  // Disable log output by default to save space (unless needed for testing)

#if APP_GATT_PROFILE_ENABLED
  #define BLE_DIS_ENABLED 1  //Device information service
  #define BLE_NUS_ENABLED 1  //Nordic UART Service
  #define BLE_DFU_ENABLED 1  //DFU service
#else
  #define BLE_DIS_ENABLED 0  //Device information service
  #define BLE_NUS_ENABLED 0  //Nordic UART Service
  #define BLE_DFU_ENABLED 0  //DFU service
#endif
#define BLE_UUID_COUNT BLE_DIS_ENABLED + BLE_NUS_ENABLED + BLE_DFU_ENABLED

// While this application does not require 10 pages, 
// Eddystone might have previous, valid configuration.
// Conserve enough space for the eddystone data + this application
// to avoid out of space when validating filesystem at boot. 
#define FDS_OP_QUEUE_SIZE 10
#define FDS_CHUNK_QUEUE_SIZE 15
#define FDS_MAX_USERS 8
#define FDS_VIRTUAL_PAGES 10
#define FDS_VIRTUAL_PAGE_SIZE 1024


// WDT_CONFIG_RELOAD_VALUE - Reload value  <15-4294967295> (ms)
// Watchdog cannot be stopped even when entering bootloader, 
// so use 6 minutes to allow DFU process to complete.
#ifndef WDT_CONFIG_RELOAD_VALUE
#define WDT_CONFIG_RELOAD_VALUE 360000 
#endif

#endif

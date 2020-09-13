#!/bin/bash
nrfutil settings generate --family NRF52 --application Output/nrf52832_xxaa/Exe/ruuvitag_fw.hex --application-version 1 --bootloader-version 1 --bl-settings-version 1 settings.hex
mergehex -m s132_nrf52_3.1.0_softdevice.hex ruuvitag_b_bootloader.hex settings.hex -o sbc.hex
mergehex -m sbc.hex Output/nrf52832_xxaa/Exe/ruuvitag_fw.hex -o ruuvi_firmware_full.hex

#nrfutil pkg generate --debug-mode --application Output/nrf52832_xxaa/Exe/ruuvitag_fw.hex --hw-version 3 --sd-req 0x91 --key-file ~/git/ruuvitag_fw/keys/ruuvi_open_private.pem ruuvi_firmware_dfu.zip



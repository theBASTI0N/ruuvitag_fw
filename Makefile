# Main makefile of ruuvi/ruuvitag_fw

ifeq ($(OS),Windows_NT)
	TOP := %cd%
else
	TOP := `pwd`
	FILEID = `gdrive list --query "name contains 'weather_station-test.zip'"|sed '2!d' |sed 's/ weather.*//'`
endif

SDK_VERSION := 12.3.0_d7731ad
SDK_URL     := https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v12.x.x
SDK_FILE    := nRF5_SDK_$(SDK_VERSION).zip

SDK_DIR     := $(basename $(SDK_FILE))
SDK_HOME    := $(TOP)/$(SDK_DIR)

ifeq ($(OS),Windows_NT)
	DOWNLOAD_CMD ?= powershell curl -o
	UNZIP_CMD ?= powershell Expand-Archive -DestinationPath .
else
	DOWNLOAD_CMD ?= curl -o
	UNZIP_CMD ?= unzip -q
endif

export $(SDK_HOME)

.PHONY: all bootstrap fw bootloader

all: bootstrap fw bootloader

bootstrap: $(SDK_FILE) $(SDK_DIR) $(SDK_DIR)/external/micro-ecc/micro-ecc
	@echo SDK_HOME = ${SDK_HOME}

$(SDK_DIR):
	$(UNZIP_CMD) $(SDK_FILE)
	$(call patch_sdk_$(SDK_VERSION))

$(SDK_FILE):
	@echo downloading SDK zip...
	$(DOWNLOAD_CMD) $(SDK_FILE) $(SDK_URL)/$(SDK_FILE)

$(SDK_DIR)/external/micro-ecc/micro-ecc:
	git clone https://github.com/kmackay/micro-ecc.git $(SDK_DIR)/external/micro-ecc/micro-ecc
	$(MAKE) -C $(SDK_DIR)/external/micro-ecc/nrf52_armgcc/armgcc

fw:
	@echo build FW
	$(MAKE) -C ruuvi_examples/ruuvi_firmware/ruuvitag_b/s132/armgcc

bootloader:
	@echo build bootloader
	$(MAKE) -C bootloader/ruuvitag_b_debug/armgcc
	$(MAKE) -C bootloader/ruuvitag_b_production/armgcc

clean:
	@echo cleaning B build files…
	$(MAKE) -C ruuvi_examples/ruuvi_firmware/ruuvitag_b/s132/armgcc clean
	$(MAKE) -C bootloader/ruuvitag_b_debug/armgcc clean
	$(MAKE) -C bootloader/ruuvitag_b_production/armgcc clean
# Licenses 

## Nordic SDK
Nordic SDK provides interface to nRF52 chip and various libraries for software development. [Nordic SDK license](<licenses/nordic_sdk_license.md>) applies to nRF5_SDK folders, with the exception of `external` subfolder. It also applies to `bsp` folder, `ruuvi_examples` folder and `sdk_overrides` folder.

## ARM CMSIS
ARM CMSIS provides _a vendor-independent hardware abstraction layer for Cortex-M processors_. ARM CMSIS is embedded into Nordic SDK and is available under
ARM BSD-3 License, please see Nordic SDK for details.

## Softdevice s132 V3.1.0
Softdevice is a software radio stack which is used to implement BLE communication on top of nRF52832 integrated circuit. It is distributed as a binary only, embedded within nRF52832.
License is Nordic softdevice license agreement. Full text is available in Nordic SDK and [licenses folder](<licenses/nordic_softdevice_license.md>)

Softdevice is available at https://www.nordicsemi.com/eng/nordic/Products/nRF52832/S132-SD-v3/56261 .

## Bootloader
Bootloader is built on top of Nordic example sources and [Nordic SDK License](<licenses/nordic_sdk_license.md>) applies to it. Additionally the bootloader uses micro-ecc library, available at https://github.com/kmackay/micro-ecc . Micro-ECC is released under [BSD-2 license](<licenses/micro-ecc.md>), copyright 2014 Kenneth MacKay.

Bootloader licensing applies to `bootloader` folder. 
Bootloader is statically linked against Softdevice S132 3.1.0

## Ruuvi firmware
Ruuvi firmware is built by Ruuvi community and it is BSD-3 licensed or Nordic SDK licensed.
These licenses apply to folders `drivers`, `keys`, `libraries`, and `licenses`. 
Files which are under Nordic SDK license can be identified by a comment in the header. 
Copyleft or BSD-3 status can be identified from the list below, please see git commit history for contributors.

Copyright in alphabetical order of GitHub usernames, real name and email are listed for user who have listed it on their profile:

* [angst7](https://github.com/angst7): Matt Roche <angst7@gmail.com> (BSD-3)
* [DG12](https://github.com/DG12):   Dennis German <DGermang@Real-World-Systems.com> (BSD-3)
* dieweltistklein (account removed): (BSD-3)
* [frawau](https://github.com/frawau): François Wautier (BSD-3)
* [jessenic](https://github.com/jessenic): Jesse Leskinen (BSD-3)
* [jrosberg](https://github.com/jrosberg): Janne Rosberg (BSD-3)
* [laurijamsa](https://github.com/laurijamsa): Lauri Jämsä <lauri@ruuvi.com> (BSD-3)
* [mdxs](https://github.com/mdxs): (BSD-3)
* [ojousima](https://github.com/ojousima): Otso Jousimaa <otso@ruuvi.com> (BSD-3)
* [Scrin](https://github.com/Scrin): (BSD-3)
* [tpikonen](https://github.com/tpikonen) (BSD-3)

Ruuvi firmware is statically linked against bootloader and softdevice. 

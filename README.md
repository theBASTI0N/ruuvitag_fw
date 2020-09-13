# RuuviTag nRF52 Bootloader & Example firmware projects
[![RuuviTag](https://ruuvi.com/assets/images/ruuvitag.jpg)](https://ruuvi.com)


Aim is to build on top of ruuvi fw and add a reed switch for use a a door monitor. Door information would be added to tx power. So if tx power % 4 == 0 door is closed, if == 1 door is opened. Plan is to use GPIO P0.31, the reed switch will be attached between  pin and ground. 

The reed switch is small enough to be placed inside fo the case.

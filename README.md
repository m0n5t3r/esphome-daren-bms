# esphome-jk-bms

![GitHub actions](https://github.com/m0n5t3r/esphome-dr-bms/actions/workflows/ci.yaml/badge.svg)

ESPHome component to monitor a Daren Battery Management System (DR-BMS) via UART-TTL

## Supported devices
Work in progress, but this intends to cover the devices showing up as DR-JC03 and similar. Mine displays DR48100JC-03-V2, but the info gotten via RS485 says NIE-JC48100.

## Requirements

* [ESPHome 2024.6.0 or higher](https://github.com/esphome/esphome/releases).
* Generic ESP32 or ESP8266 board

## References

* https://github.com/syssi/esphome-jk-bms - heavy inspiration for the esphome bits
* https://github.com/cpttinkering/daren-485 - protocol details
* https://github.com/butterwecksolutions/DR-JC03-RS485-Switcher - first thing that worked, used for double checking my commands

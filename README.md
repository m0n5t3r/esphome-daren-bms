# esphome-daren-bms

ESPHome component to monitor a Daren Battery Management System (DR-BMS) via UART-TTL

## Supported devices

This component implements the protocol documented by  Daren/DR-BMS devices showing up as DR-JC03 and similar. My device displays DR48100JC-03-V2, but the info gotten via RS485 says NIE-JC48100 and manufacturer NIE (the battery brand is Nielftor).


## Requirements

* [ESPHome 2026.2.2 or higher](https://github.com/esphome/esphome/releases).
* Generic ESP32 board
* UART connection with RS485 adapter

## Installation

Add as an external component in your ESPHome configuration

## Configuration

### Basic Configuration

```yaml
uart:
  rx_pin: GPIO16
  tx_pin: GPIO17
  baud_rate: 9600

# Daren BMS configuration
daren_bms:
  device_address: 0x01
  update_interval: 30s

binary_sensor:
  - platform: daren_bms
    charging:
      name: "Battery Charging"
    discharging:
      name: "Battery Discharging"
    balancing:
      name: "Battery Balanced"

sensor:
  - platform: daren_bms
    total_voltage:
      name: "Voltage"
    current:
      name: "Current"
    average_cell_voltage:
      name: "Average Cell Voltage"
    cell_count:
      name: "Cell Count"
    capacity_remaining:
      name: "Capacity Remaining"
    charging_cycles:
      name: "Charging Cycles"
    state_of_charge:
      name: "State of Charge"
    state_of_health:
      name: "State of Health"

    # Temperature sensors
    mos_temperature:
      name: "MOS Temperature"
    env_temperature:
      name: "Environment Temperature"
    pack_temperature:
      name: "Pack Temperature"
```

### Full Configuration
See [here](examples/daren_bms_full.yaml)

## Protocol Details

This component uses the Daren/DR-BMS RS485 protocol documented in the saren-485 repo linked below; a stand-alone implementation in Python is `scripts/bms_query.py`.

## References

* https://github.com/syssi/esphome-jk-bms - heavy inspiration for the esphome bits
* https://github.com/cpttinkering/daren-485 - protocol details
* https://github.com/butterwecksolutions/DR-JC03-RS485-Switcher - first thing that worked, used for double checking my commands

# esphome-daren-bms

<!-- ![GitHub actions](https://github.com/m0n5t3r/esphome-dr-bms/actions/workflows/ci.yaml/badge.svg) -->

ESPHome component to monitor a Daren Battery Management System (DR-BMS) via UART-TTL

## Supported devices

**WIP, not functional yet**
Done:

* Python protocol implementation that works and also makes pyright happy.
* C++ protocol implementation that compiles passes tests on my machine (TM)

TODO:
* The esphome sensors, configuration, testing on actual hardware (I have the electrical bits figured out - ESP32 hooked via a rs485 transceiver acting as a sernet server, the Python script works fine with it)

This component will support Daren/DR-BMS devices showing up as DR-JC03 and similar. My device displays DR48100JC-03-V2, but the info gotten via RS485 says NIE-JC48100 and manufacturer NIE (the battery brand is Nielftor).

An example protocol implementation in Python is `scripts/bms_query.py`

## Requirements

* [ESPHome 2024.6.0 or higher](https://github.com/esphome/esphome/releases).
* Generic ESP32 or ESP8266 board
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

daren_bms:
```

### Full Configuration

(sensor list subject to change)

```yaml
uart:
  rx_pin: GPIO16
  tx_pin: GPIO17
  baud_rate: 9600

daren_bms:
  bms_id: 0x01
  update_interval: 60s
  max_cell_count: 16
  soc:
    name: "Battery State of Charge"
  voltage:
    name: "Battery Voltage"
  current:
    name: "Battery Current"
  temperature:
    name: "Battery Temperature"
  capacity_remaining:
    name: "Remaining Capacity"
  capacity_full:
    name: "Full Capacity"
  capacity_design:
    name: "Design Capacity"
  cycle_count:
    name: "Cycle Count"
  soh:
    name: "State of Health"
  internal_resistance:
    name: "Internal Resistance"
  fet_status:
    name: "FET Status"
  balance_state:
    name: "Balance State"
```

## Available Sensors

### Basic Sensors
- `soc` - State of Charge (%)
- `voltage` - Battery voltage (V)
- `current` - Battery current (A)
- `temperature` - Pack temperature (°C)
- `capacity_remaining` - Remaining capacity (Ah)
- `capacity_full` - Full capacity (Ah)
- `capacity_design` - Design capacity (Ah)
- `cycle_count` - Cycle count
- `soh` - State of Health (%)
- `internal_resistance` - Internal resistance (mΩ)
- `fet_status` - FET status
- `balance_state` - Balance state

### Cell Voltages
Individual cell voltage sensors are automatically created based on `max_cell_count` setting:
- `cell_1_voltage`, `cell_2_voltage`, ..., `cell_16_voltage`

## Protocol Details

This component uses the Daren/DR-BMS RS485 protocol specification:
- Baud rate: 9600
- Data format: 8N1
- Protocol: Custom Daren command/response protocol

## References

* https://github.com/syssi/esphome-jk-bms - heavy inspiration for the esphome bits
* https://github.com/cpttinkering/daren-485 - protocol details
* https://github.com/butterwecksolutions/DR-JC03-RS485-Switcher - first thing that worked, used for double checking my commands

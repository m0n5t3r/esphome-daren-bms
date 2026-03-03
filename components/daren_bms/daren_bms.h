#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "protocol.h"
#include <cstdint>
#include <cstring>
#include <string>

namespace esphome {
namespace daren_bms {

class DarenBMS : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void dump_config() override;
  void loop() override;

  void set_device_address(uint8_t device_address) { this->device_address_ = device_address; }
  void set_update_interval(uint interval) { this->update_interval_ = interval; }

  // binary sensors
  void set_balancing_binary_sensor(binary_sensor::BinarySensor *balancing_binary_sensor) {
    this->balancing_binary_sensor_ = balancing_binary_sensor;
  }
  void set_charging_binary_sensor(binary_sensor::BinarySensor *charging_binary_sensor) {
    this->charging_binary_sensor_ = charging_binary_sensor;
  }
  void set_discharging_binary_sensor(binary_sensor::BinarySensor *discharging_binary_sensor) {
    this->discharging_binary_sensor_ = discharging_binary_sensor;
  }
  void set_online_status_binary_sensor(binary_sensor::BinarySensor *online_status_binary_sensor) {
    this->online_status_binary_sensor_ = online_status_binary_sensor;
  }

  // sensors
  void set_delta_cell_voltage_sensor(sensor::Sensor *delta_cell_voltage_sensor) {
    this->delta_cell_voltage_sensor_ = delta_cell_voltage_sensor;
  }
  void set_average_cell_voltage_sensor(sensor::Sensor *average_cell_voltage_sensor) {
    this->average_cell_voltage_sensor_ = average_cell_voltage_sensor;
  }
  void set_cell_count_sensor(sensor::Sensor *cell_count_sensor) { this->cell_count_sensor_ = cell_count_sensor; }
  void set_mos_temperature_sensor(sensor::Sensor *mos_temperature_sensor) {
    this->mos_temperature_sensor_ = mos_temperature_sensor;
  }
  void set_env_temperature_sensor(sensor::Sensor *env_temperature_sensor) {
    this->env_temperature_sensor_ = env_temperature_sensor;
  }
  void set_pack_temperature_sensor(sensor::Sensor *pack_temperature_sensor) {
    this->pack_temperature_sensor_ = pack_temperature_sensor;
  }
  void set_temperature_sensor_1_sensor(sensor::Sensor *temperature_sensor_1_sensor) {
    this->temperature_sensor_1_sensor_ = temperature_sensor_1_sensor;
  }
  void set_temperature_sensor_2_sensor(sensor::Sensor *temperature_sensor_2_sensor) {
    this->temperature_sensor_2_sensor_ = temperature_sensor_2_sensor;
  }
  void set_temperature_sensor_3_sensor(sensor::Sensor *temperature_sensor_3_sensor) {
    this->temperature_sensor_3_sensor_ = temperature_sensor_3_sensor;
  }
  void set_temperature_sensor_4_sensor(sensor::Sensor *temperature_sensor_4_sensor) {
    this->temperature_sensor_4_sensor_ = temperature_sensor_4_sensor;
  }
  void set_total_voltage_sensor(sensor::Sensor *total_voltage_sensor) {
    this->total_voltage_sensor_ = total_voltage_sensor;
  }
  void set_current_sensor(sensor::Sensor *current_sensor) { this->current_sensor_ = current_sensor; }
  void set_capacity_remaining_sensor(sensor::Sensor *capacity_remaining_sensor) {
    this->capacity_remaining_sensor_ = capacity_remaining_sensor;
  }
  void set_temperature_sensors_sensor(sensor::Sensor *temperature_sensors_sensor) {
    this->temperature_sensors_sensor_ = temperature_sensors_sensor;
  }
  void set_charging_cycles_sensor(sensor::Sensor *charging_cycles_sensor) {
    this->charging_cycles_sensor_ = charging_cycles_sensor;
  }
  void set_state_of_charge_sensor(sensor::Sensor *state_of_charge_sensor) {
    this->state_of_charge_sensor_ = state_of_charge_sensor;
  }
  void set_state_of_health_sensor(sensor::Sensor *state_of_health_sensor) {
    this->state_of_health_sensor_ = state_of_health_sensor;
  }

  // cell sensors
  void set_cell_voltage_sensor(uint8_t cell, sensor::Sensor *cell_voltage_sensor) {
    if (cell < 16) {
      this->cells_[cell].cell_voltage_sensor_ = cell_voltage_sensor;
    }
  }

 protected:
  uint8_t device_address_{0x01};  // Default BMS ID
  uint update_interval_{30000};   // Default update interval in ms

  // read response
  void read_response_();
  void on_response_received_(StaticVector<uint8_t, BUF_MAX_SIZE> payload);

  // Setup phase queries
  void query_manufacturer_info_();
  void query_manufacturer_params_();
  void query_capacity_params_();
  void query_system_params_();

  // Update phase queries
  void query_device_info_();

  // State tracking
  enum SetupState {
    SETUP_BEGIN,
    SETUP_MFG_INFO,
    SETUP_MFG_PARAMS,
    SETUP_CAP_PARAMS,
    SETUP_SYSTEM_PARAMS,
    SETUP_COMPLETE
  } setup_state_{SETUP_BEGIN};

  enum CommState { IDLE, COMMAND_SENT } comm_state_{IDLE};

  // Data storage
  MfgInfo manufacturer_info_;
  MfgParams manufacturer_params_;
  CapParams capacity_params_;
  SystemParams system_params_;

  // sensors
  binary_sensor::BinarySensor *balancing_binary_sensor_;
  binary_sensor::BinarySensor *charging_binary_sensor_;
  binary_sensor::BinarySensor *discharging_binary_sensor_;
  binary_sensor::BinarySensor *online_status_binary_sensor_;

  sensor::Sensor *delta_cell_voltage_sensor_;
  sensor::Sensor *average_cell_voltage_sensor_;
  sensor::Sensor *cell_count_sensor_;
  sensor::Sensor *mos_temperature_sensor_;
  sensor::Sensor *env_temperature_sensor_;
  sensor::Sensor *pack_temperature_sensor_;
  sensor::Sensor *temperature_sensor_1_sensor_;
  sensor::Sensor *temperature_sensor_2_sensor_;
  sensor::Sensor *temperature_sensor_3_sensor_;
  sensor::Sensor *temperature_sensor_4_sensor_;
  sensor::Sensor *total_voltage_sensor_;
  sensor::Sensor *current_sensor_;
  sensor::Sensor *capacity_remaining_sensor_;
  sensor::Sensor *temperature_sensors_sensor_;
  sensor::Sensor *charging_cycles_sensor_;
  sensor::Sensor *state_of_charge_sensor_;
  sensor::Sensor *state_of_health_sensor_;

  struct Cell {
    sensor::Sensor *cell_voltage_sensor_{nullptr};
  } cells_[16];

  void update_device_info_(DeviceInfo state);

  void update_sensor_(binary_sensor::BinarySensor *binary_sensor, const bool &state);
  void update_sensor_(sensor::Sensor *sensor, float state);
};
}  // namespace daren_bms
}  // namespace esphome

#include "daren_bms.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "protocol.h"
#include <cstdint>
#include <cstdio>

namespace esphome {
namespace daren_bms {

static const char *const TAG = "daren_bms";

void DarenBMS::setup() { ESP_LOGD(TAG, "Setting up Daren BMS..."); }

void DarenBMS::dump_config() {
  ESP_LOGCONFIG(TAG, "Daren BMS:");
  ESP_LOGCONFIG(TAG, "  BMS ID: 0x%02X", this->device_address_);
  /*
  LOG_BINARY_SENSOR("", "Balancing", this->balancing_binary_sensor_);
  LOG_BINARY_SENSOR("", "Charging", this->charging_binary_sensor_);
  LOG_BINARY_SENSOR("", "Discharging", this->discharging_binary_sensor_);
  LOG_BINARY_SENSOR("", "Online Status", this->online_status_binary_sensor_);
  LOG_SENSOR("", "Delta Cell Voltage", this->delta_cell_voltage_sensor_);
  LOG_SENSOR("", "Average Cell Voltage", this->average_cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Count", this->cell_count_sensor_);
  LOG_SENSOR("", "Cell Voltage 1", this->cells_[0].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 2", this->cells_[1].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 3", this->cells_[2].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 4", this->cells_[3].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 5", this->cells_[4].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 6", this->cells_[5].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 7", this->cells_[6].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 8", this->cells_[7].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 9", this->cells_[8].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 10", this->cells_[9].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 11", this->cells_[10].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 12", this->cells_[11].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 13", this->cells_[12].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 14", this->cells_[13].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 15", this->cells_[14].cell_voltage_sensor_);
  LOG_SENSOR("", "Cell Voltage 16", this->cells_[15].cell_voltage_sensor_);
  LOG_SENSOR("", "MOS Temperature", this->mos_temperature_sensor_);
  LOG_SENSOR("", "ENV Temperature", this->mos_temperature_sensor_);
  LOG_SENSOR("", "PACK Temperature", this->mos_temperature_sensor_);
  LOG_SENSOR("", "Temperature Sensor 1", this->temperature_sensor_1_sensor_);
  LOG_SENSOR("", "Temperature Sensor 2", this->temperature_sensor_2_sensor_);
  LOG_SENSOR("", "Temperature Sensor 3", this->temperature_sensor_3_sensor_);
  LOG_SENSOR("", "Temperature Sensor 4", this->temperature_sensor_4_sensor_);
  LOG_SENSOR("", "Total Voltage", this->total_voltage_sensor_);
  LOG_SENSOR("", "Current", this->current_sensor_);
  LOG_SENSOR("", "Capacity Remaining", this->capacity_remaining_sensor_);
  LOG_SENSOR("", "Temperature Sensors", this->temperature_sensors_sensor_);
  LOG_SENSOR("", "Charging Cycles", this->charging_cycles_sensor_);
  */
}

void DarenBMS::loop() {
  const uint32_t now = millis();

  this->read_response_();
  if (this->comm_state_ == COMMAND_SENT) {
    return;
  }

  // Handle setup phase
  if (this->setup_state_ != SETUP_COMPLETE) {
    // Check for responses to setup queries
    switch (this->setup_state_) {
      case SETUP_BEGIN:
        //  ESP_LOGD(TAG, "Querying manufacturer info");
        //  this->query_manufacturer_info_();
        // case SETUP_MFG_INFO:
        //  ESP_LOGD(TAG, "Querying manufacturer params");
        this->query_manufacturer_params_();
        this->comm_state_ = COMMAND_SENT;
        break;
      case SETUP_MFG_PARAMS:
        ESP_LOGD(TAG, "Querying capacity params");
        this->query_capacity_params_();
        this->comm_state_ = COMMAND_SENT;
        break;
      // case SETUP_CAP_PARAMS:
      //   ESP_LOGD(TAG, "Querying system params");
      //   this->query_system_params_();
      //   break;
      default:
        break;
    }
  }

  // Normal operation - query device info periodically
  static uint32_t last_update = 0;
  if (now - last_update > this->update_interval_) {
    last_update = now;
    this->query_device_info_();
    this->comm_state_ = COMMAND_SENT;
  }
}

void DarenBMS::read_response_() {
  std::string buf;
  if (this->available() >= 1) {
    uint8_t data;
    while (this->read_byte(&data)) {
      buf += data;
      if (data == '\r') {
        break;
      }
    }
  }
  this->comm_state_ = IDLE;
  StaticVector<uint8_t, BUF_MAX_SIZE> payload;
  if (validate_response(this->device_address_, buf, payload)) {
    this->on_response_received_(payload);
  }
}

void DarenBMS::on_response_received_(StaticVector<uint8_t, BUF_MAX_SIZE> &payload) {
  // Handle setup phase
  switch (this->setup_state_) {
    case SETUP_MFG_INFO:
      ESP_LOGD(TAG, "Got mfg info");
      this->manufacturer_info_ = unpack_mfg_info(payload);
      this->setup_state_ = SETUP_MFG_PARAMS;
      break;
    case SETUP_MFG_PARAMS:
      ESP_LOGD(TAG, "Got mfg params");
      this->manufacturer_params_ = unpack_mfg_params(payload);
      this->setup_state_ = SETUP_CAP_PARAMS;
      break;
    case SETUP_CAP_PARAMS:
      ESP_LOGD(TAG, "Got cap params");
      this->capacity_params_ = unpack_cap_params(payload);
      // this->setup_state_ = SETUP_SYSTEM_PARAMS;
      this->setup_state_ = SETUP_COMPLETE;
      break;
    case SETUP_SYSTEM_PARAMS:
      ESP_LOGD(TAG, "Got system params");
      this->system_params_ = unpack_system_params(payload);
      this->setup_state_ = SETUP_COMPLETE;
      break;
    default:
      ESP_LOGD(TAG, "Got device info");
      DeviceInfo state = unpack_device_info(payload);
      this->update_device_info_(state);
      break;
  }
}

void DarenBMS::query_manufacturer_info_() {
  std::string cmd = build_command(this->device_address_, CMD_MFG_INFO);
  this->write_str(cmd.c_str());
  ESP_LOGD(TAG, "Querying manufacturer info: %s", cmd.c_str());
}

void DarenBMS::query_manufacturer_params_() {
  std::string cmd = build_command(this->device_address_, CMD_PARAMS, CMD_PARAMS_MOD_MFG);
  this->write_str(cmd.c_str());
  ESP_LOGD(TAG, "Querying manufacturer params: %s", cmd.c_str());
}

void DarenBMS::query_capacity_params_() {
  std::string cmd = build_command(this->device_address_, CMD_PARAMS, CMD_PARAMS_MOD_CAP);
  this->write_str(cmd.c_str());
  ESP_LOGD(TAG, "Querying capacity params: %s", cmd.c_str());
}

void DarenBMS::query_system_params_() {
  std::string cmd = build_command(this->device_address_, CMD_SYSTEM_PARAMS);
  this->write_str(cmd.c_str());
  ESP_LOGD(TAG, "Querying system params: %s", cmd.c_str());
}

void DarenBMS::query_device_info_() {
  std::string cmd = build_command(this->device_address_, CMD_DEVICE_INFO);
  this->write_str(cmd.c_str());
  ESP_LOGD(TAG, "Querying device info: %s", cmd.c_str());
}

void DarenBMS::update_sensor_(binary_sensor::BinarySensor *binary_sensor, const bool &state) {
  if (binary_sensor == nullptr)
    return;

  binary_sensor->publish_state(state);
}

void DarenBMS::update_sensor_(sensor::Sensor *sensor, float state) {
  if (sensor == nullptr)
    return;

  sensor->publish_state(state);
}

void DarenBMS::update_device_info_(DeviceInfo state) {
  this->update_sensor_(this->balancing_binary_sensor_, state.balance_state_l == 0 && state.balance_state_h == 0);
  this->update_sensor_(charging_binary_sensor_, state.current > 0);
  this->update_sensor_(discharging_binary_sensor_, state.current < 0);
  this->update_sensor_(this->online_status_binary_sensor_,
                       true);  // TODO: implement online status

  float voltages[16] = {
      state.cell_1_voltage,  state.cell_2_voltage,  state.cell_3_voltage,  state.cell_4_voltage,
      state.cell_5_voltage,  state.cell_6_voltage,  state.cell_7_voltage,  state.cell_8_voltage,
      state.cell_9_voltage,  state.cell_10_voltage, state.cell_11_voltage, state.cell_12_voltage,
      state.cell_13_voltage, state.cell_14_voltage, state.cell_15_voltage, state.cell_16_voltage,
  };

  float vmin = voltages[0];
  float vmax = voltages[0];
  float vsum = 0;

  for (size_t i = 0; i < state.cell_count; i++) {
    float v = voltages[i];
    if (vmin > v) {
      vmin = v;
    }
    if (vmax < v) {
      vmax = v;
    }
    vsum += v;
    this->update_sensor_(this->cells_[i].cell_voltage_sensor_, v);
  }

  float vdelta = vmax - vmin;
  float vavg = vsum / state.cell_count;
  this->update_sensor_(this->delta_cell_voltage_sensor_, vdelta);
  this->update_sensor_(this->average_cell_voltage_sensor_, vavg);
  this->update_sensor_(this->cell_count_sensor_, state.cell_count);
  this->update_sensor_(this->mos_temperature_sensor_, state.mos_temp);
  this->update_sensor_(this->env_temperature_sensor_, state.env_temp);
  this->update_sensor_(this->pack_temperature_sensor_, state.pack_temp);
  this->update_sensor_(this->temperature_sensor_1_sensor_, state.temp_1);
  this->update_sensor_(this->temperature_sensor_2_sensor_, state.temp_2);
  this->update_sensor_(this->temperature_sensor_3_sensor_, state.temp_3);
  this->update_sensor_(this->temperature_sensor_4_sensor_, state.temp_4);
  this->update_sensor_(this->total_voltage_sensor_, state.voltage);
  this->update_sensor_(this->current_sensor_, state.current);
  this->update_sensor_(this->capacity_remaining_sensor_, state.charge_now_ah);
  this->update_sensor_(this->temperature_sensors_sensor_, state.tot_temps);
  this->update_sensor_(this->charging_cycles_sensor_, state.cycle_count);
  this->update_sensor_(this->state_of_charge_sensor_, state.soc);
  this->update_sensor_(this->state_of_health_sensor_, state.soh);
}

}  // namespace daren_bms
}  // namespace esphome

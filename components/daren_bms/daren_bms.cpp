#include "daren_bms.h"
#include "protocol.h"
#include <cstdint>
#include <cstdio>

#ifndef TESTING
#include "esphome/core/log.h"
#endif // !TESTING

namespace esphome {
  namespace daren_bms {

    static const char *const TAG = "daren_bms";

#ifndef TESTING
    void DarenBMS::setup() {
      ESP_LOGD(TAG, "Setting up Daren BMS...");

      // Start with first setup query
      this->query_manufacturer_info_();
    }

    void DarenBMS::dump_config() {
      ESP_LOGCONFIG(TAG, "Daren BMS:");
      ESP_LOGCONFIG(TAG, "  BMS ID: 0x%02X", this->bms_id_);
      ESP_LOGCONFIG(TAG, "  Manufacturer Info: %s", this->manufacturer_info_.c_str());
      ESP_LOGCONFIG(TAG, "  Manufacturer Params: %s", this->manufacturer_params_.c_str());
      ESP_LOGCONFIG(TAG, "  Capacity Params: %s", this->capacity_params_.c_str());
      ESP_LOGCONFIG(TAG, "  System Params: %s", this->system_params_.c_str());
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
    }

    void DarenBMS::loop() {
      const uint32_t now = millis();

      // Handle setup phase
      if (this->setup_state_ != SETUP_COMPLETE) {
        // Check for responses to setup queries
        StaticVector<uint8_t,BUF_MAX_SIZE> payload;
        if (validate_response(this->bms_id_, this->read_response_(), payload)) {
          switch (this->setup_state_) {
            case SETUP_MFG_INFO:
              this->manufacturer_info_ = std::string(payload.begin(), payload.end());
              ESP_LOGD(TAG, "Querying manufacturer params");
              this->query_manufacturer_params_();
              break;
            case SETUP_MFG_PARAMS:
              this->manufacturer_params_ = std::string(payload.begin(), payload.end());
              ESP_LOGD(TAG, "Querying capacity params");
              this->query_capacity_params_();
              break;
            case SETUP_CAP_PARAMS:
              this->capacity_params_ = std::string(payload.begin(), payload.end());
              ESP_LOGD(TAG, "Querying system params");
              this->query_system_params_();
              break;
            case SETUP_SYSTEM_PARAMS:
              this->system_params_ = std::string(payload.begin(), payload.end());
              this->setup_state_ = SETUP_COMPLETE;
              ESP_LOGI(TAG, "Setup complete");
              break;
            default:
              break;
          }
        }
        return;
      }

      // Normal operation - query device info periodically
      static uint32_t last_update = 0;
      if (now - last_update > 60000) {  // 60 seconds
        last_update = now;
        this->query_device_info_();
      }
    }
#endif // !TESTING

    std::string DarenBMS::read_response_() {
      std::string buf;
#ifndef TESTING
      if (this->available() >= 1) {
        uint8_t data;
        while (this->read_byte(&data)) {
          buf += data;
          if (data == '\r') {
            break;
          }
        }
      }
#endif // !TESTING
      return buf;
    }

    void DarenBMS::query_manufacturer_info_() {
      std::string cmd = build_command(this->bms_id_, CMD_MFG_INFO);
#ifndef TESTING
      this->write_str(cmd.c_str());
      ESP_LOGD(TAG, "Querying manufacturer info: %s", cmd.c_str());
#endif // !TESTING
    }

    void DarenBMS::query_manufacturer_params_() {
      std::string cmd = build_command(this->bms_id_, CMD_PARAMS, CMD_PARAMS_MOD_MFG);
#ifndef TESTING
      this->write_str(cmd.c_str());
      ESP_LOGD(TAG, "Querying manufacturer params: %s", cmd.c_str());
#endif // !TESTING
    }

    void DarenBMS::query_capacity_params_() {
      std::string cmd = build_command(this->bms_id_, CMD_PARAMS, CMD_PARAMS_MOD_CAP);
#ifndef TESTING
      this->write_str(cmd.c_str());
      ESP_LOGD(TAG, "Querying capacity params: %s", cmd.c_str());
#endif // !TESTING
    }

    void DarenBMS::query_system_params_() {
      std::string cmd = build_command(this->bms_id_, CMD_SYSTEM_PARAMS);
#ifndef TESTING
      this->write_str(cmd.c_str());
      ESP_LOGD(TAG, "Querying system params: %s", cmd.c_str());
#endif // !TESTING
    }

    void DarenBMS::query_device_info_() {
      std::string cmd = build_command(this->bms_id_, CMD_DEVICE_INFO);
#ifndef TESTING
      this->write_str(cmd.c_str());
      ESP_LOGD(TAG, "Querying device info: %s", cmd.c_str());
#endif // !TESTING
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

  }  // namespace daren_bms
}  // namespace esphome

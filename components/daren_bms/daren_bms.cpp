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

  }  // namespace daren_bms
}  // namespace esphome

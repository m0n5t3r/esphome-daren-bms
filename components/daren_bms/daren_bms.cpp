#include "daren_bms.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <cstdint>
#include <vector>

namespace esphome {
namespace daren_bms {

static const char *const TAG = "daren_bms";

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
    std::vector<uint8_t> payload;
    if (this->parse_response_(this->read_response(), payload)) {
      switch (this->setup_state_) {
        case SETUP_MFG_INFO:
          this->manufacturer_info_ = std::string(payload.begin(), payload.end());
          this->query_manufacturer_params_();
          break;
        case SETUP_MFG_PARAMS:
          this->manufacturer_params_ = std::string(payload.begin(), payload.end());
          this->query_capacity_params_();
          break;
        case SETUP_CAP_PARAMS:
          this->capacity_params_ = std::string(payload.begin(), payload.end());
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

void DarenBMS::append_hex_(std::string &str, uint8_t value) {
  char hex[3];
  format_hex_to(hex, value);
  str += hex[0];
  str += hex[1];
}

void DarenBMS::append_hex_(std::string &str, uint16_t value) {
  char hex[5];
  format_hex_to(hex, value);
  str += hex[0];
  str += hex[1];
  str += hex[2];
  str += hex[3];
}

std::string DarenBMS::build_command_(uint8_t cid2, const uint8_t *data, size_t len) {
  std::string cmd;

  cmd += '~';
  this->append_hex_(cmd, VER_);
  this->append_hex_(cmd, this->bms_id_);
  this->append_hex_(cmd, CID1_);
  this->append_hex_(cmd, cid2);

  // Length and checksum
  if (len > 0) {
    uint16_t length_cs = this->length_checksum_(len * 2);  // len in hex chars
    this->append_hex_(cmd, length_cs);

    // Data
    for (size_t i = 0; i < len; i++) {
      this->append_hex_(cmd, data[i]);
    }
  } else {
    cmd += "0000";
  }

  // Calculate checksum of everything except SOF
  uint16_t cksum = this->checksum_(cmd.substr(1));
  this->append_hex_(cmd, cksum);

  // End with CR
  cmd += '\r';

  return cmd;
}

uint16_t DarenBMS::length_checksum_(uint16_t value) {
  value &= 0x0FFF;
  uint16_t n1 = value & 0xF;
  uint16_t n2 = (value >> 4) & 0xF;
  uint16_t n3 = (value >> 8) & 0xF;
  uint16_t cksum = ((n1 + n2 + n3) & 0xF) ^ 0xF;
  cksum += 1;
  return value + (cksum << 12);
}

uint16_t DarenBMS::checksum_(const std::string &s) {
  std::string upper_s = s;
  // Convert to uppercase
  for (char &c : upper_s) {
    c = toupper(c);
  }
  uint16_t cksum = 0;
  for (char c : upper_s) {
    cksum += static_cast<uint8_t>(c);
  }
  cksum ^= 0xFFFF;
  return cksum + 1;
}

std::vector<uint8_t> DarenBMS::read_response() {
  std::vector<uint8_t> response = {};
  if (this->available() >= 1) {
    uint8_t data;
    while (this->read_byte(&data)) {
      response.push_back(data);
      if (data == '\r') {
        break;
      }
    }
  }
  return response;
}

bool DarenBMS::parse_response_(const std::vector<uint8_t> &response, std::vector<uint8_t> &payload) {
  if (response.size() < 6) {
    return false;  // Minimum response size
  }

  // Check start of frame
  if (response[0] != '~') {
    return false;
  }

  // Check address
  if (response[1] != this->bms_id_) {
    return false;
  }

  // Check response CID1 (should be 0xA5 for responses)
  if (response[2] != 0xA5) {
    return false;
  }

  // Extract payload length
  uint8_t length = response[4];
  if (response.size() < 6 + length) {
    return false;
  }

  // Verify checksum
  uint8_t checksum = 0;
  for (size_t i = 1; i < response.size() - 1; i++) {
    checksum ^= response[i];
  }
  if (checksum != response.back()) {
    return false;
  }

  // Extract payload (skip header and checksum)
  payload.assign(response.begin() + 5, response.begin() + 5 + length - 1);
  return true;
}

void DarenBMS::query_manufacturer_info_() {
  std::string cmd = this->build_command_(0x47);
  this->write_str(cmd);
  ESP_LOGD(TAG, "Querying manufacturer info: %s", cmd.c_str());
}

void DarenBMS::query_manufacturer_params_() {
  uint8_t data[] = {0x60, 0x0A, 0x01, 0x01, 0x03, 0xFF, 0x00};
  std::string cmd = this->build_command_(0xB0, data, sizeof(data));
  this->write_str(cmd);
  ESP_LOGD(TAG, "Querying manufacturer params: %s", cmd.c_str());
}

void DarenBMS::query_capacity_params_() {
  uint8_t data[] = {0x60, 0x0A, 0x01, 0x01, 0x04, 0xFF, 0x00};
  std::string cmd = this->build_command_(0xB0, data, sizeof(data));
  this->write_str(cmd);
  ESP_LOGD(TAG, "Querying capacity params: %s", cmd.c_str());
}

void DarenBMS::query_system_params_() {
  std::string cmd = this->build_command_(0x42);
  this->write_str(cmd);
  ESP_LOGD(TAG, "Querying system params: %s", cmd.c_str());
}

void DarenBMS::query_device_info_() {
  std::string cmd = this->build_command_(0x42);
  this->write_str(cmd);
  ESP_LOGD(TAG, "Querying device info: %s", cmd.c_str());
}

}  // namespace daren_bms
}  // namespace esphome

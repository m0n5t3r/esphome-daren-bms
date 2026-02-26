#include "daren_bms.h"
#include <cstdint>
#include <cstdio>
#include <vector>
#include <iostream>

#ifndef TESTING
#include "esphome/core/log.h"
#endif // !TESTING

namespace esphome {
namespace daren_bms {

static const char *const TAG = "daren_bms";

const std::unordered_map<uint8_t, std::string> DarenBMS::CID2_CODES_ = {
  {0x00, "CID2 response ok."},
  {0x01, "VER error."},
  {0x02, "CHKSUM error."},
  {0x03, "LCHKSUM error."},
  {0x04, "CID2 invalid."},
  {0x05, "Command format error."},
  {0x06, "INFO data invalid."},
  {0x90, "ADR error."},
  {0x91, "Battery communication error."},
};

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
    std::vector<uint8_t> payload;
    if (this->parse_response_(this->read_response_(), payload)) {
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

void DarenBMS::append_hex_(std::string &str, uint8_t value) {
  char hex[3];
  sprintf(hex, "%02X", value);
  str += hex[0];
  str += hex[1];
}

void DarenBMS::append_hex_(std::string &str, uint16_t value) {
  char hex[5];
  sprintf(hex, "%02X", value);
  str += hex[0];
  str += hex[1];
  str += hex[2];
  str += hex[3];
}

std::vector<uint8_t> DarenBMS::parse_hex(std::string str) {
  std::vector<uint8_t> parsed = {};
  uint buf;
  size_t start = 0;
  size_t end = str.length();
  if(str.length() % 2 != 0) {
    return parsed;
  }
  if(str[0] == '~') {
    parsed.push_back('~');
    start = 1;
  }
  if(str[end - 1] == '\r') {
    end -= 1;
  }
  for (size_t i = start; i < end; i += 2) {
      sscanf(str.substr(i, 2).c_str(), "%02X", &buf);
      parsed.push_back(buf);
  }
  if(end != str.length()) {
    parsed.push_back('\r');
  }
  return parsed;
}

#ifdef TESTING
std::string DarenBMS::gen_cmd(const uint8_t cmd_id, const uint8_t module_id) {
  switch(cmd_id) {
    case CMD_PARAMS_:
      return this->build_command_(cmd_id, {this->bms_id_, 0x01, module_id, 0xff, 0x00});
    case CMD_MFG_INFO_:
    case CMD_PROTOCOL_VERSION_:
      return this->build_command_(cmd_id);
    default:
      return this->build_command_(cmd_id, {this->bms_id_});
  }
}

  void DarenBMS::setup() {
    std::vector<uint8_t> payload;
    bool is_okay = this->parse_response_(this->read_response_(), payload);

    if(is_okay) {
      std::cout << "Payload is " << this->format_hex(payload) << std::endl;
    } else {
      std::cout << "Payload parsing failed" << std::endl;
    }
  }
#endif // TESTING

std::string DarenBMS::build_command_(uint8_t cid2, const std::vector<uint8_t> info) {
  std::string cmd;
  size_t len = info.size();

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
      this->append_hex_(cmd, info[i]);
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
  uint16_t cksum = 0;
  for (char c : s) {
    cksum += static_cast<uint8_t>(c);
  }
  cksum ^= 0xFFFF;
  return cksum + 1;
}

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
#else
  buf = "~22014A00E0C60026F914DF100D0D0D0B0D0B0D0C0D0D0D0C0D0C0D0C0D0B0D0B0D0B0D0B0D0C0D0C0D0C0D0C009E007E008804007F0082007A007D000000000064012834281C0001000000000000000010230000000000000000000000000000000000000000004000D4A8\r";
#endif // !TESTING
  return buf;
}

bool DarenBMS::parse_response_(const std::string &buf, std::vector<uint8_t> &payload) {
  std::vector<uint8_t>response = this->parse_hex(buf);

  if (response.size() < 6) {
    return false;  // Minimum response size
  }

  // Check start of frame
  if (response[0] != '~') {
    return false;
  }

  // Check version
  if (response[1] != this->VER_) {
    return false;
  }

  // Check address
  if (response[2] != this->bms_id_) {
    return false;
  }

  // Check response CID1 (should be 0xA5 for responses)
  if (response[3] != this->CID1_) {
    return false;
  }

  // check CID2
  if (response[4] != 0x00) {
#ifndef TESTING
  ESP_LOGD(TAG, "CID2 error: %s", CID2_CODES_[response[4]].c_str());
#endif // !TESTING
    return false;
  }

  // Extract payload length
  uint16_t len_id = response[5] << 8 | response[6];
  uint16_t length = (len_id & 0x0fff) / 2;
  if (response.size() < 6 + length) {
#ifndef TESTING
  ESP_LOGD(TAG, "Length error: found %d, should be > %d", response.size(), length);
#endif // !TESTING
    return false;
  }

  uint16_t buf_checksum = response[response.size() - 3] << 8 | response[response.size() - 2];
  uint16_t calc_checksum = this->checksum_(buf.substr(1, buf.length() - 6));
  if(buf_checksum != calc_checksum) {
#ifndef TESTING
    ESP_LOGD(TAG, "Checksum error: received %04x, calculated %04x", buf_checksum, calc_checksum);
#endif // !TESTING
    return false;
  }

  // Extract payload (skip header and checksum)
  payload.assign(response.begin() + 7, response.begin() + 7 + length);
  return true;
}

void DarenBMS::query_manufacturer_info_() {
  std::string cmd = this->build_command_(0x47);
#ifndef TESTING
  this->write_str(cmd.c_str());
  ESP_LOGD(TAG, "Querying manufacturer info: %s", cmd.c_str());
#endif // !TESTING
}

void DarenBMS::query_manufacturer_params_() {
  std::vector<uint8_t> data = {0x60, 0x0A, 0x01, 0x01, 0x03, 0xFF, 0x00};
  std::string cmd = this->build_command_(0xB0, data);
#ifndef TESTING
  this->write_str(cmd.c_str());
  ESP_LOGD(TAG, "Querying manufacturer params: %s", cmd.c_str());
#endif // !TESTING
}

void DarenBMS::query_capacity_params_() {
  std::vector<uint8_t> data = {0x60, 0x0A, 0x01, 0x01, 0x04, 0xFF, 0x00};
  std::string cmd = this->build_command_(0xB0, data);
#ifndef TESTING
  this->write_str(cmd.c_str());
  ESP_LOGD(TAG, "Querying capacity params: %s", cmd.c_str());
#endif // !TESTING
}

void DarenBMS::query_system_params_() {
  std::string cmd = this->build_command_(0x42);
#ifndef TESTING
  this->write_str(cmd.c_str());
  ESP_LOGD(TAG, "Querying system params: %s", cmd.c_str());
#endif // !TESTING
}

void DarenBMS::query_device_info_() {
  std::string cmd = this->build_command_(0x42);
#ifndef TESTING
  this->write_str(cmd.c_str());
  ESP_LOGD(TAG, "Querying device info: %s", cmd.c_str());
#endif // !TESTING
}

}  // namespace daren_bms
}  // namespace esphome

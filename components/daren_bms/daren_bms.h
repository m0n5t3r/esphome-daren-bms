#pragma once

#ifndef TESTING
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#endif // !TESTING
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>

namespace esphome {
namespace daren_bms {

#ifndef TESTING
class DarenBMS : public Component, public uart::UARTDevice {
#else // TESTING
class DarenBMS {
#endif // !TESTING
 public:
  static const uint8_t CMD_SYSTEM_PARAMS_ = 0x47;
  static const uint8_t CMD_PROTOCOL_VERSION_ = 0x4F;
  static const uint8_t CMD_MFG_INFO_ = 0x51;
  static const uint8_t CMD_DEVICE_INFO_ = 0x42;
  static const uint8_t CMD_PARAMS_ = 0xB0;
  static const uint8_t CMD_PARAMS_MOD_OCV_ = 0x01;
  static const uint8_t CMD_PARAMS_MOD_HWPROT_ = 0x02;
  static const uint8_t CMD_PARAMS_MOD_MFG_ = 0x03;
  static const uint8_t CMD_PARAMS_MOD_CAP_ = 0x04;
#ifndef TESTING
  void setup() override;
  void dump_config() override;
  void loop() override;
#else // TESTING
	DarenBMS() = default;
  void setup();
  char* format_hex(std::vector<uint8_t> data) {
    char* hexString = new char[data.size() * 2 + 1];

    for (size_t i = 0; i < data.size(); ++i) {
      // Convert each byte to its hexadecimal representation
      sprintf(hexString + (i * 2), "%02X", data[i]);
    }

    // Null-terminate the string
    hexString[data.size() * 2] = '\0';
    return hexString;
  }

#endif // !TESTING

  void set_bms_id(uint8_t bms_id) { this->bms_id_ = bms_id; }
  uint8_t get_bms_id() { return this->bms_id_; }

#ifdef TESTING
  std::string gen_cmd(const uint8_t cmd_id, const uint8_t module_id = 0);
#endif // TESTING

 protected:
  static const uint8_t VER_ = 0x22;
  static const uint8_t CID1_ = 0x4A;
  static const std::unordered_map<uint8_t, std::string> CID2_CODES_;
  uint8_t bms_id_ = 0x01;  // Default BMS ID

  // Command building
  std::string build_command_(uint8_t cid2, const std::vector<uint8_t> info = {});

  // Response parsing
  bool parse_response_(const std::string &response, std::vector<uint8_t> &payload);

  // read response
  std::string read_response_();

  // Setup phase queries
  void query_manufacturer_info_();
  void query_manufacturer_params_();
  void query_capacity_params_();
  void query_system_params_();

  // Update phase queries
  void query_device_info_();

  // Helper methods
  uint16_t length_checksum_(uint16_t value);
  uint16_t checksum_(const std::string &s);
  uint16_t checksum_(const std::vector<uint8_t> &vec);
  void append_hex_(std::string &str, uint8_t value);
  void append_hex_(std::string &str, uint16_t value);
  std::vector<uint8_t> parse_hex(std::string str);

  // State tracking
  enum SetupState {
    SETUP_MFG_INFO,
    SETUP_MFG_PARAMS,
    SETUP_CAP_PARAMS,
    SETUP_SYSTEM_PARAMS,
    SETUP_COMPLETE
  } setup_state_{SETUP_MFG_INFO};

  // Data storage
  std::string manufacturer_info_;
  std::string manufacturer_params_;
  std::string capacity_params_;
  std::string system_params_;
};

}  // namespace daren_bms
}  // namespace esphome

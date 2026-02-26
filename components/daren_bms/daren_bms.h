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
#ifndef TESTING
  void setup() override;
  void dump_config() override;
  void loop() override;
#else // TESTING
	DarenBMS() = default;
#endif // !TESTING

  void set_bms_id(uint8_t bms_id) { this->bms_id_ = bms_id; }
  uint8_t get_bms_id() { return this->bms_id_; }

 protected:
  uint8_t bms_id_ = 0x01;  // Default BMS ID

  // read response
  std::string read_response_();

  // Setup phase queries
  void query_manufacturer_info_();
  void query_manufacturer_params_();
  void query_capacity_params_();
  void query_system_params_();

  // Update phase queries
  void query_device_info_();

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

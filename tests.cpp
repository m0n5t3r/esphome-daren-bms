#define TESTING 1

#include "components/daren_bms/daren_bms.h"
#include "components/daren_bms/protocol.h"
#include <cstdint>
#include <string>
#include <iostream>
#include <ostream>
#include <iomanip>
#include <format>
#include <unordered_map>
#include <cstdio>

enum checks {
  MFG_INFO,
  DEVICE_INFO,
  SYSTEM_PARAMS,
  MFG_PARAMS,
  CAP_PARAMS
};

std::string expected_cmds[5] = {
  "~22014A510000FDA0",
  "~22014A42E00201FD28",
  "~22014A47E00201FD23",
  "~22014AB0600A010103FF00FB6C",
  "~22014AB0600A010104FF00FB6B"
};

std::string payloads[5] = {
  "~22014A00204A54310000000000000000535444303800000000003136533130304A43323501050502154843EF0D\r",
  "~22014A00E0C60025D214CE100D010D010D010D010D020D020D010D010D010D010D010D010D020D010D010D0100A00081008B0400830085007D008000000000006401283426ED0001000000000000000010230000000000000000000000000000000000000000004000D5EF\r",
  "~22014A006082000E4209C4003CFFFB2EE00E4209C400102EE0283405A00000494E2D45434A3834303100300000000000000000494E2D45434A3834303132304335303337320036E2A7\r",
  "~22014A0040EEB0010103FF714E49452D4A433438313030323543333032373600000000000000000000004E49452D4A433438313030000000000000000000000000000000000000004E49452D4A433438313030323543333032373600000000000000000000001909184E49450000000000000000000000000000000000CE61\r",
  "~22014A00D030B0010104FF1226ED28342710000000ED0000009900810050F3B3\r",
};

uint8_t bms_id = 1;

void print_result(std::unordered_map<std::string, std::string> data) {
  for (const auto& pair : data) {
    std::cout << pair.first << ": " << pair.second << std::endl;
  }
}

void check_mfg_info(){
  std::string cmd = esphome::daren_bms::build_command(bms_id, esphome::daren_bms::CMD_MFG_INFO);
  cmd = cmd.substr(0, cmd.length()-1);
  std::cout << "MFG INFO: ";
  if(cmd == expected_cmds[checks::MFG_INFO]) {
    std::cout << "OK";
  } else {
    std::cout << std::format("Not ok: {} != {}", cmd, expected_cmds[checks::MFG_INFO]) << std::endl;
  }
  std::cout << std::endl;
}

void check_device_info(){
  std::string cmd = esphome::daren_bms::build_command(bms_id, esphome::daren_bms::CMD_DEVICE_INFO);
  cmd = cmd.substr(0, cmd.length()-1);
  std::cout << "DEVICE INFO: ";
  if(cmd == expected_cmds[checks::DEVICE_INFO]) {
    std::cout << "OK";
  } else {
    std::cout << std::format("Not ok: {} != {}", cmd, expected_cmds[checks::DEVICE_INFO]);
  }
  std::cout << std::endl;
}

void check_system_params(){
  std::string cmd = esphome::daren_bms::build_command(bms_id, esphome::daren_bms::CMD_SYSTEM_PARAMS);
  cmd = cmd.substr(0, cmd.length()-1);
  std::cout << "SYSTEM PARAMS: ";
  if(cmd == expected_cmds[checks::SYSTEM_PARAMS]) {
    std::cout << "OK";
  } else {
    std::cout << std::format("Not ok: {} != {}", cmd, expected_cmds[checks::SYSTEM_PARAMS]);
  }
  std::cout << std::endl;
}

void check_mfg_params(){
  std::string cmd = esphome::daren_bms::build_command(bms_id, esphome::daren_bms::CMD_PARAMS, esphome::daren_bms::CMD_PARAMS_MOD_MFG);
  cmd = cmd.substr(0, cmd.length()-1);
  std::cout << "MFG PARAMS: ";
  if(cmd == expected_cmds[checks::MFG_PARAMS]) {
    std::cout << "OK";
  } else {
    std::cout << std::format("Not ok: {} != {}", cmd, expected_cmds[checks::MFG_PARAMS]);
  }
  std::cout << std::endl;
}

void check_cap_params(){
  std::string cmd = esphome::daren_bms::build_command(bms_id, esphome::daren_bms::CMD_PARAMS, esphome::daren_bms::CMD_PARAMS_MOD_CAP);
  cmd = cmd.substr(0, cmd.length()-1);
  std::cout << "CAP PARAMS: ";
  if(cmd == expected_cmds[checks::CAP_PARAMS]) {
    std::cout << "OK";
  } else {
    std::cout << std::format("Not ok: {} != {}", cmd, expected_cmds[checks::CAP_PARAMS]);
  }
  std::cout << std::endl;
}

void check_decode_mfg_info() {
  esphome::StaticVector<uint8_t,BUF_MAX_SIZE> payload;
  std::cout << "\nMFG INFO\n" << std::endl;
  if(esphome::daren_bms::validate_response(bms_id, payloads[checks::MFG_INFO], payload)) {
    std::unordered_map<std::string, std::string>result = esphome::daren_bms::unpack_mfg_info(payload);
    print_result(result);
  } else {
    std::cout << "Validation failed" << std::endl;
  }
}

void check_decode_device_info() {
  esphome::StaticVector<uint8_t,BUF_MAX_SIZE> payload;
  std::cout << "\nDEVICE INFO\n" << std::endl;
  if(esphome::daren_bms::validate_response(bms_id, payloads[checks::DEVICE_INFO], payload)) {
    std::unordered_map<std::string, std::string>result = esphome::daren_bms::unpack_device_info(payload);
    print_result(result);
  } else {
    std::cout << "Validation failed" << std::endl;
  }
}

void check_decode_system_params() {
  esphome::StaticVector<uint8_t,BUF_MAX_SIZE> payload;
  std::cout << "\nSYSTEM PARAMS\n" << std::endl;
  if(esphome::daren_bms::validate_response(bms_id, payloads[checks::SYSTEM_PARAMS], payload)) {
    std::unordered_map<std::string, std::string>result = esphome::daren_bms::unpack_system_params(payload);
    print_result(result);
  } else {
    std::cout << "Validation failed" << std::endl;
  }
}

void check_decode_mfg_params() {
  esphome::StaticVector<uint8_t,BUF_MAX_SIZE> payload;
  std::cout << "\nMFG PARAMS\n" << std::endl;
  if(esphome::daren_bms::validate_response(bms_id, payloads[checks::MFG_PARAMS], payload)) {
    std::unordered_map<std::string, std::string>result = esphome::daren_bms::unpack_mfg_params(payload);
    print_result(result);
  } else {
    std::cout << "Validation failed" << std::endl;
  }
}

void check_decode_cap_params() {
  esphome::StaticVector<uint8_t,BUF_MAX_SIZE> payload;
  std::cout << "\nCAP PARAMS\n" << std::endl;
  if(esphome::daren_bms::validate_response(bms_id, payloads[checks::CAP_PARAMS], payload)) {
    std::unordered_map<std::string, std::string>result = esphome::daren_bms::unpack_cap_params(payload);
    print_result(result);
  } else {
    std::cout << "Validation failed" << std::endl;
  }
}

int main() {
  esphome::daren_bms::DarenBMS *bms = new esphome::daren_bms::DarenBMS();
  uint8_t bms_id = bms->get_bms_id();

  std::cout << "BMS id: "  << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bms_id) << std::endl;
  std::cout << "Command generation" << std::endl;
  check_mfg_info();
  check_device_info();
  check_system_params();
  check_mfg_params();
  check_cap_params();

  std::cout << "Response parsing" << std::endl;
  check_decode_mfg_info();
  check_decode_device_info();
  check_decode_system_params();
  check_decode_mfg_params();
  check_decode_cap_params();
}

#define TESTING 1

#include "components/daren_bms/protocol.h"
#include <cstdint>
#include <string>
#include <iostream>
#include <ostream>
#include <iomanip>
#include <format>
#include <unordered_map>
#include <cstdio>

enum checks { MFG_INFO, DEVICE_INFO, SYSTEM_PARAMS, MFG_PARAMS, CAP_PARAMS };

std::string expected_cmds[5] = {"~22014A510000FDA0", "~22014A42E00201FD28", "~22014A47E00201FD23",
                                "~22014AB0600A010103FF00FB6C", "~22014AB0600A010104FF00FB6B"};

std::string payloads[5] = {
    "~22014A00204A54310000000000000000535444303800000000003136533130304A43323501050502154843EF0D\r",
    "~22014A00E0C60025D214CE100D010D010D010D010D020D020D010D010D010D010D010D010D020D010D010D0100A00081008B0400830085007"
    "D008000000000006401283426ED0001000000000000000010230000000000000000000000000000000000000000004000D5EF\r",
    "~22014A006082000E4209C4003CFFFB2EE00E4209C400102EE0283405A00000494E2D45434A3834303100300000000000000000494E2D45434"
    "A3834303132304335303337320036E2A7\r",
    "~22014A0040EEB0010103FF714E49452D4A433438313030323543333032373600000000000000000000004E49452D4A4334383130300000000"
    "00000000000000000000000000000004E49452D4A433438313030323543333032373600000000000000000000001909184E494500000000000"
    "00000000000000000000000CE61\r",
    "~22014A00D030B0010104FF1226ED28342710000000ED0000009900810050F3B3\r",
};

uint8_t bms_id = 1;

void print_result(std::unordered_map<std::string, std::string> data) {
  for (const auto &pair : data) {
    std::cout << pair.first << ": " << pair.second << std::endl;
  }
}

void check_mfg_info() {
  std::string cmd = esphome::daren_bms::build_command(bms_id, esphome::daren_bms::CMD_MFG_INFO);
  cmd = cmd.substr(0, cmd.length() - 1);
  std::cout << "MFG INFO: ";
  if (cmd == expected_cmds[checks::MFG_INFO]) {
    std::cout << "OK";
  } else {
    std::cout << std::format("Not ok: {} != {}", cmd, expected_cmds[checks::MFG_INFO]) << std::endl;
  }
  std::cout << std::endl;
}

void check_device_info() {
  std::string cmd = esphome::daren_bms::build_command(bms_id, esphome::daren_bms::CMD_DEVICE_INFO);
  cmd = cmd.substr(0, cmd.length() - 1);
  std::cout << "DEVICE INFO: ";
  if (cmd == expected_cmds[checks::DEVICE_INFO]) {
    std::cout << "OK";
  } else {
    std::cout << std::format("Not ok: {} != {}", cmd, expected_cmds[checks::DEVICE_INFO]);
  }
  std::cout << std::endl;
}

void check_system_params() {
  std::string cmd = esphome::daren_bms::build_command(bms_id, esphome::daren_bms::CMD_SYSTEM_PARAMS);
  cmd = cmd.substr(0, cmd.length() - 1);
  std::cout << "SYSTEM PARAMS: ";
  if (cmd == expected_cmds[checks::SYSTEM_PARAMS]) {
    std::cout << "OK";
  } else {
    std::cout << std::format("Not ok: {} != {}", cmd, expected_cmds[checks::SYSTEM_PARAMS]);
  }
  std::cout << std::endl;
}

void check_mfg_params() {
  std::string cmd =
      esphome::daren_bms::build_command(bms_id, esphome::daren_bms::CMD_PARAMS, esphome::daren_bms::CMD_PARAMS_MOD_MFG);
  cmd = cmd.substr(0, cmd.length() - 1);
  std::cout << "MFG PARAMS: ";
  if (cmd == expected_cmds[checks::MFG_PARAMS]) {
    std::cout << "OK";
  } else {
    std::cout << std::format("Not ok: {} != {}", cmd, expected_cmds[checks::MFG_PARAMS]);
  }
  std::cout << std::endl;
}

void check_cap_params() {
  std::string cmd =
      esphome::daren_bms::build_command(bms_id, esphome::daren_bms::CMD_PARAMS, esphome::daren_bms::CMD_PARAMS_MOD_CAP);
  cmd = cmd.substr(0, cmd.length() - 1);
  std::cout << "CAP PARAMS: ";
  if (cmd == expected_cmds[checks::CAP_PARAMS]) {
    std::cout << "OK";
  } else {
    std::cout << std::format("Not ok: {} != {}", cmd, expected_cmds[checks::CAP_PARAMS]);
  }
  std::cout << std::endl;
}

void check_decode_mfg_info() {
  esphome::StaticVector<uint8_t, BUF_MAX_SIZE> payload;
  std::cout << "\nMFG INFO\n" << std::endl;
  if (esphome::daren_bms::validate_response(bms_id, payloads[checks::MFG_INFO], payload)) {
    esphome::daren_bms::MfgInfo result = esphome::daren_bms::unpack_mfg_info(payload);
    std::cout << "hwtype: " << result.hwtype << std::endl;
    std::cout << "product_code: " << result.product_code << std::endl;
    std::cout << "project_code: " << result.project_code << std::endl;
    std::cout << "software_version: " << result.software_version << std::endl;
  } else {
    std::cout << "Validation failed" << std::endl;
  }
}

void check_decode_device_info() {
  esphome::StaticVector<uint8_t, BUF_MAX_SIZE> payload;
  std::cout << "\nDEVICE INFO\n" << std::endl;
  if (esphome::daren_bms::validate_response(bms_id, payloads[checks::DEVICE_INFO], payload)) {
    esphome::daren_bms::DeviceInfo result = esphome::daren_bms::unpack_device_info(payload);
    std::cout << "soc: " << result.soc << std::endl;
    std::cout << "voltage: " << result.voltage << std::endl;
    std::cout << "cell_count: " << static_cast<int>(result.cell_count) << std::endl;
    std::cout << "cell_1_voltage: " << result.cell_1_voltage << std::endl;
    std::cout << "cell_2_voltage: " << result.cell_2_voltage << std::endl;
    std::cout << "cell_3_voltage: " << result.cell_3_voltage << std::endl;
    std::cout << "cell_4_voltage: " << result.cell_4_voltage << std::endl;
    std::cout << "cell_5_voltage: " << result.cell_5_voltage << std::endl;
    std::cout << "cell_6_voltage: " << result.cell_6_voltage << std::endl;
    std::cout << "cell_7_voltage: " << result.cell_7_voltage << std::endl;
    std::cout << "cell_8_voltage: " << result.cell_8_voltage << std::endl;
    std::cout << "cell_9_voltage: " << result.cell_9_voltage << std::endl;
    std::cout << "cell_10_voltage: " << result.cell_10_voltage << std::endl;
    std::cout << "cell_11_voltage: " << result.cell_11_voltage << std::endl;
    std::cout << "cell_12_voltage: " << result.cell_12_voltage << std::endl;
    std::cout << "cell_13_voltage: " << result.cell_13_voltage << std::endl;
    std::cout << "cell_14_voltage: " << result.cell_14_voltage << std::endl;
    std::cout << "cell_15_voltage: " << result.cell_15_voltage << std::endl;
    std::cout << "cell_16_voltage: " << result.cell_16_voltage << std::endl;
    std::cout << "env_temp: " << result.env_temp << std::endl;
    std::cout << "pack_temp: " << result.pack_temp << std::endl;
    std::cout << "mos_temp: " << result.mos_temp << std::endl;
    std::cout << "tot_temps: " << static_cast<int>(result.tot_temps) << std::endl;
    std::cout << "temp_1: " << result.temp_1 << std::endl;
    std::cout << "temp_2: " << result.temp_2 << std::endl;
    std::cout << "temp_3: " << result.temp_3 << std::endl;
    std::cout << "temp_4: " << result.temp_4 << std::endl;
    std::cout << "current: " << result.current << std::endl;
    std::cout << "internal_r: " << result.internal_r << std::endl;
    std::cout << "soh: " << result.soh << std::endl;
    std::cout << "user_custom: " << static_cast<int>(result.user_custom) << std::endl;
    std::cout << "charge_full_ah: " << result.charge_full_ah << std::endl;
    std::cout << "charge_now_ah: " << result.charge_now_ah << std::endl;
    std::cout << "cycle_count: " << result.cycle_count << std::endl;
    std::cout << "voltage_status: " << result.voltage_status << std::endl;
    std::cout << "current_status: " << result.current_status << std::endl;
    std::cout << "temp_status: " << result.temp_status << std::endl;
    std::cout << "fet_status: " << result.fet_status << std::endl;
    std::cout << "overvolt_prot_status_l: " << result.overvolt_prot_status_l << std::endl;
    std::cout << "undervolt_prot_status_l: " << result.undervolt_prot_status_l << std::endl;
    std::cout << "overvolt_alarm_status_l: " << result.overvolt_alarm_status_l << std::endl;
    std::cout << "undervolt_alarm_status_l: " << result.undervolt_alarm_status_l << std::endl;
    std::cout << "balance_state_l: " << result.balance_state_l << std::endl;
    std::cout << "balance_state_h: " << result.balance_state_h << std::endl;
    std::cout << "overvolt_prot_status_h: " << result.overvolt_prot_status_h << std::endl;
    std::cout << "undervolt_prot_status_h: " << result.undervolt_prot_status_h << std::endl;
    std::cout << "overvolt_alarm_status_h: " << result.overvolt_alarm_status_h << std::endl;
    std::cout << "undervolt_alarm_status_h: " << result.undervolt_alarm_status_h << std::endl;
    std::cout << "machine_status_list: " << static_cast<int>(result.machine_status_list) << std::endl;
    std::cout << "io_status_list: " << result.io_status_list << std::endl;
  } else {
    std::cout << "Validation failed" << std::endl;
  }
}

void check_decode_system_params() {
  esphome::StaticVector<uint8_t, BUF_MAX_SIZE> payload;
  std::cout << "\nSYSTEM PARAMS\n" << std::endl;
  if (esphome::daren_bms::validate_response(bms_id, payloads[checks::SYSTEM_PARAMS], payload)) {
    esphome::daren_bms::SystemParams result = esphome::daren_bms::unpack_system_params(payload);
    std::cout << "cell_v_upper_limit: " << result.cell_v_upper_limit << std::endl;
    std::cout << "cell_v_lower_limit: " << result.cell_v_lower_limit << std::endl;
    std::cout << "temp_upper_limit: " << result.temp_upper_limit << std::endl;
    std::cout << "temp_lower_limit: " << result.temp_lower_limit << std::endl;
    std::cout << "chg_c_upper_limit: " << result.chg_c_upper_limit << std::endl;
    std::cout << "tot_v_upper_limit: " << result.tot_v_upper_limit << std::endl;
    std::cout << "tot_v_lower_limit: " << result.tot_v_lower_limit << std::endl;
    std::cout << "number_of_cells: " << result.number_of_cells << std::endl;
    std::cout << "chg_c_limit: " << result.chg_c_limit << std::endl;
    std::cout << "design_cap_none: " << result.design_cap_none << std::endl;
    std::cout << "history_storage_interval: " << result.history_storage_interval << std::endl;
    std::cout << "balanced_mode: " << result.balanced_mode << std::endl;
    std::cout << "product_barcode: " << result.product_barcode << std::endl;
    std::cout << "bms_barcode: " << result.bms_barcode << std::endl;
  } else {
    std::cout << "Validation failed" << std::endl;
  }
}

void check_decode_mfg_params() {
  esphome::StaticVector<uint8_t, BUF_MAX_SIZE> payload;
  std::cout << "\nMFG PARAMS\n" << std::endl;
  if (esphome::daren_bms::validate_response(bms_id, payloads[checks::MFG_PARAMS], payload)) {
    esphome::daren_bms::MfgParams result = esphome::daren_bms::unpack_mfg_params(payload);
    std::cout << "pack_sn: " << result.pack_sn << std::endl;
    std::cout << "product_id: " << result.product_id << std::endl;
    std::cout << "bms_id: " << result.bms_id << std::endl;
    std::cout << "manufacturer: " << result.manufacturer << std::endl;
    std::cout << "born_date: " << result.born_date << std::endl;
  } else {
    std::cout << "Validation failed" << std::endl;
  }
}

void check_decode_cap_params() {
  esphome::StaticVector<uint8_t, BUF_MAX_SIZE> payload;
  std::cout << "\nCAP PARAMS\n" << std::endl;
  if (esphome::daren_bms::validate_response(bms_id, payloads[checks::CAP_PARAMS], payload)) {
    esphome::daren_bms::CapParams result = esphome::daren_bms::unpack_cap_params(payload);
    std::cout << "remaining_cap_ah: " << result.remaining_cap_ah << std::endl;
    std::cout << "full_cap_ah: " << result.full_cap_ah << std::endl;
    std::cout << "design_cap_ah: " << result.design_cap_ah << std::endl;
    std::cout << "total_charge_cap_ah: " << result.total_charge_cap_ah << std::endl;
    std::cout << "total_discharge_cap_ah: " << result.total_discharge_cap_ah << std::endl;
    std::cout << "total_charge_kwh: " << result.total_charge_kwh << std::endl;
    std::cout << "total_discharge_kwh: " << result.total_discharge_kwh << std::endl;
  } else {
    std::cout << "Validation failed" << std::endl;
  }
}

int main() {
  std::cout << "BMS id: " << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bms_id)
            << std::endl;
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

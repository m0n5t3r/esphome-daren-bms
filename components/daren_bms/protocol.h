#include <array>
#ifndef DR_PROTOCOL_H
#define DR_PROTOCOL_H 1
#include <cstdint>
#include <string>
#include "esphome/core/helpers.h"

#ifndef BUF_MAX_SIZE
#define BUF_MAX_SIZE 130
#endif // !BUF_MAX_SIZE

namespace esphome {
  namespace daren_bms {

    static const uint8_t VER = 0x22;
    static const uint8_t CID1 = 0x4A;
    struct DICT_ENTRY {
      uint8_t key;
      const char* value;
    };
    static const std::array<DICT_ENTRY, 9> CID2_CODES = {{
      {0x00, "CID2 response ok."},
      {0x01, "VER error."},
      {0x02, "CHKSUM error."},
      {0x03, "LCHKSUM error."},
      {0x04, "CID2 invalid."},
      {0x05, "Command format error."},
      {0x06, "INFO data invalid."},
      {0x90, "ADR error."},
      {0x91, "Battery communication error."},
    }};

    static const uint8_t CMD_SYSTEM_PARAMS = 0x47;
    static const uint8_t CMD_PROTOCOL_VERSION = 0x4F;
    static const uint8_t CMD_MFG_INFO = 0x51;
    static const uint8_t CMD_DEVICE_INFO = 0x42;
    static const uint8_t CMD_PARAMS = 0xB0;
    static const uint8_t CMD_PARAMS_MOD_OCV = 0x01;
    static const uint8_t CMD_PARAMS_MOD_HWPROT = 0x02;
    static const uint8_t CMD_PARAMS_MOD_MFG = 0x03;
    static const uint8_t CMD_PARAMS_MOD_CAP = 0x04;

    // parse hex string into byte array
    StaticVector<uint8_t, BUF_MAX_SIZE> parse_hex(std::string str);
    // assemble command
    std::string build_command(uint8_t device_address, uint8_t cid2, const StaticVector<uint8_t,BUF_MAX_SIZE> info);
    std::string build_command(uint8_t device_address, uint8_t cid2, const uint8_t module_id=0);
    // validate response and extract payload
    bool validate_response(uint8_t device_address, std::string buf, StaticVector<uint8_t,BUF_MAX_SIZE> &payload);

    // Helper methods
    uint16_t length_checksum(uint16_t value);
    uint16_t checksum(const std::string &s);
    void append_hex(std::string &str, uint8_t value);
    void append_hex(std::string &str, uint16_t value);

    struct MfgInfo {
      std::string hwtype;
      std::string product_code;
      std::string project_code;
      std::string software_version;
    };

    struct DeviceInfo {
      float soc;
      float voltage;
      uint8_t cell_count;
      float cell_1_voltage;
      float cell_2_voltage;
      float cell_3_voltage;
      float cell_4_voltage;
      float cell_5_voltage;
      float cell_6_voltage;
      float cell_7_voltage;
      float cell_8_voltage;
      float cell_9_voltage;
      float cell_10_voltage;
      float cell_11_voltage;
      float cell_12_voltage;
      float cell_13_voltage;
      float cell_14_voltage;
      float cell_15_voltage;
      float cell_16_voltage;
      float env_temp;
      float pack_temp;
      float mos_temp;
      uint8_t tot_temps;
      float temp_1;
      float temp_2;
      float temp_3;
      float temp_4;
      float current;
      float internal_r;
      uint16_t soh;
      uint8_t user_custom;
      float charge_full_ah;
      float charge_now_ah;
      uint16_t cycle_count;
      uint16_t voltage_status;
      uint16_t current_status;
      uint16_t temp_status;
      uint16_t fet_status;
      uint16_t overvolt_prot_status_l;
      uint16_t undervolt_prot_status_l;
      uint16_t overvolt_alarm_status_l;
      uint16_t undervolt_alarm_status_l;
      uint16_t balance_state_l;
      uint16_t balance_state_h;
      uint16_t overvolt_prot_status_h;
      uint16_t undervolt_prot_status_h;
      uint16_t overvolt_alarm_status_h;
      uint16_t undervolt_alarm_status_h;
      uint8_t machine_status_list;
      uint16_t io_status_list;
    };

    struct SystemParams {
      float cell_v_upper_limit;
      float cell_v_lower_limit;
      int16_t temp_upper_limit;
      int16_t temp_lower_limit;
      float chg_c_upper_limit;
      float tot_v_upper_limit;
      float tot_v_lower_limit;
      uint16_t number_of_cells;
      float chg_c_limit;
      float design_cap_none;
      uint16_t history_storage_interval;
      uint16_t balanced_mode;
      std::string product_barcode;
      std::string bms_barcode;
    };

    struct MfgParams {
      std::string pack_sn;
      std::string product_id;
      std::string bms_id;
      std::string manufacturer;
      std::string born_date;
    };

    struct CapParams {
      float remaining_cap_ah;
      float full_cap_ah;
      float design_cap_ah;
      float total_charge_cap_ah;
      float total_discharge_cap_ah;
      float total_charge_kwh;
      float total_discharge_kwh;
    };

    MfgInfo unpack_mfg_info(StaticVector<uint8_t,BUF_MAX_SIZE> payload);
    DeviceInfo unpack_device_info(StaticVector<uint8_t,BUF_MAX_SIZE> payload);
    SystemParams unpack_system_params(StaticVector<uint8_t,BUF_MAX_SIZE> payload);
    MfgParams unpack_mfg_params(StaticVector<uint8_t,BUF_MAX_SIZE> payload);
    CapParams unpack_cap_params(StaticVector<uint8_t,BUF_MAX_SIZE> payload);

#ifdef TESTING
    char* format_hex(StaticVector<uint8_t,BUF_MAX_SIZE> data);
#endif
  }
}
#endif // DR_PROTOCOL_H

#include "protocol.h"
#include <cstdint>
#include <cstdio>
#include <format>
#include <string>

#ifndef TESTING
#include "esphome/core/log.h"
#endif // !TESTING

namespace esphome {
  namespace daren_bms {
    void append_hex(std::string &str, uint8_t value) {
      char hex[3];
      sprintf(hex, "%02X", value);
      str += hex[0];
      str += hex[1];
    }

    void append_hex(std::string &str, uint16_t value) {
      char hex[5];
      sprintf(hex, "%02X", value);
      str += hex[0];
      str += hex[1];
      str += hex[2];
      str += hex[3];
    }

    StaticVector<uint8_t,BUF_MAX_SIZE> parse_hex(std::string str) {
      StaticVector<uint8_t,BUF_MAX_SIZE> parsed = {};
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

    bool validate_response(uint8_t bms_id, std::string buf, StaticVector<uint8_t,BUF_MAX_SIZE> &payload) {
      StaticVector<uint8_t,BUF_MAX_SIZE>response = parse_hex(buf);

      if (response.size() < 6) {
        return false;  // Minimum response size
      }

      // Check start of frame
      if (response[0] != '~') {
        return false;
      }

      // Check version
      if (response[1] != VER) {
        return false;
      }

      // Check address
      if (response[2] != bms_id) {
        return false;
      }

      // Check response CID1 (should be 0xA5 for responses)
      if (response[3] != CID1) {
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
      uint16_t calc_checksum = checksum(buf.substr(1, buf.length() - 6));
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

    std::string build_command(uint8_t bms_id, uint8_t cid2, const StaticVector<uint8_t,BUF_MAX_SIZE> info) {
      std::string cmd;
      size_t len = info.size();

      cmd += '~';
      append_hex(cmd, VER);
      append_hex(cmd, bms_id);
      append_hex(cmd, CID1);
      append_hex(cmd, cid2);

      // Length and checksum
      if (len > 0) {
        uint16_t length_cs = length_checksum(len * 2);  // len in hex chars
        append_hex(cmd, length_cs);

        // Data
        for (size_t i = 0; i < len; i++) {
          append_hex(cmd, info[i]);
        }
      } else {
        cmd += "0000";
      }

      // Calculate checksum of everything except SOF
      uint16_t cksum = checksum(cmd.substr(1));
      append_hex(cmd, cksum);

      // End with CR
      cmd += '\r';

      return cmd;
    }

    std::string build_command(uint8_t bms_id, uint8_t cid2, uint8_t module_id) {
      switch(cid2) {
        case CMD_PARAMS:
          return build_command(bms_id, cid2, StaticVector<uint8_t,BUF_MAX_SIZE>{bms_id, 0x01, module_id, 0xff, 0x00});
        case CMD_MFG_INFO:
        case CMD_PROTOCOL_VERSION:
          return build_command(bms_id, cid2, StaticVector<uint8_t,BUF_MAX_SIZE>{});
        default:
          return build_command(bms_id, cid2, StaticVector<uint8_t,BUF_MAX_SIZE>{bms_id});
      }
    }

    uint16_t length_checksum(uint16_t value) {
      value &= 0x0FFF;
      uint16_t n1 = value & 0xF;
      uint16_t n2 = (value >> 4) & 0xF;
      uint16_t n3 = (value >> 8) & 0xF;
      uint16_t cksum = ((n1 + n2 + n3) & 0xF) ^ 0xF;
      cksum += 1;
      return value + (cksum << 12);
    }

    uint16_t checksum(const std::string &s) {
      uint16_t cksum = 0;
      for (char c : s) {
        cksum += static_cast<uint8_t>(c);
      }
      cksum ^= 0xFFFF;
      return cksum + 1;
    }

    MfgInfo unpack_mfg_info(StaticVector<uint8_t,BUF_MAX_SIZE> payload) {
      MfgInfo result;
      result.hwtype = std::string(payload.begin(), payload.begin() + 10);
      result.product_code = std::string(payload.begin() + 10, payload.begin() + 20);
      result.project_code = std::string(payload.begin() + 20, payload.begin() + 30);
      result.software_version = std::format("{:02d}.{:02d}.{:02d}", payload[30], payload[31], payload[32]);
      return result;
    }

    DeviceInfo unpack_device_info(StaticVector<uint8_t,BUF_MAX_SIZE> payload) {
      DeviceInfo result;
      uint8_t cell_count = payload[5];
      result.soc = static_cast<float>(payload[1] << 8 | payload[2]) / 100.0f;
      result.voltage = static_cast<float>(payload[3] << 8 | payload[4]) / 100.0f;
      result.cell_count = cell_count;

      size_t off = 6;
      result.cell_1_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_2_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_3_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_4_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_5_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_6_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_7_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_8_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_9_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_10_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_11_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_12_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_13_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_14_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_15_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;
      result.cell_16_voltage = static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f;
      off += 2;

      uint8_t tot_temps = payload[off + 6];
      result.env_temp = static_cast<float>(static_cast<int16_t>(payload[off] << 8 | payload[off + 1])) / 10.0f;
      result.pack_temp = static_cast<float>(static_cast<int16_t>(payload[off + 2] << 8 | payload[off + 3])) / 10.0f;
      result.mos_temp = static_cast<float>(static_cast<int16_t>(payload[off + 4] << 8 | payload[off + 5])) / 10.0f;
      result.tot_temps = tot_temps;

      off += 7;
      result.temp_1 = static_cast<float>(static_cast<int16_t>(payload[off] << 8 | payload[off + 1])) / 10.0f;
      off += 2;
      result.temp_2 = static_cast<float>(static_cast<int16_t>(payload[off] << 8 | payload[off + 1])) / 10.0f;
      off += 2;
      result.temp_3 = static_cast<float>(static_cast<int16_t>(payload[off] << 8 | payload[off + 1])) / 10.0f;
      off += 2;
      result.temp_4 = static_cast<float>(static_cast<int16_t>(payload[off] << 8 | payload[off + 1])) / 10.0f;
      off += 2;

      result.current = static_cast<float>(static_cast<int16_t>(payload[off] << 8 | payload[off + 1])) / 100.0f;
      result.internal_r = static_cast<float>(payload[off + 2] << 8 | payload[off + 3]) / 10.0f;
      result.soh = payload[off + 4] << 8 | payload[off + 5];
      result.user_custom = payload[off + 6];
      result.charge_full_ah = static_cast<float>(payload[off + 7] << 8 | payload[off + 8]) / 100.0f;
      result.charge_now_ah = static_cast<float>(payload[off + 9] << 8 | payload[off + 10]) / 100.0f;
      result.cycle_count = payload[off + 11] << 8 | payload[off + 12];
      result.voltage_status = payload[off + 13] << 8 | payload[off + 14];
      result.current_status = payload[off + 15] << 8 | payload[off + 16];
      result.temp_status = payload[off + 17] << 8 | payload[off + 18];
      result.fet_status = payload[off + 19] << 8 | payload[off + 20];
      result.overvolt_prot_status_l = payload[off + 21] << 8 | payload[off + 22];
      result.undervolt_prot_status_l = payload[off + 23] << 8 | payload[off + 24];
      result.overvolt_alarm_status_l = payload[off + 25] << 8 | payload[off + 26];
      result.undervolt_alarm_status_l = payload[off + 27] << 8 | payload[off + 28];
      result.balance_state_l = payload[off + 29] << 8 | payload[off + 30];
      result.balance_state_h = payload[off + 31] << 8 | payload[off + 32];
      result.overvolt_prot_status_h = payload[off + 33] << 8 | payload[off + 34];
      result.undervolt_prot_status_h = payload[off + 35] << 8 | payload[off + 36];
      result.overvolt_alarm_status_h = payload[off + 37] << 8 | payload[off + 38];
      result.undervolt_alarm_status_h = payload[off + 39] << 8 | payload[off + 40];
      result.machine_status_list = payload[off + 41];
      result.io_status_list = payload[off + 42] << 8 | payload[off + 43];

      return result;
    }

    SystemParams unpack_system_params(StaticVector<uint8_t,BUF_MAX_SIZE> payload) {
      SystemParams result;
      result.cell_v_upper_limit = static_cast<float>(payload[1] << 8 | payload[2]) / 1000.0f;
      result.cell_v_lower_limit = static_cast<float>(payload[3] << 8 | payload[4]) / 1000.0f;
      result.temp_upper_limit = static_cast<int16_t>(payload[5] << 8 | payload[6]);
      result.temp_lower_limit = static_cast<int16_t>(payload[7] << 8 | payload[8]);
      result.chg_c_upper_limit = static_cast<float>(payload[9] << 8 | payload[10]) / 100.0f;
      result.tot_v_upper_limit = static_cast<float>(payload[11] << 8 | payload[12]) / 1000.0f;
      result.tot_v_lower_limit = static_cast<float>(payload[13] << 8 | payload[14]) / 1000.0f;
      result.number_of_cells = payload[15] << 8 | payload[16];
      result.chg_c_limit = static_cast<float>(payload[17] << 8 | payload[18]) / 100.0f;
      result.design_cap_none = static_cast<float>(payload[19] << 8 | payload[20]) / 100.0f;
      result.history_storage_interval = payload[21] << 8 | payload[22];
      result.balanced_mode = payload[23] << 8 | payload[24];
      result.product_barcode = std::string(payload.begin() + 25, payload.begin() + 45);
      result.bms_barcode = std::string(payload.begin() + 45, payload.begin() + 65);
      return result;
    }

    MfgParams unpack_mfg_params(StaticVector<uint8_t,BUF_MAX_SIZE> payload) {
      MfgParams result;
      result.pack_sn = std::string(payload.begin() + 6, payload.begin() + 36);
      result.product_id = std::string(payload.begin() + 36, payload.begin() + 66);
      result.bms_id = std::string(payload.begin() + 66, payload.begin() + 96);
      result.manufacturer = std::string(payload.begin() + 99, payload.begin() + 119);
      result.born_date = std::format("{}-{:02d}-{:02d}", payload[96] + 2000, payload[97], payload[98]);
      return result;
    }

    CapParams unpack_cap_params(StaticVector<uint8_t,BUF_MAX_SIZE> payload) {
      CapParams result;
      result.remaining_cap_ah = static_cast<float>(payload[6] << 8 | payload[7]) / 100.0f;
      result.full_cap_ah = static_cast<float>(payload[8] << 8 | payload[9]) / 100.0f;
      result.design_cap_ah = static_cast<float>(payload[10] << 8 | payload[11]) / 100.0f;
      result.total_charge_cap_ah = static_cast<float>(payload[12] << 8 | payload[13] << 8 | payload[14] << 8 | payload[15]) / 100.0f;
      result.total_discharge_cap_ah = static_cast<float>(payload[16] << 8 | payload[17] << 8 | payload[18] << 8 | payload[19]) / 100.0f;
      result.total_charge_kwh = static_cast<float>(payload[20] << 8 | payload[21]) / 100.0f;
      result.total_discharge_kwh = static_cast<float>(payload[22] << 8 | payload[23]) / 100.0f;
      return result;
    }

#ifdef TESTING
    char* format_hex(StaticVector<uint8_t,BUF_MAX_SIZE> data) {
      char* hexString = new char[data.size() * 2 + 1];

      for (size_t i = 0; i < data.size(); ++i) {
        // Convert each byte to its hexadecimal representation
        sprintf(hexString + (i * 2), "%02X", data[i]);
      }

      // Null-terminate the string
      hexString[data.size() * 2] = '\0';
      return hexString;
    }
#endif // TESTING

  }
}

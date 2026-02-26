#include "protocol.h"
#include <cstdint>
#include <cstdio>
#include <format>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

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

    std::vector<uint8_t> parse_hex(std::string str) {
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

    bool validate_response(uint8_t bms_id, std::string buf, std::vector<uint8_t> &payload) {
      std::vector<uint8_t>response = parse_hex(buf);

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

    std::string build_command(uint8_t bms_id, uint8_t cid2, const std::vector<uint8_t> info) {
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
          return build_command(bms_id, cid2, std::vector<uint8_t>{bms_id, 0x01, module_id, 0xff, 0x00});
        case CMD_MFG_INFO:
        case CMD_PROTOCOL_VERSION:
          return build_command(bms_id, cid2, std::vector<uint8_t>{});
        default:
          return build_command(bms_id, cid2, std::vector<uint8_t>{bms_id});
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

    std::unordered_map<std::string, std::string> unpack_mfg_info(std::vector<uint8_t> payload) {
      std::unordered_map<std::string, std::string> result;
      result["hwtype"] = std::string(payload.begin(), payload.begin() + 10);
      result["product_code"] = std::string(payload.begin() + 10, payload.begin() + 20);
      result["project_code"] = std::string(payload.begin() + 20, payload.begin() + 30);
      result["software_version"] = std::format("{:02d}.{:02d}.{:02d}", payload[30], payload[31], payload[32]);
      return result;
    }

    std::unordered_map<std::string, std::string> unpack_device_info(std::vector<uint8_t> payload) {
      std::unordered_map<std::string, std::string> result;
      uint8_t cell_count = payload[5];
      result["soc"] = std::format("{:.2f}", static_cast<float>(payload[1] << 8 | payload[2]) / 100.0f);
      result["voltage"] = std::format("{:.2f}", static_cast<float>(payload[3] << 8 | payload[4]) / 100.0f);
      result["cell_count"] = std::to_string(cell_count);

      size_t off = 6;
      for (uint8_t cell_no = 0; cell_no < cell_count; cell_no++) {
        result[std::format("cell_{}_voltage", cell_no + 1)] = std::format("{:.3f}", static_cast<float>(payload[off] << 8 | payload[off + 1]) / 1000.0f);
        off += 2;
      }

      uint8_t tot_temps = payload[off + 6];
      result["env_temp"] = std::format("{:.1f}", static_cast<float>(static_cast<int16_t>(payload[off] << 8 | payload[off + 1])) / 10.0f);
      result["pack_temp"] = std::format("{:.1f}", static_cast<float>(static_cast<int16_t>(payload[off + 2] << 8 | payload[off + 3])) / 10.0f);
      result["mos_temp"] = std::format("{:.1f}", static_cast<float>(static_cast<int16_t>(payload[off + 4] << 8 | payload[off + 5])) / 10.0f);
      result["tot_temps"] = std::to_string(tot_temps);

      off += 7;
      for (uint8_t temp_no = 0; temp_no < tot_temps; temp_no++) {
        result[std::format("temp_{}", temp_no + 1)] = std::format("{:.1f}", static_cast<float>(static_cast<int16_t>(payload[off] << 8 | payload[off + 1])) / 10.0f);
        off += 2;
      }

      result["current"] = std::format("{:.2f}", static_cast<float>(static_cast<int16_t>(payload[off] << 8 | payload[off + 1])) / 100.0f);
      result["internal_r"] = std::format("{:.1f}", static_cast<float>(payload[off + 2] << 8 | payload[off + 3]) / 10.0f);
      result["soh"] = std::to_string(payload[off + 4] << 8 | payload[off + 5]);
      result["user_custom"] = std::to_string(payload[off + 6]);
      result["charge_full_ah"] = std::format("{:.2f}", static_cast<float>(payload[off + 7] << 8 | payload[off + 8]) / 100.0f);
      result["charge_now_ah"] = std::format("{:.2f}", static_cast<float>(payload[off + 9] << 8 | payload[off + 10]) / 100.0f);
      result["cycle_count"] = std::to_string(payload[off + 11] << 8 | payload[off + 12]);
      result["voltage_status"] = std::to_string(payload[off + 13] << 8 | payload[off + 14]);
      result["current_status"] = std::to_string(payload[off + 15] << 8 | payload[off + 16]);
      result["temp_status"] = std::to_string(payload[off + 17] << 8 | payload[off + 18]);
      result["fet_status"] = std::to_string(payload[off + 19] << 8 | payload[off + 20]);
      result["overvolt_prot_status_l"] = std::to_string(payload[off + 21] << 8 | payload[off + 22]);
      result["undervolt_prot_status_l"] = std::to_string(payload[off + 23] << 8 | payload[off + 24]);
      result["overvolt_alarm_status_l"] = std::to_string(payload[off + 25] << 8 | payload[off + 26]);
      result["undervolt_alarm_status_l"] = std::to_string(payload[off + 27] << 8 | payload[off + 28]);
      result["balance_state_l"] = std::to_string(payload[off + 29] << 8 | payload[off + 30]);
      result["balance_state_h"] = std::to_string(payload[off + 31] << 8 | payload[off + 32]);
      result["overvolt_prot_status_h"] = std::to_string(payload[off + 33] << 8 | payload[off + 34]);
      result["undervolt_prot_status_h"] = std::to_string(payload[off + 35] << 8 | payload[off + 36]);
      result["overvolt_alarm_status_h"] = std::to_string(payload[off + 37] << 8 | payload[off + 38]);
      result["undervolt_alarm_status_h"] = std::to_string(payload[off + 39] << 8 | payload[off + 40]);
      result["machine_status_list"] = std::to_string(payload[off + 41]);
      result["io_status_list"] = std::format("{:04x}", payload[off + 42] << 8 | payload[off + 43]);

      return result;
    }

    std::unordered_map<std::string, std::string> unpack_system_params(std::vector<uint8_t> payload) {
      std::unordered_map<std::string, std::string> result;
      result["cell_v_upper_limit"] = std::format("{:.3f}", static_cast<float>(payload[1] << 8 | payload[2]) / 1000.0f);
      result["cell_v_lower_limit"] = std::format("{:.3f}", static_cast<float>(payload[3] << 8 | payload[4]) / 1000.0f);
      result["temp_upper_limit"] = std::to_string(static_cast<int16_t>(payload[5] << 8 | payload[6]));
      result["temp_lower_limit"] = std::to_string(static_cast<int16_t>(payload[7] << 8 | payload[8]));
      result["chg_c_upper_limit"] = std::format("{:.2f}", static_cast<float>(payload[9] << 8 | payload[10]) / 100.0f);
      result["tot_v_upper_limit"] = std::format("{:.3f}", static_cast<float>(payload[11] << 8 | payload[12]) / 1000.0f);
      result["tot_v_lower_limit"] = std::format("{:.3f}", static_cast<float>(payload[13] << 8 | payload[14]) / 1000.0f);
      result["number_of_cells"] = std::to_string(payload[15] << 8 | payload[16]);
      result["chg_c_limit"] = std::format("{:.2f}", static_cast<float>(payload[17] << 8 | payload[18]) / 100.0f);
      result["design_cap_none"] = std::format("{:.2f}", static_cast<float>(payload[19] << 8 | payload[20]) / 100.0f);
      result["history_storage_interval"] = std::to_string(payload[21] << 8 | payload[22]);
      result["balanced_mode"] = std::to_string(payload[23] << 8 | payload[24]);
      result["product_barcode"] = std::string(payload.begin() + 25, payload.begin() + 45);
      result["bms_barcode"] = std::string(payload.begin() + 45, payload.begin() + 65);
      return result;
    }

    std::unordered_map<std::string, std::string> unpack_mfg_params(std::vector<uint8_t> payload) {
      std::unordered_map<std::string, std::string> result;
      result["pack_sn"] = std::string(payload.begin() + 6, payload.begin() + 36);
      result["product_id"] = std::string(payload.begin() + 36, payload.begin() + 66);
      result["bms_id"] = std::string(payload.begin() + 66, payload.begin() + 96);
      result["manufacturer"] = std::string(payload.begin() + 99, payload.begin() + 119);
      result["born_date"] = std::format("{}-{:02d}-{:02d}", payload[96] + 2000, payload[97], payload[98]);
      return result;
    }

    std::unordered_map<std::string, std::string> unpack_cap_params(std::vector<uint8_t> payload) {
      std::unordered_map<std::string, std::string> result;
      result["remaining_cap_ah"] = std::format("{:.2f}", static_cast<float>(payload[6] << 8 | payload[7]) / 100.0f);
      result["full_cap_ah"] = std::format("{:.2f}", static_cast<float>(payload[8] << 8 | payload[9]) / 100.0f);
      result["design_cap_ah"] = std::format("{:.2f}", static_cast<float>(payload[10] << 8 | payload[11]) / 100.0f);
      result["total_charge_cap_ah"] = std::format("{:.2f}", static_cast<float>(payload[12] << 8 | payload[13] << 8 | payload[14] << 8 | payload[15]) / 100.0f);
      result["total_discharge_cap_ah"] = std::format("{:.2f}", static_cast<float>(payload[16] << 8 | payload[17] << 8 | payload[18] << 8 | payload[19]) / 100.0f);
      result["total_charge_kwh"] = std::format("{:.2f}", static_cast<float>(payload[20] << 8 | payload[21]) / 100.0f);
      result["total_discharge_kwh"] = std::format("{:.2f}", static_cast<float>(payload[22] << 8 | payload[23]) / 100.0f);
      return result;
    }

#ifdef TESTING
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
#endif // TESTING

  }
}

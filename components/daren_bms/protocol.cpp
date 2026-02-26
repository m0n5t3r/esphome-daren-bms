#include "protocol.h"
#include <cstdint>
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


    bool validate_response(uint8_t bms_id, std::string &buf, std::vector<uint8_t> payload) {
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

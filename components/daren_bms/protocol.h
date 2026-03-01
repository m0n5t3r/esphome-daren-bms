#ifndef DR_PROTOCOL_H
#define DR_PROTOCOL_H 1
#include <cstdint>
#include <string>
#include <unordered_map>
#include "esphome/core/helpers.h"

#ifndef BUF_MAX_SIZE
#define BUF_MAX_SIZE 130
#endif // !BUF_MAX_SIZE


namespace esphome {
  namespace daren_bms {

    static const uint8_t VER = 0x22;
    static const uint8_t CID1 = 0x4A;
    static const std::unordered_map<uint8_t, std::string> CID2_CODES = {
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
    std::string build_command(uint8_t bms_id, uint8_t cid2, const StaticVector<uint8_t,BUF_MAX_SIZE> info);
    std::string build_command(uint8_t bms_id, uint8_t cid2, const uint8_t module_id=0);
    // validate response and extract payload
    bool validate_response(uint8_t bms_id, std::string buf, StaticVector<uint8_t,BUF_MAX_SIZE> &payload);

    // Helper methods
    uint16_t length_checksum(uint16_t value);
    uint16_t checksum(const std::string &s);
    void append_hex(std::string &str, uint8_t value);
    void append_hex(std::string &str, uint16_t value);

    std::unordered_map<std::string, std::string> unpack_mfg_info(StaticVector<uint8_t,BUF_MAX_SIZE> payload);
    std::unordered_map<std::string, std::string> unpack_device_info(StaticVector<uint8_t,BUF_MAX_SIZE> payload);
    std::unordered_map<std::string, std::string> unpack_system_params(StaticVector<uint8_t,BUF_MAX_SIZE> payload);
    std::unordered_map<std::string, std::string> unpack_mfg_params(StaticVector<uint8_t,BUF_MAX_SIZE> payload);
    std::unordered_map<std::string, std::string> unpack_cap_params(StaticVector<uint8_t,BUF_MAX_SIZE> payload);

#ifdef TESTING
    char* format_hex(StaticVector<uint8_t,BUF_MAX_SIZE> data);
#endif
  }
}
#endif // DR_PROTOCOL_H

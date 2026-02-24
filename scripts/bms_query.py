import serial
import sys
import logging
import json
from functools import cache
from datetime import datetime
import time
from collections.abc import Mapping, Callable

logger = logging.getLogger("root")
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.INFO)


class DarenProto:
    SOI: str = "~"
    VER: bytes = b"\x22"
    CID1: bytes = b"\x4A"

    def __init__(self, addr: bytes) -> None:
        self.addr: bytes = addr

    def get_system_params(self) -> str:
        """
        Get system parameters: voltage and current limits, barcode, etc.
        Sample command: ~22014A47E00201FD23
        """
        return self.command(b"\x47", self.addr)

    def get_protocol_version(self) -> str:
        """
        Get protocol version
        """
        return self.command(b"\x4F")

    def get_device_info(self) -> str:
        """
        Get device info suitable for monitoring: charge, voltages, alarms, etc.
        Sample command: ~22014A42E00201FD28
        """
        return self.command(b"\x42", self.addr)

    def get_mfg_info(self) -> str:
        """
        Get manufacturer info: project code, software version, etc.
        Sample command: ~22014A510000FDA0
        """
        return self.command(b"\x51")

    def get_params(self, module: bytes) -> str:
        info_bits = [
            self.addr,
            b"\x01",  # operation
            module,  # module
            b"\xFF",  # function id
            b"\x00",  # function len
        ]
        info = b"".join(info_bits)
        return self.command(b"\xB0", info)

    def get_ocv_params(self) -> str:
        return self.get_params(b"\x01")

    def get_hwprot_params(self) -> str:
        return self.get_params(b"\x02")

    def get_mfg_params(self) -> str:
        """
        Get manufacturer params: serial numbers, manufacturer, production date
        Sample command: ~22014AB0600A010103FF00FB6C
        """
        return self.get_params(b"\x03")

    def get_cap_params(self) -> str:
        """
        Get capacity info
        Sample commands: ~22014AB0600A010104FF00FB6B
        """
        return self.get_params(b"\x04")

    def command(self, cid2: bytes, info: bytes = b"") -> str:
        cmd_bits = [
            self.VER,
            self.addr,
            self.CID1,
            cid2,
        ]
        if len(info) > 0:
            length_cs = self.length_checksum(len(info.hex()))
            cmd_bits.append(length_cs.to_bytes(2))
            cmd_bits.append(info)
        else:
            cmd_bits.append(b"\x00\x00")
        cmd = self.SOI + b"".join(cmd_bits).hex().upper()
        cksum = self.checksum(cmd[1 : len(cmd)])
        cmd += cksum.to_bytes(2).hex()
        cmd += "\r"
        return cmd.upper()

    def length_checksum(self, value: int) -> int:
        value &= 0x0FFF
        n1 = value & 0xF
        n2 = (value >> 4) & 0xF
        n3 = (value >> 8) & 0xF
        cksum = ((n1 + n2 + n3) & 0xF) ^ 0xF
        cksum += 1
        return value + (cksum << 12)

    def checksum(self, s: str) -> int:
        cksum = 0
        for value in s:
            cksum += ord(value)
        cksum ^= 0xFFFF
        return cksum + 1

    def decode_cid2(self, cid2: bytes) -> int:
        codes = {
            b"00": "CID2 response ok.",
            b"01": "VER error.",
            b"02": "CHKSUM error.",
            b"03": "LCHKSUM error.",
            b"04": "CID2 invalid.",
            b"05": "Command format error.",
            b"06": "INFO data invalid.",
            b"90": "ADR error.",
            b"91": "Battery communication error.",
        }
        logger.debug(f"CID2 is {cid2}: {codes.get(cid2, None)}")
        return 0 if cid2 == b"00" else -1

    def decode_mfg_params(self, payload: bytearray) -> Mapping[str, str]:
        ret: Mapping[str, str] = {
            "pack_sn": self.decode_ascii(payload[6:36]),
            "product_id": self.decode_ascii(payload[36:66]),
            "bms_id": self.decode_ascii(payload[66:96]),
            "manufacturer": self.decode_ascii(payload[99:119]),
        }
        y, m, d = payload[96:99]
        ret["born_date"] = f"{y + 2000}-{m:02d}-{d:02d}"
        return ret

    def decode_cap_params(self, payload: bytearray) -> Mapping[str, float]:
        ret: Mapping[str, float] = {
            "remaining_cap_ah": self.div100(payload[6:8]),
            "full_cap_ah": self.div100(payload[8:10]),
            "design_cap_ah": self.div100(payload[10:12]),
            "total_charge_cap_ah": self.div100(payload[12:16]),
            "total_discharge_cap_ah": self.div100(payload[16:20]),
            "total_charge_kwh": self.div100(payload[20:22]),
            "total_discharge_kwh": self.div100(payload[22:24]),
        }
        return ret

    def decode_mfg_info(self, payload: bytearray) -> Mapping[str, str]:
        ret: Mapping[str, str] = {
            "hwtype": self.decode_ascii(payload[:10]),
            "product_code": self.decode_ascii(payload[10:20]),
            "project_code": self.decode_ascii(payload[20:30]),
        }
        major, minor, patch = payload[30:33]
        ret["software_version"] = f"{major:02d}.{minor:02d}.{patch:02d}"
        return ret

    def decode_system_params(self, payload: bytearray) -> Mapping[str, str | int | float]:
        ret: Mapping[str, str | int | float] = {
            "cell_v_upper_limit": self.div1000(payload[1:3]),
            "cell_v_lower_limit": self.div1000(payload[3:5]),
            "temp_upper_limit": self.decode_int(payload[5:7]),
            "temp_lower_limit": self.decode_int(payload[7:9]),
            "chg_c_upper_limit": self.div100(payload[9:11]),
            "tot_v_upper_limit": self.div1000(payload[11:13]),
            "tot_v_lower_limit": self.div1000(payload[13:15]),
            "number_of_cells": self.decode_int(payload[15:17]),
            "chg_c_limit": self.div100(payload[17:19]),
            "design_cap_none": self.div100(payload[19:21]),
            "history_storage_interval": self.decode_int(payload[21:23]),
            "balanced_mode": self.decode_int(payload[23:25]),
            "product_barcode": self.decode_ascii(payload[25:45]),
            "bms_barcode": self.decode_ascii(payload[45:65]),
        }
        return ret

    def decode_device_info(self, payload: bytearray) -> Mapping[str, int | float]:
        logger.debug(f"Got payload {payload.hex()}")
        cell_count: int = self.decode_int(payload[5:6])
        ret: Mapping[str, int | float] = {
            "soc": self.div100(payload[1:3]),
            "voltage": self.div100(payload[3:5]),
            "cell_count": cell_count,
        }
        off: int = 6
        for cell_no in range(cell_count):
            ret[f"cell_{cell_no + 1}_voltage"] = self.div1000(payload[off : off + 2])
            off += 2
        tot_temps = self.decode_int(payload[off + 6 : off + 7])
        ret.update(
            {
                "env_temp": self.decode_temp(payload[off : off + 2]),
                "pack_temp": self.decode_temp(payload[off + 2 : off + 4]),
                "mos_temp": self.decode_temp(payload[off + 4 : off + 6]),
                "tot_temps": tot_temps,
            }
        )
        off += 7
        for temp_no in range(tot_temps):
            ret[f"temp_{temp_no + 1}"] = self.decode_temp(payload[off : off + 2])
            off += 2

        ret.update(
            {
                "current": self.decode_current(payload[off : off + 2]),
                "internal_r": self.decode_int(payload[off + 2 : off + 4]) / 10,
                "soh": self.decode_int(payload[off + 4 : off + 6]),
                "user_custom": self.decode_int(payload[off + 6 : off + 7]),
                "charge_full_ah": self.div100(payload[off + 7 : off + 9]),
                "charge_now_ah": self.div100(payload[off + 9 : off + 11]),
                "cycle_count": self.decode_int(payload[off + 11 : off + 13]),
                "voltage_status": self.decode_int(payload[off + 13 : off + 15]),
                "current_status": self.decode_int(payload[off + 15 : off + 17]),
                "temp_status": self.decode_int(payload[off + 17 : off + 19]),
                "fet_status": self.decode_int(payload[off + 19 : off + 21]),
                "overvolt_prot_status_l": self.decode_int(payload[off + 21 : off + 23]),
                "undervolt_prot_status_l": self.decode_int(
                    payload[off + 23 : off + 25]
                ),
                "overvolt_alarm_status_l": self.decode_int(
                    payload[off + 25 : off + 27]
                ),
                "undervolt_alarm_status_l": self.decode_int(
                    payload[off + 27 : off + 29]
                ),
                "balance_state_l": self.decode_int(payload[off + 29 : off + 31]),
                "balance_state_h": self.decode_int(payload[off + 31 : off + 33]),
                "overvolt_prot_status_h": self.decode_int(payload[off + 33 : off + 35]),
                "undervolt_prot_status_h": self.decode_int(
                    payload[off + 35 : off + 37]
                ),
                "overvolt_alarm_status_h": self.decode_int(
                    payload[off + 37 : off + 39]
                ),
                "undervolt_alarm_status_h": self.decode_int(
                    payload[off + 39 : off + 41]
                ),
                "machine_status_list": self.decode_int(payload[off + 41 : off + 42]),
                "io_status_list": self.decode_int(payload[off + 42 : off + 44]),
            }
        )
        return ret

    def decode_ascii(self, data: bytearray) -> str:
        return data.decode().strip("\x00")

    def div1000(self, data: bytearray) -> float:
        return int(data.hex(), 16) / 1000

    def div100(self, data: bytearray) -> float:
        return int(data.hex(), 16) / 100

    def decode_int(self, data: bytearray) -> int:
        return int(data.hex(), 16)

    def decode_temp(self, data: bytearray) -> float:
        return int.from_bytes(bytes(data), signed=True) / 10

    def decode_current(self, data: bytearray) -> float:
        return int.from_bytes(bytes(data), signed=True) / 100


class DarenClient:
    def __init__(self, addr: str, port: str) -> None:
        self.proto: DarenProto = DarenProto(bytes.fromhex(addr))
        self.ser: serial.Serial = serial.Serial(port, 9600)

    @cache
    def get_mfg_params(self) -> Mapping[str, str | int | float]:
        return self.run_cmd(self.proto.get_mfg_params, self.proto.decode_mfg_params)

    @cache
    def get_cap_params(self) -> Mapping[str, str | int | float]:
        return self.run_cmd(self.proto.get_cap_params, self.proto.decode_cap_params)

    @cache
    def get_mfg_info(self) -> Mapping[str, str | int | float]:
        return self.run_cmd(self.proto.get_mfg_info, self.proto.decode_mfg_info)

    @cache
    def get_system_params(self) -> Mapping[str, str | int | float]:
        return self.run_cmd(
            self.proto.get_system_params, self.proto.decode_system_params
        )

    def get_device_info(self) -> Mapping[str, str | int | float]:
        return self.run_cmd(self.proto.get_device_info, self.proto.decode_device_info)

    def run_cmd(
        self,
        proto_cmd: Callable[[], str],
        proto_parser: Callable[[bytearray], Mapping[str, str | int | float]],
    ) -> Mapping[str, str | int | float]:
        cmd = proto_cmd()
        self.send_cmd(cmd)
        payload = bytearray.fromhex(self.read_response().decode())
        if len(payload) == 0:
            logger.error("Payload is empty!")
            return {}
        return proto_parser(payload)

    def send_cmd(self, cmd: str) -> None:
        ser = self.ser
        ser.reset_output_buffer()
        ser.reset_input_buffer()
        _ = ser.write(cmd.encode())
        logger.debug(f"Sent command {cmd}")

    def read_response(self) -> bytes:
        ser = self.ser
        buff = ser.read_until(b"\r")

        try:
            CID2 = buff[7:9]
            if self.proto.decode_cid2(CID2) == -1:
                logger.debug("CID2_Decode error!")
                logger.debug(f"Buffer contents: {buff}")
                return b""
        except Exception as e:
            logger.error(f"read_response Data invalid!: {e}")
            logger.error(f"Received data: {buff}")
            return b""

        logger.debug(f"Received data: {buff}")

        try:
            LENID = int(buff[9:13], base=16)
            length = LENID & 0x0FFF
            if self.proto.length_checksum(length) == LENID:
                logger.debug("Data length ok.")
            else:
                logger.error("Data length error.")
                return b""
        except Exception as e:
            logger.error("Exception during data length check: {}".format(e))
            logger.error("Received data: {}".format(buff))
            return b""

        try:
            chksum = int(buff[len(buff) - 5 :], base=16)
            calculated_chksum = self.proto.checksum(buff.decode()[1 : len(buff) - 5])
            if calculated_chksum == chksum:
                logger.debug("Checksum ok.")
            else:
                logger.error(
                    "Checksum error. Calculated: {}, Received: {}".format(
                        calculated_chksum, chksum
                    )
                )
                return b""

        except Exception as e:
            logger.error("Exception during checksum calculation: {}".format(e))
            return b""

        logger.debug("read_response Data valid!")

        return buff[13 : len(buff) - 5]  # return payload


def print_pack_info(client: DarenClient) -> None:
    mfg_info = client.get_mfg_params()
    cap_info = client.get_cap_params()

    print(f"BMS          : {mfg_info['pack_sn']}")
    print(f"Full charge  : {cap_info['full_cap_ah']} Ah")
    print(f"Design charge: {cap_info['design_cap_ah']}")


def print_rt_data(client: DarenClient, overwrite: bool = False):
    rt_data = client.get_device_info()

    if overwrite:
        print("\x1b[A\x1b[A\x1b[A\x1b[A\x1b[A\x1b[A", end="\r")

    print(datetime.now().strftime("Time           : %Y-%m-%d %H:%M:%S"))
    print(f"State of charge: {rt_data['soc']}%                  ")
    print(f"Capacity now   : {rt_data['charge_now_ah']} Ah      ")
    print(f"Charge current : {rt_data['current']} A             ")
    print(f"Voltage        : {rt_data['voltage']}               ")
    print(f"Temperature    : {rt_data['pack_temp']} C           ")


def pprint(title: str, data: Mapping[str,str|float|int]) -> None:
    print('---------------------------------------------')
    print(title)
    print('---------------------------------------------')
    print(json.dumps(data, indent=2))

if __name__ == "__main__":
    client = DarenClient("01", "/dev/ttyUSB0")

    if len(sys.argv) > 1 and sys.argv[1] == "-d":
        pprint("mfg params", client.get_mfg_params())
        pprint("cap params", client.get_cap_params())
        pprint("mfg info", client.get_mfg_info())
        pprint("system params", client.get_system_params())
        pprint("device info", client.get_device_info())
        exit(0)


    print_pack_info(client)
    print("\nRealtime data:\n")

    print_rt_data(client)
    time.sleep(60)

    while True:
        try:
            print_rt_data(client, overwrite=True)
            time.sleep(60)
        except KeyboardInterrupt:
            break

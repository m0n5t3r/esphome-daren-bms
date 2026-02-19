import serial
import logging
from functools import cache
from struct import unpack
from datetime import datetime
import time

logger = logging.getLogger('root')
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.INFO)


class DarenProto():
    SOI = '~'
    VER = '22'
    CID1 = '4A'

    def __init__(self, addr):
        self.addr = addr

    def get_system_params(self):
        return self.command('47', self.addr)

    def get_protocol_version(self):
        return self.command('4F')

    def get_device_info(self):
        return self.command('42', self.addr)

    def get_mfg_info(self):
        return self.command('51')

    def get_params(self, module):
        info_bits = [
            self.addr,
            '01',   # operation
            module, # module
            'FF',   # function id
            '00'    # function len
        ]

        info = ''.join(info_bits)
        return self.command('B0', info)

    def get_ocv_params(self):
        return self.get_params('01')

    def get_hwprot_params(self):
        return self.get_params('02')

    def get_mfg_params(self):
        return self.get_params('03')

    def get_cap_params(self):
        return self.get_params('04')

    def command(self, cid2, info=""):
        cmd_bits = [
            self.SOI, 
            self.VER, 
            self.addr, 
            self.CID1, 
            cid2
        ]

        if len(info) > 0:
            length_cs = self.length_checksum(len(info))
            cmd_bits.append(format(length_cs, 'x'))
            cmd_bits.append(info)
        else:
            cmd_bits.append('0000')

        cmd = ''.join(cmd_bits).upper()

        cksum = self.checksum(cmd[1:len(cmd)])
        cmd += format(cksum, 'x')
        cmd += '\r'

        return cmd.upper()

    def length_checksum(self, value):
        value = value & 0x0FFF
        n1 = value & 0xF
        n2 = (value >> 4) & 0xF
        n3 = (value >> 8) & 0xF
        cksum = ((n1 + n2 + n3) & 0xF) ^ 0xF
        cksum += 1
        return value + (cksum << 12)

    def checksum(self, s):
        cksum = 0
        for value in s:
            cksum += ord(value)
        cksum = cksum ^ 0xFFFF
        return cksum + 1

    def decode_cid2(self, cid2):
        codes = {
            "00": "CID2 response ok.",
            "01": "VER error.",
            "02": "CHKSUM error.",
            "03": "LCHKSUM error.",
            "04": "CID2 invalid.",
            "05": "Command format error.",
            "06": "INFO data invalid.",
            "90": "ADR error.",
            "91": "Battery communication error.",
        }
        logger.debug(codes.get(cid2))
        if cid2 == "00":
            return 0
        return -1

    def decode_mfg_params(self, payload):
        ret = {
            "pack_sn": payload[12:72],
            "product_id": payload[72:132],
            "bms_id": payload[132:192],
            "manufacturer": payload[198:238]
        }

        for k, v in ret.items():
            ret[k] = self.decode_ascii(v)

        y, m, d = bytearray.fromhex(payload[192:198])
        ret["born_date"] = f'{y + 2000}-{m:02d}-{d:02d}'

        return ret

    def decode_cap_params(self, payload):
        ret = {
            "remaining_cap_ah": payload[12:16],
            "full_cap_ah": payload[16:20],
            "design_cap_ah": payload[20:24],
            "total_charge_cap_ah": payload[24:32],
            "total_discharge_cap_ah": payload[32:40],
            "total_charge_kwh": payload[40:44],
            "total_discharge_kwh": payload[44:48],
        }

        for k, v in ret.items():
            ret[k] = self.div100(v)

        return ret

    def decode_mfg_info(self, payload):
        ret = {
            "hwtype": payload[:20],
            "product_code": payload[20:40],
            "project_code": payload[40:60],
        }

        for k, v in ret.items():
            ret[k] = self.decode_ascii(v)

        major, minor, patch = bytearray.fromhex(payload[60:66])

        ret["software_version"] = f"{major:02d}.{minor:02d}.{patch:02d}"
        return ret

    def decode_system_params(self, payload):
        ret = {}
        values = [
            ("cell_v_upper_limit", payload[2:6], self.div1000),
            ("cell_v_lower_limit", payload[6:10], self.div1000),
            ("temp_upper_limit", payload[10:14], self.decode_int),
            ("temp_lower_limit", payload[14:18], self.decode_int),
            ("chg_c_upper_limit", payload[18:22], self.div100),
            ("tot_v_upper_limit", payload[22:26], self.div1000),
            ("tot_v_lower_limit", payload[26:30], self.div1000),
            ("number_of_cells", payload[30:34], self.decode_int),
            ("chg_c_limit", payload[34:38], self.div100),
            ("design_cap_none", payload[38:42], self.div100),
            ("history_storage_interval", payload[42:46], self.decode_int),
            ("balanced_mode", payload[46:50], self.decode_int),
            ("product_barcode", payload[50:90], self.decode_ascii),
            ("bms_barcode", payload[90:130], self.decode_ascii),
        ]

        for k, v, modifier in values:
            ret[k] = modifier(v)

        return ret

    def decode_device_info(self, payload):
        ret = {
            "soc": self.div100(payload[2:6]),
            "voltage": self.div100(payload[6:10]),
            "cell_count": self.decode_int(payload[10:12]),
        }
        for cell_no in range(ret["cell_count"]):
            start_offset = cell_no * 4 + 12
            end_offset = start_offset + 4
            ret[f"cell_{cell_no + 1}_voltage"] = self.div1000(payload[start_offset:end_offset])

        off = end_offset

        ret.update({
            "env_temp": self.decode_temp(payload[off:off + 4]),
            "pack_temp": self.decode_temp(payload[off + 4:off + 8]),
            "mos_temp": self.decode_temp(payload[off + 8:off + 12]),
            "tot_temps": self.decode_int(payload[off + 12:off + 14]),
        })

        off += 14
        for temp_no in range(ret["tot_temps"]):
            start_offset = temp_no * 4 + off
            end_offset = start_offset + 4
            ret[f"temp_{temp_no + 1}"] = self.decode_temp(payload[start_offset:end_offset])

        off += ret["tot_temps"] * 4

        ret.update({
            "current": self.decode_current(payload[off:off + 4]),
            "internal_r": self.decode_int(payload[off + 4:off + 8]) / 10,
            "soh": self.decode_int(payload[off + 8:off + 12]),
            "user_custom": self.decode_int(payload[off + 12:off + 14]),
            "charge_full_ah": self.div100(payload[off + 14:off + 18]),
            "charge_now_ah": self.div100(payload[off + 18:off + 22]),
            "cycle_count": self.decode_int(payload[off + 22:off + 26]),
            "voltage_status": self.decode_int(payload[off + 26:off + 30]),
            "current_status": self.decode_int(payload[off + 30:off + 34]),
            "temp_status": self.decode_int(payload[off + 34:off + 38]),
            "fet_status": self.decode_int(payload[off + 38:off + 42]),
            "overvolt_prot_status_l": self.decode_int(payload[off + 42:off + 46]),
            "undervolt_prot_status_l": self.decode_int(payload[off + 46:off + 50]),
            "overvolt_alarm_status_l": self.decode_int(payload[off + 50:off + 54]),
            "undervolt_alarm_status_l": self.decode_int(payload[off + 54:off + 58]),
            "balance_state_l": self.decode_int(payload[off + 58:off + 62]),
            "balance_state_h": self.decode_int(payload[off + 62:off + 66]),
            "overvolt_prot_status_h": self.decode_int(payload[off + 66:off + 70]),
            "undervolt_prot_status_h": self.decode_int(payload[off + 70:off + 74]),
            "overvolt_alarm_status_h": self.decode_int(payload[off + 74:off + 78]),
            "undervolt_alarm_status_h": self.decode_int(payload[off + 78:off + 82]),
            "machine_status_list": self.decode_int(payload[off + 82:off + 84]),
            "io_status_list": self.decode_int(payload[off + 84:off + 88]),
        })


        return ret

    def decode_ascii(self, data):
        return bytearray.fromhex(data).decode().strip('\x00')

    def div1000(self, data):
        return int(data, 16) / 1000

    def div100(self, data):
        return int(data, 16) / 100

    def decode_int(self, data):
        return int(data, 16)

    def decode_temp(self, data):
        return unpack('>h', bytes.fromhex(data))[0] / 10

    def decode_current(self, data):
        return unpack('>h', bytes.fromhex(data))[0] / 100


class DarenClient():
    def __init__(self, addr, port):
        self.proto = DarenProto(addr)
        self.ser = serial.Serial(port, 9600)

    @cache
    def get_mfg_params(self):
        return self.run_cmd(self.proto.get_mfg_params, self.proto.decode_mfg_params)

    @cache
    def get_cap_params(self):
        return self.run_cmd(self.proto.get_cap_params, self.proto.decode_cap_params)

    @cache
    def get_mfg_info(self):
        return self.run_cmd(self.proto.get_mfg_info, self.proto.decode_mfg_info)

    @cache
    def get_system_params(self):
        return self.run_cmd(self.proto.get_system_params, self.proto.decode_system_params)

    def get_device_info(self):
        return self.run_cmd(self.proto.get_device_info, self.proto.decode_device_info)

    def run_cmd(self, proto_cmd, proto_parser):
        cmd = proto_cmd()
        self.send_cmd(cmd)
        payload = self.read_response()
        return proto_parser(payload)

    def send_cmd(self, cmd):
        ser = self.ser
        ser.flushOutput()
        ser.flushInput()
        ser.write(cmd.encode())

    def read_response(self):
        ser = self.ser
        buff = ser.read_until(b'\r').decode()

        try:
            CID2 = buff[7:9]
            if self.proto.decode_cid2(CID2) == -1:
                logger.debug("CID2_Decode error!")
                logger.debug("Buffer contents: {}".format(buff))
                return False
        except Exception as e:
            logger.error("read_response Data invalid!: {}".format(e))
            logger.error("Received data: {}".format(buff))
            return False

        logger.debug("Received data: {}".format(buff))

        try:
            LENID = int(buff[9:13], base=16)
            length = LENID & 0x0FFF
            if self.proto.length_checksum(length) == LENID:
                logger.debug("Data length ok.")
            else:
                logger.error("Data length error.")
                return False
        except Exception as e:
            logger.error("Exception during data length check: {}".format(e))
            logger.error("Received data: {}".format(buff))
            return False

        try:
            chksum = int(buff[len(buff) - 5 :], base=16)
            calculated_chksum = self.proto.checksum(buff[1 : len(buff) - 5])
            if calculated_chksum == chksum:
                logger.debug("Checksum ok.")
            else:
                logger.error(
                    "Checksum error. Calculated: {}, Received: {}".format(
                        calculated_chksum, chksum
                    )
                )
                return False

        except Exception as e:
            logger.error("Exception during checksum calculation: {}".format(e))
            return False

        logger.debug("read_response Data valid!")

        return buff[13:len(buff) - 4] # return payload



def print_pack_info(client):
    mfg_info = client.get_mfg_params()
    cap_info = client.get_cap_params()

    print(f"BMS           : {mfg_info['pack_sn']}")
    print(f"Full charge   : {cap_info['full_cap_ah']} Ah")
    print(f"Current charge: {cap_info['remaining_cap_ah']}")

def print_rt_data(client, overwrite=False):
    rt_data = client.get_device_info()

    if overwrite:
        print('\x1b[A\x1b[A\x1b[A\x1b[A\x1b[A\x1b[A', end='\r')

    print(datetime.now().strftime("Time           : %Y-%m-%d %H:%M:%S"))
    print(f"State of charge: {rt_data['soc']}%                  ")
    print(f"Capacity now   : {rt_data['charge_now_ah']} Ah      ")
    print(f"Charge current : {rt_data['current']} A             ")
    print(f"Voltage        : {rt_data['voltage']}               ")
    print(f"Temperature    : {rt_data['pack_temp']} C           ")


if __name__ == '__main__':
    client = DarenClient('01', '/dev/ttyUSB0')

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

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_ADDRESS

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor"]
MULTI_CONF = True

daren_bms_ns = cg.esphome_ns.namespace("daren_bms")
DarenBMS = daren_bms_ns.class_("DarenBMS", cg.Component, uart.UARTDevice)

CONF_DAREN_BMS_ID = "daren_bms_id"
DAREN_BMS_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(DarenBMS),
        cv.Optional(CONF_ADDRESS): cv.int_range(0, 255),
    }
)

CONFIG_SCHEMA = DAREN_BMS_COMPONENT_SCHEMA.extend(
    cv.polling_component_schema("30s")
).extend(uart.UART_DEVICE_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add_define("BUF_MAX_SIZE", 130)
    cg.add(var.set_bms_id(config[CONF_ADDRESS]))
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

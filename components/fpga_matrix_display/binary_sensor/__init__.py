# SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
# SPDX-License-Identifier: MIT
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_TYPE

from ..display import MATRIX_ID, MatrixDisplay

matrix_display_status_ns = cg.esphome_ns.namespace(
    "matrix_display::matrix_display_status"
)
MatrixDisplayStatusFlag = matrix_display_status_ns.class_(
    "MatrixDisplayStatusFlag", binary_sensor.BinarySensor, cg.PollingComponent
)
StatusFlagType = matrix_display_status_ns.enum("StatusFlagType", is_class=True)

# FLAGS register bits, per the FPGA's display_core.sv:
# value[2:0] = {fpga_ready, ctrl_busy, ctrl_ready_for_data}
FLAG_TYPES = {
    "fpga_ready": StatusFlagType.FPGA_READY,
    "ctrl_busy": StatusFlagType.CTRL_BUSY,
    "ctrl_ready_for_data": StatusFlagType.CTRL_READY_FOR_DATA,
}

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(MatrixDisplayStatusFlag)
    .extend(
        {
            cv.Required(MATRIX_ID): cv.use_id(MatrixDisplay),
            cv.Required(CONF_TYPE): cv.enum(FLAG_TYPES, lower=True),
        }
    )
    .extend(cv.polling_component_schema("10s"))
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)

    matrix = await cg.get_variable(config[MATRIX_ID])
    cg.add(var.set_display(matrix))
    cg.add(var.set_flag_type(config[CONF_TYPE]))

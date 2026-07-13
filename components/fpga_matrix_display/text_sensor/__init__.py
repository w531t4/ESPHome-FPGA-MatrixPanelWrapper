# SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
# SPDX-License-Identifier: MIT
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor

from ..display import MATRIX_ID, MatrixDisplay

matrix_display_version_ns = cg.esphome_ns.namespace(
    "matrix_display::matrix_display_version"
)
MatrixDisplayVersion = matrix_display_version_ns.class_(
    "MatrixDisplayVersion", text_sensor.TextSensor, cg.PollingComponent
)

# Publishes the decoded gateware version string read from the FPGA's
# STATUS_ADDR_VERSION register (see reg_version.sv). The version is effectively
# static per boot but can change on a remote reflash, so it is polled slowly.
CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema(MatrixDisplayVersion)
    .extend(
        {
            cv.Required(MATRIX_ID): cv.use_id(MatrixDisplay),
        }
    )
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)

    matrix = await cg.get_variable(config[MATRIX_ID])
    cg.add(var.set_display(matrix))

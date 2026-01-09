# SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
# SPDX-License-Identifier: MIT
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID

from ..display import MATRIX_ID, MatrixDisplay

matrix_display_ns = cg.esphome_ns.namespace("matrix_display")
MatrixDisplayLightOutput = matrix_display_ns.class_(
    "MatrixDisplayLightOutput", light.LightOutput
)

CONFIG_SCHEMA = light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(MatrixDisplayLightOutput),
        cv.Required(MATRIX_ID): cv.use_id(MatrixDisplay),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)

    matrix = await cg.get_variable(config[MATRIX_ID])
    cg.add(var.set_display(matrix))

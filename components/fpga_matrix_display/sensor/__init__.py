# SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
# SPDX-License-Identifier: MIT
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    ICON_TIMER,
    STATE_CLASS_MEASUREMENT,
)

from ..display import MATRIX_ID, MatrixDisplay

matrix_display_update_duration_ns = cg.esphome_ns.namespace(
    "matrix_display::matrix_display_update_duration"
)
MatrixDisplayUpdateDuration = matrix_display_update_duration_ns.class_(
    "MatrixDisplayUpdateDuration", sensor.Sensor, cg.PollingComponent
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        MatrixDisplayUpdateDuration,
        unit_of_measurement="µs",
        icon=ICON_TIMER,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.Required(MATRIX_ID): cv.use_id(MatrixDisplay),
        }
    )
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    matrix = await cg.get_variable(config[MATRIX_ID])
    cg.add(var.set_display(matrix))

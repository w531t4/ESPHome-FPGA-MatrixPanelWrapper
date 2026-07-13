# SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
# SPDX-License-Identifier: MIT
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_TYPE,
    DEVICE_CLASS_DATA_RATE,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_FREQUENCY,
    ICON_TIMER,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
)

from ..display import MATRIX_ID, MatrixDisplay

matrix_display_update_duration_ns = cg.esphome_ns.namespace(
    "matrix_display::matrix_display_update_duration"
)
MatrixDisplayUpdateDuration = matrix_display_update_duration_ns.class_(
    "MatrixDisplayUpdateDuration", sensor.Sensor, cg.PollingComponent
)

matrix_display_status_value_ns = cg.esphome_ns.namespace(
    "matrix_display::matrix_display_status_value"
)
MatrixDisplayStatusValue = matrix_display_status_value_ns.class_(
    "MatrixDisplayStatusValue", sensor.Sensor, cg.PollingComponent
)

# Status register addresses come from the C++ header (MatrixPanel_FPGA_SPI
# STATUS_ADDR_* constants) so the address values live in one place.
MatrixPanelConstants = cg.global_ns.namespace("MatrixPanel_FPGA_SPI")
STATUS_VALUE_ADDRS = {
    "rx_kbps": MatrixPanelConstants.STATUS_ADDR_RX_KBPS,
    "hub75_fps": MatrixPanelConstants.STATUS_ADDR_HUB75_FPS,
    "fb_fps": MatrixPanelConstants.STATUS_ADDR_FB_FPS,
    "uptime": MatrixPanelConstants.STATUS_ADDR_UPTIME,
}

MATRIX_SCHEMA = {
    cv.Required(MATRIX_ID): cv.use_id(MatrixDisplay),
}


def _status_value_schema(default_update_interval, **sensor_kwargs):
    """Schema for a sensor publishing one numeric status register."""
    return (
        sensor.sensor_schema(
            MatrixDisplayStatusValue, accuracy_decimals=0, **sensor_kwargs
        )
        .extend(MATRIX_SCHEMA)
        .extend(cv.polling_component_schema(default_update_interval))
    )


CONFIG_SCHEMA = cv.typed_schema(
    {
        "update_duration": sensor.sensor_schema(
            MatrixDisplayUpdateDuration,
            unit_of_measurement="µs",
            icon=ICON_TIMER,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        )
        .extend(MATRIX_SCHEMA)
        .extend(cv.polling_component_schema("60s")),
        # Rolling measurement of kilobytes/s received by the FPGA from the
        # ESP32 (STATUS_ADDR_RX_KBPS).
        "rx_kbps": _status_value_schema(
            "10s",
            unit_of_measurement="kB/s",
            device_class=DEVICE_CLASS_DATA_RATE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        # HUB75 frame-emit rate, 5 s sliding average (STATUS_ADDR_HUB75_FPS).
        "hub75_fps": _status_value_schema(
            "10s",
            unit_of_measurement="Hz",
            device_class=DEVICE_CLASS_FREQUENCY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        # Framebuffer swap rate, 5 s average; 0 unless double-buffered
        # (STATUS_ADDR_FB_FPS).
        "fb_fps": _status_value_schema(
            "10s",
            unit_of_measurement="Hz",
            device_class=DEVICE_CLASS_FREQUENCY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        # Whole seconds since FPGA reset (STATUS_ADDR_UPTIME).
        "uptime": _status_value_schema(
            "60s",
            unit_of_measurement="s",
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
    },
    default_type="update_duration",
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    matrix = await cg.get_variable(config[MATRIX_ID])
    cg.add(var.set_display(matrix))

    if config[CONF_TYPE] in STATUS_VALUE_ADDRS:
        cg.add(var.set_address(STATUS_VALUE_ADDRS[config[CONF_TYPE]]))

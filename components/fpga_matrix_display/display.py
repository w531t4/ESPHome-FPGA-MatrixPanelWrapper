# SPDX-FileCopyrightText: 2019 ESPHome
# SPDX-FileCopyrightText: 2025 Aaron White <w531t4@gmail.com>
# SPDX-License-Identifier: MIT
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import display
from esphome.const import (
    CONF_HEIGHT,
    CONF_ID,
    CONF_LAMBDA,
    CONF_UPDATE_INTERVAL,
    CONF_WIDTH,
)

DEPENDENCIES = ["esp32"]

MATRIX_ID = "matrix_id"
CHAIN_LENGTH = "chain_length"
BRIGHTNESS = "brightness"

SPI_CE_PIN =  "SPI_CE_pin"
SPI_CLK_PIN = "SPI_CLK_pin"
SPI_MOSI_PIN = "SPI_MOSI_pin"

FPGA_RESETSTATUS_PIN = "FPGA_RESETSTATUS_pin"

SPISPEED = "spispeed"

USE_CUSTOM_LIBRARY = "use_custom_library"
USE_WATCHDOG = "use_watchdog"
WATCHDOG_INTERVAL_USEC = "watchdog_interval_usec"

matrix_display_ns = cg.esphome_ns.namespace("matrix_display")
MatrixDisplay = matrix_display_ns.class_(
    "MatrixDisplay", cg.PollingComponent, display.DisplayBuffer
)

clk_speed = cg.global_ns.namespace("FPGA_SPI_CFG").enum("clk_speed")
CLOCK_SPEEDS = {
    "HZ_8M": clk_speed.HZ_8M,
    "HZ_10M": clk_speed.HZ_10M,
    "HZ_15M": clk_speed.HZ_15M,
    "HZ_16M": clk_speed.HZ_16M,
    "HZ_20M": clk_speed.HZ_20M,
    "HZ_26M": clk_speed.HZ_26M,
    "HZ_40M": clk_speed.HZ_40M,
    "HZ_80M": clk_speed.HZ_80M,
}

CONFIG_SCHEMA = display.FULL_DISPLAY_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(MatrixDisplay),
        cv.Required(CONF_WIDTH): cv.positive_int,
        cv.Required(CONF_HEIGHT): cv.positive_int,
        cv.Optional(USE_CUSTOM_LIBRARY, default=False): cv.boolean,
        cv.Optional(CHAIN_LENGTH, default=1): cv.positive_int,
        cv.Optional(BRIGHTNESS, default=128): cv.int_range(min=0, max=255),
        cv.Optional(
            CONF_UPDATE_INTERVAL, default="16ms"
        ): cv.positive_time_period_milliseconds,
        cv.Optional(SPI_CE_PIN, default=15): pins.gpio_output_pin_schema,
        cv.Optional(SPI_CLK_PIN, default=14): pins.gpio_output_pin_schema,
        cv.Optional(SPI_MOSI_PIN, default=2): pins.gpio_output_pin_schema,
        cv.Optional(FPGA_RESETSTATUS_PIN, default=27): pins.gpio_input_pin_schema,
        cv.Optional(SPISPEED): cv.enum(CLOCK_SPEEDS, upper=True, space="_"),
        cv.Optional(USE_WATCHDOG, default=True): cv.boolean,
        cv.Optional(WATCHDOG_INTERVAL_USEC, default=1000000): cv.positive_int,
    }
)


async def to_code(config):
    if not config[USE_CUSTOM_LIBRARY]:
        cg.add_library(
            "https://github.com/w531t4/ESP32-FPGA-MatrixPanel#v2.0.0",
            None,
        )

    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_panel_width(config[CONF_WIDTH]))
    cg.add(var.set_panel_height(config[CONF_HEIGHT]))
    cg.add(var.set_chain_length(config[CHAIN_LENGTH]))
    cg.add(var.set_initial_brightness(config[BRIGHTNESS]))

    SPI_CE_pin   = await cg.gpio_pin_expression(config[SPI_CE_PIN])
    SPI_CLK_pin  = await cg.gpio_pin_expression(config[SPI_CLK_PIN])
    SPI_MOSI_pin = await cg.gpio_pin_expression(config[SPI_MOSI_PIN])
    FPGA_RESETSTATUS_pin = await cg.gpio_pin_expression(config[FPGA_RESETSTATUS_PIN])

    cg.add(
        var.set_pins(
            SPI_CE_pin,
            SPI_CLK_pin,
            SPI_MOSI_pin,
            FPGA_RESETSTATUS_pin,
        )
    )
    cg.add(var.set_initial_watchdog(config[USE_WATCHDOG]))
    cg.add(var.set_initial_watchdog_interval_usec(config[WATCHDOG_INTERVAL_USEC]))

    if SPISPEED in config:
        cg.add(var.set_spispeed(config[SPISPEED]))

    await display.register_display(var, config)

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))

<!--
SPDX-FileCopyrightText: 2019 ESPHome
SPDX-FileCopyrightText: 2025 Aaron White <w531t4@gmail.com>
SPDX-License-Identifier: MIT
-->
# Modelling this project in the spirit of
https://github.com/TillFleisch/ESPHome-HUB75-MatrixDisplayWrapper
... but for my FPGA project.

# ESP32-FPGA-MatrixPanel ESPHome wrapper [![CI](https://github.com/w531t4/ESPHome-FPGA-MatrixDisplayWrapper/actions/workflows/ci.yaml/badge.svg?branch=main)](https://github.com/w531t4/ESPHome-FPGA-MatrixDisplayWrapper/actions/workflows/ci.yaml)

This custom component is a [ESPHome](https://esphome.io/) wrapper for the [ESP32-FPGA-MatrixPanel](https://github.com/w531t4/ESP32-FPGA-MatrixPanel#main) library. For more details regarding wiring, choosing the correct parameters and more complex configurations please refer to the above linked documentation.
This ESPHome component wraps the library into an ESPHome [display component](https://esphome.io/components/display/index.html) which can be used to show text, sensor values and images.

This wrapper currently only supports horizontally chained panels.

# Configuration variables

The custom component can be added to a yaml configuration by adding the external component like this:

```yaml
esphome:
  name: matrix-display
  friendly_name: Matrix Display

external_components:
  - source: github://w531t4/ESPHome-FPGA-MatrixDisplayWrapper@main

esp32:
  board: esp32dev
  framework:
    type: arduino
```

Note that the component will only compile on ESP32-based devices.
An example configuration can be found [here](example.yaml).

## Matrix Display

A minimum working example for setting up the display. A more complex configuration can be found [here](complex_matrix_config.yaml).

```yaml
display:
  - platform: fpga_matrix_display
    id: matrix
    width: 64
    height: 32
```


- **id**(**Required**, string): Matrix ID which will be used for entity configuration.
- **width**(**Required**, int): Width of the individual panels.
- **height**(**Required**, int): Height of the individual panels.
- **chain_length**(**Optional**, int): The number of panels chained one after another. Defaults to `1`.
- **brightness**(**Optional**, int): Initial brightness of the display (0-255). Defaults to `128`.

- **SPI_CE_PIN**(**Optional**, [Pin](https://esphome.io/guides/configuration-types.html#config-pin)): Pin connected to the S_CE pin on the FPGA. Defaults to `15`.
- **SPI_CLK_PIN**(**Optional**, [Pin](https://esphome.io/guides/configuration-types.html#config-pin)): Pin connected to the SCLK pin on the FPGA. Defaults to `14`.
- **SPI_MOSI_PIN**(**Optional**, [Pin](https://esphome.io/guides/configuration-types.html#config-pin)): Pin connected to the MOSI pin on the FPGA. Defaults to `2`.
- **FPGA_RESETSTATUS_PIN**(**Optional**, [Pin](https://esphome.io/guides/configuration-types.html#config-pin)): Input pin connected to the FPGA resetstatus indicator. When provided, the driver uses it to detect FPGA resets (falling edge or low-at-boot) and resync the display state. Defaults to `27`.
- **FPGA_BUSY_PIN**(**Optional**, [Pin](https://esphome.io/guides/configuration-types.html#config-pin)): Input pin connected to the FPGA BUSY line (wifi_gpio35). When set, the driver waits for BUSY to go low after each command before sending the next. No internal pull-up is enabled. Defaults to `35`.
- **STATUS_SPI_SCK_PIN**(**Optional**, [Pin](https://esphome.io/guides/configuration-types.html#config-pin)): Output pin driving the clock of the status readback SPI bus (FPGA `reg_spi_responder`).
- **STATUS_SPI_CS_PIN**(**Optional**, [Pin](https://esphome.io/guides/configuration-types.html#config-pin)): Output pin driving the chip select of the status readback SPI bus.
- **STATUS_SPI_MISO_PIN**(**Optional**, [Pin](https://esphome.io/guides/configuration-types.html#config-pin)): Input pin reading data from the status readback SPI bus.

The three `STATUS_SPI_*` pins must be given together or not at all; omitting them disables status readback entirely. The FPGA bitstream must be built with `USE_STATUS_SPI` for the responder to exist on the other end. `dump_config` reports `Status SPI ready: YES/NO`, and failed status reads are logged at DEBUG level with the failure reason and the raw frame bytes.

- **spispeed**(**Optional**): I2SSpeed used for configuring the display. Select one of `HZ_8M`, `HZ_10M`, `HZ_15M`, `HZ_16M`,`HZ_20M`.
- **use_custom_library**(**Optional**, boolean): If set to `true` a custom library must be defined using `platformio_options:lib_deps`. Defaults to `false`. See [this example](custom_library.yaml) for more details.

- All other options from [Display](https://esphome.io/components/display/index.html)

### Test Graphic Mode

A helper test graphic emits FPGA commands (clear, fill, rect, pixels, brightness, swap) so you can verify the command path before resuming normal rendering. Call `enter_test_state()` to show the graphic and `exit_test_state()` to return to the usual layout.

```yaml
on_boot:
  then:
    - lambda: |-
        id(matrix).enter_test_state();
```

Trigger the logic from automations or scripts; the display stays in the test state until you call `exit_test_state()`.

Note that the default pin configurations are the ones mentioned in the [ESP32-FPGA-MatrixPanel](https://github.com/w531t4/ESP32-FPGA-MatrixPanel) library. Some of these pins are used as strapping pins on ESPs. It is recommended to not use these.

## Switch

This switch can be used to turn the display on or off. In it's off state the display is showing a blank screen.

- **matrix_id**(**Required**, string): The matrix display entity to which this power switch belongs.
- All other options from [Switch](https://esphome.io/components/switch/index.html#config-switch)

## Brightness

This number entity can be used to set the display brightness. In combination with a brightness sensor this can used to adaptively change matrix displays brightness.

- **matrix_id**(**Required**, string): The matrix display entity to which this brightness value belongs.
- All other options from [Number](https://esphome.io/components/number/index.html#config-number)

## Sensor

One platform with a `type:` selector. `update_duration` is measured on the ESP32; all other types poll a numeric FPGA status register over the status readback SPI at their `update_interval` (requires the `STATUS_SPI_*` pins; when a read fails, the sensor keeps its last state).

```yaml
sensor:
  - platform: fpga_matrix_display
    matrix_id: matrix
    type: rx_kbps
    name: "FPGA RX Rate"
    update_interval: 10s
```

- **matrix_id**(**Required**, string): The matrix display entity this sensor reads from.
- **type**(**Optional**, string): What to report. Defaults to `update_duration`.
  - `update_duration`: moving average duration of display updates, µs (default interval `60s`).
  - `rx_kbps`: rolling kilobytes/s received by the FPGA from the ESP32 (`10s`).
  - `hub75_fps`: HUB75 frame-emit rate in Hz, 5 s sliding average (`10s`).
  - `fb_fps`: framebuffer swap rate in Hz, 5 s average; reads 0 unless the FPGA is built with double buffering (`10s`).
  - `uptime`: whole seconds since the FPGA came out of reset (`60s`).
- All other options from [Sensor](https://esphome.io/components/sensor/index.html#config-sensor), including `update_interval`.

## Status Binary Sensor

Polls one FPGA status flag over the status readback SPI at its `update_interval` and publishes it. Requires the `STATUS_SPI_*` pins to be configured on the display; when a read fails, the sensor keeps its last state.

```yaml
binary_sensor:
  - platform: fpga_matrix_display
    matrix_id: matrix
    name: "FPGA Ready"
    type: fpga_ready
    update_interval: 10s
```

- **matrix_id**(**Required**, string): The matrix display entity this sensor reads from.
- **type**(**Required**, string): Which flag to publish, one of `fpga_ready`, `ctrl_busy`, `ctrl_ready_for_data` (the FLAGS status register bits).
- All other options from [Binary Sensor](https://esphome.io/components/binary_sensor/index.html#config-binary-sensor), including `update_interval` (defaults to `10s`).

## Version Text Sensor

Polls the FPGA gateware version register (`STATUS_ADDR_VERSION`) over the status readback SPI and publishes the decoded string, git-describe style. An exact, clean tagged build shows just `vmajor.minor.patch` (e.g. `v1.2.3`); anything else appends `+commits.shaGITSHA` and a `-dirty` suffix when the gateware was built from a dirty working tree (e.g. `v1.2.3+45.sha1a2b3c4d-dirty`). Requires the `STATUS_SPI_*` pins; when a read fails, the sensor keeps its last state. Decoding/formatting is done in the panel library (`readVersion`/`formatVersion`).

```yaml
text_sensor:
  - platform: fpga_matrix_display
    matrix_id: matrix
    name: "FPGA Gateware Version"
```

- **matrix_id**(**Required**, string): The matrix display entity this sensor reads from.
- All other options from [Text Sensor](https://esphome.io/components/text_sensor/index.html#config-text-sensor), including `update_interval` (defaults to `60s`; the version is static per boot but can change on a remote reflash).

# writing esphome image
`esptool --baud 1152000 write_flash 0x0000 .esphome/build/blah/.pioenvs/blah/firmware.factory.bin`

# Related work

- Library used in this Project: [ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA) by [@mrcodetastic](https://github.com/mrcodetastic)
- Library used in this Project: [ESPHome-HUB75-MatrixDisplayWrapper](https://github.com/TillFleisch/ESPHome-HUB75-MatrixDisplayWrapper) by [@TillFleisch](https://github.com/TillFleisch)

# clangd/vscode

## Updating compile steps
  - `cd .esphome/build/<project>`
  - `pio run -t compiledb`

## Add to user vscode settings
  ```
      "clangd.arguments": [
          "--compile-commands-dir=${workspaceFolder}/.esphome/build/<project>",
          "--background-index",
          "--query-driver=**/xtensa-esp32-elf-g++",
      ],
  ```
## Add to ${FULLPATH}/.clangd
  ```
  CompileFlags:
    Remove:
      - -fno-tree-switch-conversion
      - -fstrict-volatile-bitfields
      - -mlongcalls
    Add:
      - -Qunused-arguments
      - -I${FULLPATH}$/config/.esphome/build/<project>/src
  ```

# Getting Started
[![Open in Dev Container](https://img.shields.io/badge/Open-Dev%20Container-blue?logo=visualstudiocode)](
https://vscode.dev/redirect?url=vscode://ms-vscode-remote.remote-containers/cloneInVolume?url=https://github.com/w531t4/ESPHome-FPGA-MatrixPanelWrapper
)
[![Open in Dev Container (SSH)](https://img.shields.io/badge/Open-Dev%20Container%20SSH-blue?logo=visualstudiocode)](
https://vscode.dev/redirect?url=vscode://ms-vscode-remote.remote-containers/cloneInVolume?url=ssh%3A%2F%2Fgit%40github.com%2Fw531t4%2FESPHome-FPGA-MatrixPanelWrapper.git
)

# Leveraging a private ESPHome yaml library
1. mkdir -p ~/.config/environment.d
1. edit ~/.config/environment.d/esphome.conf
  - add `ESPHOME_HOME_GIT_URL=ssh://git@github.com/YourOrg/YourPrivateRepo.git`
1. reboot/login-out
1. click link from above section to launch vscode container
# Acknowledgements

This project is based on and incorporates code from:

- **ESPHome-HUB75-MatrixDisplayWrapper** by TillFleisch
  - https://github.com/TillFleisch/ESPHome-HUB75-MatrixDisplayWrapper
  - License: Portions are GPL-3.0-or-later, Portions are MIT

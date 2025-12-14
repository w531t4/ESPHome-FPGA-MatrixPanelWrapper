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

SPI_CE_PIN =  "SPI_CE_pin"
SPI_CLK_PIN = "SPI_CLK_pin"
SPI_MOSI_PIN = "SPI_MOSI_pin"
- **SPI_CE_PIN**(**Optional**, [Pin](https://esphome.io/guides/configuration-types.html#config-pin)): Pin connected to the S_CE pin on the FPGA. Defaults to `15`.
- **SPI_CLK_PIN**(**Optional**, [Pin](https://esphome.io/guides/configuration-types.html#config-pin)): Pin connected to the SCLK pin on the FPGA. Defaults to `14`.
- **SPI_MOSI_PIN**(**Optional**, [Pin](https://esphome.io/guides/configuration-types.html#config-pin)): Pin connected to the MOSI pin on the FPGA. Defaults to `2`.

- **spispeed**(**Optional**): I2SSpeed used for configuring the display. Select one of `HZ_8M`, `HZ_10M`, `HZ_15M`, `HZ_16M`,`HZ_20M`.
- **use_custom_library**(**Optional**, boolean): If set to `true` a custom library must be defined using `platformio_options:lib_deps`. Defaults to `false`. See [this example](custom_library.yaml) for more details.

- All other options from [Display](https://esphome.io/components/display/index.html)

Note that the default pin configurations are the ones mentioned in the [ESP32-FPGA-MatrixPanel](https://github.com/w531t4/ESP32-FPGA-MatrixPanel) library. Some of these pins are used as strapping pins on ESPs. It is recommended to not use these.

## Switch

This switch can be used to turn the display on or off. In it's off state the display is showing a blank screen.

- **matrix_id**(**Required**, string): The matrix display entity to which this power switch belongs.
- All other options from [Switch](https://esphome.io/components/switch/index.html#config-switch)

## Brightness

This number entity can be used to set the display brightness. In combination with a brightness sensor this can used to adaptively change matrix displays brightness.

- **matrix_id**(**Required**, string): The matrix display entity to which this brightness value belongs.
- All other options from [Number](https://esphome.io/components/number/index.html#config-number)

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
          "--path-mappings=${FULLPATH}$.esphome/build/<project>/src=${FULLPATH}$/config/include"
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

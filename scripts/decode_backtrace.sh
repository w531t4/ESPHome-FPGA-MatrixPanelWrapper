#!/bin/bash
# SPDX-FileCopyrightText: 2025 Aaron White <w531t4@gmail.com>
# SPDX-License-Identifier: MIT
ADDR2LINE=/home/vscode/.platformio/tools/toolchain-xtensa-esp-elf/bin/xtensa-esp32-elf-addr2line
FIRMWARE=/workspace/ESPHome-FPGA-MatrixPanelWrapper/config/.esphome/build/ulx3s-b0b21c636b54/.pioenvs/ulx3s-b0b21c636b54/firmware.elf

# to get backtrace, run fpgarpi /workspace/ESP32-FPGA-MatrixPanel/scripts/troubleshoot.sh and cat log.txt
${ADDR2LINE} -pfiaC -e ${FIRMWARE} ${@}
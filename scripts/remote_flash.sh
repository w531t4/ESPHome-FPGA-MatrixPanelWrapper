#!/bin/bash
# SPDX-FileCopyrightText: 2025 Aaron White <w531t4@gmail.com>
# SPDX-License-Identifier: MIT
python3 -m esphome compile ../config/ulx3s-636b54.yaml

scp /workspace/ESPHome-FPGA-MatrixPanelWrapper/config/.esphome/build/ulx3s-b0b21c636b54/.pioenvs/ulx3s-b0b21c636b54/firmware.factory.bin fpgarpi:/tmp/firmware.bin

ssh fpgarpi "sh /home/pi/code/ESP32-FPGA-MatrixPanel/scripts/flash.sh"

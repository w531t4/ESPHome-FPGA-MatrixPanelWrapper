// SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only
#include "matrix_display_status_flag.h"

namespace esphome::matrix_display::matrix_display_status {

static const char *const TAG = "matrix_display.status_flag";

void MatrixDisplayStatusFlag::update() {
    if (this->display_ == nullptr)
        return;
    MatrixPanel_FPGA_SPI::FpgaStatusFlags flags;
    if (!this->display_->read_status_flags(flags)) {
        ESP_LOGD(TAG, "status flags read failed; keeping last state");
        return;
    }
    switch (this->flag_type_) {
    case StatusFlagType::FPGA_READY:
        this->publish_state(flags.fpga_ready);
        break;
    case StatusFlagType::CTRL_BUSY:
        this->publish_state(flags.ctrl_busy);
        break;
    case StatusFlagType::CTRL_READY_FOR_DATA:
        this->publish_state(flags.ctrl_ready_for_data);
        break;
    }
}

void MatrixDisplayStatusFlag::dump_config() {
    LOG_BINARY_SENSOR("", "MatrixDisplayStatusFlag", this);
    LOG_UPDATE_INTERVAL(this);
}

} // namespace esphome::matrix_display::matrix_display_status

// SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only
#include "matrix_display_version.h"

namespace esphome::matrix_display::matrix_display_version {

static const char *const TAG = "matrix_display.version";

void MatrixDisplayVersion::update() {
    if (this->display_ == nullptr)
        return;
    MatrixPanel_FPGA_SPI::FpgaVersion version;
    if (!this->display_->read_version(version))
        return;
    char buf[MatrixPanel_FPGA_SPI::VERSION_STR_MAX];
    MatrixPanel_FPGA_SPI::formatVersion(version, buf, sizeof(buf));
    this->publish_state(buf);
}

void MatrixDisplayVersion::dump_config() {
    LOG_TEXT_SENSOR("", "MatrixDisplayVersion", this);
    LOG_UPDATE_INTERVAL(this);
}

} // namespace esphome::matrix_display::matrix_display_version

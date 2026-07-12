// SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only
#include "matrix_display_status_value.h"

namespace esphome::matrix_display::matrix_display_status_value {

static const char *const TAG = "matrix_display.status_value";

void MatrixDisplayStatusValue::dump_config() {
    LOG_SENSOR("", "MatrixDisplayStatusValue", this);
    ESP_LOGCONFIG(TAG, "  Status register: 0x%02X", this->address_);
    LOG_UPDATE_INTERVAL(this);
}

} // namespace esphome::matrix_display::matrix_display_status_value

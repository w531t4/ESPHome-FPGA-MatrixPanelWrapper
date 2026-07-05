// SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only
#include "matrix_display_update_duration.h"

namespace esphome::matrix_display::matrix_display_update_duration {

static const char *const TAG = "matrix_display.sensor";

void MatrixDisplayUpdateDuration::dump_config() {
    LOG_SENSOR("", "MatrixDisplayUpdateDuration", this);
    LOG_UPDATE_INTERVAL(this);
}

} // namespace esphome::matrix_display::matrix_display_update_duration

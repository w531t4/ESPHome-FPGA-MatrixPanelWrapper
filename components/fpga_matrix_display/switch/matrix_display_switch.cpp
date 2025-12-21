// SPDX-FileCopyrightText: 2019 ESPHome
// SPDX-FileCopyrightText: 2025 Aaron White <w531t4@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only
#include "matrix_display_switch.h"

namespace esphome::matrix_display::matrix_display_switch {
void MatrixDisplaySwitch::setup() {
    auto initial_state =
        this->get_initial_state_with_restore_mode().value_or(false);
    ESP_LOGD(TAG, "Setting up matrix display switch with initial state: %i",
             initial_state);
    write_state(initial_state);
}

void MatrixDisplaySwitch::write_state(bool state) {
    // Update display status and forwad state to all registered power switches.
    display_->set_state(state);
    for (MatrixDisplaySwitch *switch_ : display_->get_power_switches()) {
        switch_->publish_state(state);
    }
};

void MatrixDisplaySwitch::dump_config() {
    ESP_LOGCONFIG(TAG, "MatrixDisplay Power Switch");
};
} // namespace esphome::matrix_display::matrix_display_switch
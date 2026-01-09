// SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/core/hal.h"

#include "../matrix_display.h"

namespace esphome {
namespace matrix_display {

class MatrixDisplayLightOutput : public light::LightOutput {
  public:
    void set_display(MatrixDisplay *display) { this->display_ = display; }

    light::LightTraits get_traits() override {
        light::LightTraits traits;
        traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
        return traits;
    }

    void write_state(light::LightState *state) override {
        float bright = 0.0f;
        state->current_values_as_brightness(&bright);
        if (this->display_ != nullptr) {
            const int level = static_cast<int>(bright * 255.0f + 0.5f);
            this->display_->set_brightness(level);
        }
    }

  protected:
    MatrixDisplay *display_{nullptr};
};

} // namespace matrix_display
} // namespace esphome

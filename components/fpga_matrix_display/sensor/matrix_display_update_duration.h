// SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "../matrix_display.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome::matrix_display::matrix_display_update_duration {

/**
 * Reports the average duration of MatrixDisplay::update() over the sensor's
 * configured update_interval. Replaces the old per-100-frames ESP_LOGD line.
 */
class MatrixDisplayUpdateDuration : public sensor::Sensor,
                                    public PollingComponent {
  public:
    void update() override {
        if (this->display_ == nullptr)
            return;
        this->publish_state(this->display_->get_avg_update_micros());
    }

    void dump_config() override;

    /**
     * Sets the reference to the display component this sensor measures.
     *
     * @param display Matrix display component reference
     */
    void set_display(MatrixDisplay *display) { this->display_ = display; }

  protected:
    /// @brief display component this sensor measures
    MatrixDisplay *display_{nullptr};
};

} // namespace esphome::matrix_display::matrix_display_update_duration

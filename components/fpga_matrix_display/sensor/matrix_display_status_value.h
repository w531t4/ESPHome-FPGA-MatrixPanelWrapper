// SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "../matrix_display.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome::matrix_display::matrix_display_status_value {

/**
 * Polls one numeric FPGA status register over the status SPI at the
 * configured update_interval and publishes its value. When the read fails
 * (status bus not configured, FPGA in reset, no fresh frame) nothing is
 * published, so the last known state is retained; the failure detail is
 * logged by MatrixDisplay.
 */
class MatrixDisplayStatusValue : public sensor::Sensor,
                                 public PollingComponent {
  public:
    void update() override {
        if (this->display_ == nullptr)
            return;
        uint64_t value;
        if (!this->display_->read_status_value(this->address_, value))
            return;
        this->publish_state(static_cast<float>(value));
    }

    void dump_config() override;

    /**
     * Sets the reference to the display component this sensor reads from.
     *
     * @param display Matrix display component reference
     */
    void set_display(MatrixDisplay *display) { this->display_ = display; }

    /**
     * Selects the status register this sensor publishes.
     *
     * @param address a MatrixPanel_FPGA_SPI::STATUS_ADDR_* constant
     */
    void set_address(uint8_t address) { this->address_ = address; }

  protected:
    /// @brief display component this sensor reads from
    MatrixDisplay *display_{nullptr};
    /// @brief status register address to publish
    uint8_t address_{0};
};

} // namespace esphome::matrix_display::matrix_display_status_value

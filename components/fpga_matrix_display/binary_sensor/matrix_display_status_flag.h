// SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "../matrix_display.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/component.h"

namespace esphome::matrix_display::matrix_display_status {

/// Which bit of the FPGA FLAGS status register this sensor reports.
enum class StatusFlagType : uint8_t {
    FPGA_READY,
    CTRL_BUSY,
    CTRL_READY_FOR_DATA,
};

/**
 * Polls one FPGA status flag over the status SPI at the configured
 * update_interval and publishes it. When the read fails (status bus not
 * configured, FPGA in reset, no fresh frame) nothing is published, so the
 * last known state is retained.
 */
class MatrixDisplayStatusFlag : public binary_sensor::BinarySensor,
                                public PollingComponent {
  public:
    void update() override;

    void dump_config() override;

    /**
     * Sets the reference to the display component this sensor reads from.
     *
     * @param display Matrix display component reference
     */
    void set_display(MatrixDisplay *display) { this->display_ = display; }

    /**
     * Selects which FLAGS bit this sensor publishes.
     *
     * @param type flag selector
     */
    void set_flag_type(StatusFlagType type) { this->flag_type_ = type; }

  protected:
    /// @brief display component this sensor reads from
    MatrixDisplay *display_{nullptr};
    /// @brief which FLAGS bit to publish
    StatusFlagType flag_type_{StatusFlagType::FPGA_READY};
};

} // namespace esphome::matrix_display::matrix_display_status

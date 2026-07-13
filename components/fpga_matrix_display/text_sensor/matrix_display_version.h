// SPDX-FileCopyrightText: 2026 Aaron White <w531t4@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "../matrix_display.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

namespace esphome::matrix_display::matrix_display_version {

/**
 * Polls the FPGA gateware version register (STATUS_ADDR_VERSION) at the
 * configured update_interval and publishes it as a formatted string, e.g.
 * "v1.2.3+45.sha1a2b3c4d-dirty". The library owns the decoding and formatting
 * (MatrixPanel_FPGA_SPI::readVersion / formatVersion). When the read fails
 * (status bus not configured, FPGA in reset, no fresh frame) nothing is
 * published, so the last known state is retained; the failure detail is
 * logged by MatrixDisplay.
 */
class MatrixDisplayVersion : public text_sensor::TextSensor,
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

  protected:
    /// @brief display component this sensor reads from
    MatrixDisplay *display_{nullptr};
};

} // namespace esphome::matrix_display::matrix_display_version

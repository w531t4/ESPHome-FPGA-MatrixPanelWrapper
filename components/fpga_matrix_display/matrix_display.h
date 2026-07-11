// SPDX-FileCopyrightText: 2019 ESPHome
// SPDX-FileCopyrightText: 2025, 2026 Aaron White <w531t4@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <utility>
#include <vector>

#include "esphome/components/display/display_buffer.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include <esp_timer.h>

#include "matrix_panel_fpga.hpp"

namespace esphome {
namespace matrix_display {
class MatrixDisplay;
namespace matrix_display_switch {
class MatrixDisplaySwitch;
static void set_reference(MatrixDisplaySwitch *switch_, MatrixDisplay *display);
} // namespace matrix_display_switch

namespace matrix_display_brightness {
class MatrixDisplayBrightness;
static void set_reference(MatrixDisplayBrightness *brightness,
                          MatrixDisplay *display);
} // namespace matrix_display_brightness

class MatrixDisplay : public display::DisplayBuffer {
  public:
    void setup() override;

    void dump_config() override;

    void update() override;

    /**
     * Registers a power switch on this matrix entity.
     *
     * @param power_switch Reference to a power switch
     */
    void register_power_switch(
        matrix_display_switch::MatrixDisplaySwitch *power_switch) {
        this->power_switches_.push_back(power_switch);
        set_reference(power_switch, this);
    };

    /**
     *Registers a brightness value on this matrix entity.
     *
     * @param brightness Reference to a brightness number entity
     */
    void register_brightness(
        matrix_display_brightness::MatrixDisplayBrightness *brightness) {
        this->brightness_values_.push_back(brightness);
        set_reference(brightness, this);
    };

    /**
     * Sets the hight of each individual panel.
     *
     * @param height height in pixels for a single panel
     */
    void set_panel_height(int panel_height) {
        this->mxconfig_.mx_height = panel_height;
    }

    /**
     * Sets the width of each individual panel.
     *
     * @param width width in pixels for a single panel
     */
    void set_panel_width(int panel_width) {
        this->mxconfig_.mx_width = panel_width;
    }

    /**
     * Sets the nr of panels chained on after another
     *
     * @param chain_length nr of panels
     */
    void set_chain_length(int chain_length) {
        this->mxconfig_.chain_length = chain_length;
    }

    /**
     * Sets the initial brightness of the display.
     *
     * @param brightness brightness value (0-255)
     */
    void set_initial_brightness(int brightness) {
        this->initial_brightness_ = brightness;
        this->current_brightness_ = brightness;
    };

    /**
     * Sets the boolean which decides whether watchdog must be fed
     *
     * @param use_watchdog
     */
    void set_initial_watchdog(bool use_watchdog) {
        this->use_watchdog = use_watchdog;
    };

    /**
     * Sets the iterval at which watchdog must be fed
     * @param watchdog_interval_usec
     */
    void set_initial_watchdog_interval_usec(int interval) {
        this->watchdog_interval_usec = interval;
    };

    /**
     * Sets the max time (ms) a display flush waits for the SPI worker to drain
     * before giving up on the frame. Caps the wait so an unresponsive FPGA
     * can't make update() block forever and leave the device frozen.
     *
     * @param ms timeout in milliseconds
     */
    void set_worker_idle_timeout_ms(uint32_t ms) {
        this->worker_idle_timeout_ms_ = ms;
    };

    /**
     * Gets the inital brightness value from this display.
     */
    int get_initial_brightness() { return this->initial_brightness_; }
    int get_current_brightness() const { return this->current_brightness_; }

    /**
     * @return duration of the most recent update() call, in microseconds.
     */
    uint32_t get_last_update_micros() const {
        return this->last_update_micros_;
    }

    /**
     * Returns the moving average of recent update() durations, in microseconds
     * (smoothed over ~kUpdateTimeWindow frames). Non-mutating, so it can be read
     * at any cadence. Falls back to the last sample before the window has
     * filled, or 0 if no update() has run yet.
     */
    uint32_t get_avg_update_micros() const {
        if (this->update_time_count_ == 0)
            return this->last_update_micros_;
        // sum stays near kUpdateTimeWindow * mean, so it cannot overflow and the
        // quotient fits comfortably in 32 bits.
        return static_cast<uint32_t>(this->update_time_sum_ /
                                     this->update_time_count_);
    }
    uint32_t get_reset_epoch() const {
        return this->dma_display_ ? this->dma_display_->get_reset_epoch() : 0;
    }

    void set_pins(InternalGPIOPin *SPI_CE_pin, InternalGPIOPin *SPI_CLK_pin,
                  InternalGPIOPin *SPI_MOSI_pin,
                  InternalGPIOPin *FPGA_RESETSTATUS_pin,
                  InternalGPIOPin *FPGA_BUSY_pin) {
        this->mxconfig_.gpio = {static_cast<int8_t>(SPI_CE_pin->get_pin()),
                                static_cast<int8_t>(SPI_CLK_pin->get_pin()),
                                static_cast<int8_t>(SPI_MOSI_pin->get_pin()),
                                static_cast<int8_t>(FPGA_RESETSTATUS_pin->get_pin()),
                                static_cast<int8_t>(FPGA_BUSY_pin->get_pin())};
    }

    /**
     * Sets the pins for the status readback SPI bus (FPGA reg_spi_responder).
     * The ESP32 is master on this bus: sck/cs outputs, miso input. The
     * feature stays disabled unless all three are configured.
     */
    void set_status_pins(InternalGPIOPin *STATUS_SPI_SCK_pin,
                         InternalGPIOPin *STATUS_SPI_CS_pin,
                         InternalGPIOPin *STATUS_SPI_MISO_pin) {
        this->mxconfig_.status_gpio.sck =
            static_cast<int8_t>(STATUS_SPI_SCK_pin->get_pin());
        this->mxconfig_.status_gpio.cs =
            static_cast<int8_t>(STATUS_SPI_CS_pin->get_pin());
        this->mxconfig_.status_gpio.miso =
            static_cast<int8_t>(STATUS_SPI_MISO_pin->get_pin());
    }

    /**
     * Reads the FPGA status flags over the status SPI. Synchronous (takes
     * the SPI mutex). False when the display isn't up, the status bus isn't
     * configured, or no fresh frame could be read; failures are logged with
     * the library's failure reason and the raw frame bytes.
     */
    bool read_status_flags(MatrixPanel_FPGA_SPI::FpgaStatusFlags &out);

    /**
     * Sets the clock speed
     *
     * @param speed i2s clock speed
     */
    void set_spispeed(FPGA_SPI_CFG::clk_speed speed) {
        this->mxconfig_.spispeed = speed;
    };

    display::DisplayType get_display_type() override {
        return display::DisplayType::DISPLAY_TYPE_COLOR;
    }

    /**
     * Sets the on/off state of the matrix display
     *
     * @param state new state
     */
    void set_state(bool state) { this->enabled_ = state; }

    /**
     * Sets the brightness value of the display
     *
     * @param brightness new brightness value (0-255)
     */
    void set_brightness(int brightness);

    /**
     * Forces the display to show a known test graphic built from FPGA commands.
     */
    void enter_test_state();

    /**
     * Restores normal update behavior after a test graphic was shown.
     */
    void exit_test_state();

    /**
     * @return true if the display is currently locked into the test state.
     */
    bool is_test_state_active() const { return this->test_state_active_; }

    /**
     * Gets a vector of all registered power switches from this matrix display
     */
    std::vector<matrix_display_switch::MatrixDisplaySwitch *>
    get_power_switches() {
        return this->power_switches_;
    }

    /**
     * Gets a vector of all registered brightness number entities from this
     * matrix display
     */
    std::vector<matrix_display_brightness::MatrixDisplayBrightness *>
    get_brightness_values() {
        return this->brightness_values_;
    }
    void write_display_data();
    void swap();
    static void periodic_callback(void *arg);

    void run_test_state_sequence_();

  protected:
    /// @brief Wrapped matrix display
    MatrixPanel_FPGA_SPI *dma_display_ = nullptr;

    /// @brief Matrix configuration
    FPGA_SPI_CFG mxconfig_;

    /// @brief initial brightness of the display
    int initial_brightness_ = 128;
    int current_brightness_ = 128;

    /// @brief duration of the most recent update() call, in microseconds
    uint32_t last_update_micros_ = 0;

    /// @brief smoothing window (in frames) for the reported moving average.
    /// Larger = smoother/slower to react. Old samples decay exponentially
    /// rather than being stored, so memory is fixed regardless of the window.
    static constexpr uint32_t kUpdateTimeWindow = 64;
    /// @brief running sum, held near kUpdateTimeWindow * mean (see push helper)
    uint64_t update_time_sum_ = 0;
    /// @brief samples seen so far, saturating at kUpdateTimeWindow
    uint32_t update_time_count_ = 0;

    /**
     * Folds an update() duration into the moving average. While warming up it
     * is a plain cumulative mean; once kUpdateTimeWindow samples have been seen
     * it drops one window-average worth per new sample, keeping the sum near
     * kUpdateTimeWindow * mean. Bounded, overflow-free, and O(1) in both time
     * and memory -- no per-sample buffer (EMA with alpha = 1 / window).
     *
     * @param elapsed duration of the update() call, in microseconds
     */
    void push_update_micros_(uint32_t elapsed) {
        this->last_update_micros_ = elapsed;
        if (this->update_time_count_ < kUpdateTimeWindow) {
            // Warm-up: exact cumulative mean until the window fills.
            this->update_time_sum_ += elapsed;
            this->update_time_count_++;
        } else {
            // Steady state: drop one window-average, add the new sample.
            // sum >= window_avg always, so the subtraction cannot underflow.
            uint64_t window_avg = this->update_time_sum_ / kUpdateTimeWindow;
            this->update_time_sum_ =
                this->update_time_sum_ - window_avg + elapsed;
        }
    }

    /// @brief on-off status of the display matrix
    bool enabled_ = false;

    /// @brief power switches belonging to this matrix display
    std::vector<matrix_display_switch::MatrixDisplaySwitch *> power_switches_;

    /// @brief brightness value number entities belonging to this matrix display
    std::vector<matrix_display_brightness::MatrixDisplayBrightness *>
        brightness_values_;

    int get_width_internal() override {
        return this->mxconfig_.mx_width * this->mxconfig_.chain_length;
    };
    int get_height_internal() override { return this->mxconfig_.mx_height; };

    /**
     * Draws a single pixel on the display.
     *
     * @param x x-coordinate of the pixel
     * @param y y-coordinate of the pixel
     * @param color Color of the pixel
     */
    void draw_absolute_pixel_internal(int x, int y, Color color) override;
    int cached_width_ = 0;
    int cached_height_ = 0;
    static constexpr int kChunkWidth = 16;
    bool use_watchdog = false;
    int watchdog_interval_usec = 1000000;
    /// @brief max time (ms) a flush waits for the SPI worker before giving up on
    /// the frame; keeps an unresponsive FPGA from making update() block forever
    uint32_t worker_idle_timeout_ms_ = 1500;
    uint32_t watchdog_last_checkin = 0;
    esp_timer_handle_t periodic_timer;
    std::vector<uint8_t> dirty_chunks_;
    uint8_t *chunk_buffer_ = nullptr;
    size_t chunk_buffer_bytes_ = 0;
    int chunk_count_ = 0;
    bool dirty_any_ = false; // Fast path: skip flushing when no columns changed.

    bool test_state_active_ = false;
    bool test_state_dirty_ = false;
};

} // namespace matrix_display
} // namespace esphome

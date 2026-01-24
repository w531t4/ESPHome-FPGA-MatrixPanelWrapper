// SPDX-FileCopyrightText: 2019 ESPHome
// SPDX-FileCopyrightText: 2025 Aaron White <w531t4@gmail.com>
// SPDX-License-Identifier: GPL-3.0-only
#include "matrix_display.h"
#include "esphome/core/helpers.h" // For micros()
#include <algorithm>
#include <cstring>
#include <esp_heap_caps.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esphome {
namespace matrix_display {

static const char *const TAG = "matrix_display";

void MatrixDisplay::enter_test_state() {
    this->test_state_active_ = true;
    this->test_state_dirty_ = true;
    this->run_test_state_sequence_();
}

void MatrixDisplay::exit_test_state() {
    this->test_state_active_ = false;
    this->test_state_dirty_ = false;
}

void MatrixDisplay::run_test_state_sequence_() {
    if (!this->test_state_active_ || !this->test_state_dirty_ ||
        this->dma_display_ == nullptr) {
        return;
    }

    this->dma_display_->run_test_graphic();
    this->test_state_dirty_ = false;
}

/**
 * Initialize the wrapped matrix display with user parameters
 */
void MatrixDisplay::periodic_callback(void *arg) {
    auto *self = static_cast<MatrixDisplay *>(arg);
    // ESP_LOGD(TAG, "interval_usec=%d has elapsed. feeding watchdog.",
    //          self->watchdog_interval_usec);
    self->dma_display_->fulfillWatchdog();
}
void MatrixDisplay::setup() {
    ESP_LOGCONFIG(TAG, "Setting up MatrixDisplay...");
    ESP_LOGD("MatrixDisplay", "Framebuffer size: %dx%d",
             this->get_width_internal(), this->get_height_internal());
    // The min refresh rate correlates with the update frequency of the
    // component
    this->mxconfig_.min_refresh_rate = 1000 / update_interval_;
    display::DisplayBuffer::setup();
    this->cached_width_ = this->get_width_internal();
    this->cached_height_ = this->get_height_internal();
    // Split the panel into fixed-width chunks for dirty tracking.
    this->chunk_count_ =
        (this->cached_width_ + kChunkWidth - 1) / kChunkWidth;
    this->dirty_chunks_.assign(this->chunk_count_, 0);
    size_t bufsize = this->cached_width_ * this->cached_height_ * 3;
    this->init_internal_(bufsize);
    if (this->buffer_ == nullptr) {
        ESP_LOGE(TAG, "Framebuffer allocation failed; display not ready");
        return;
    }
    // Preallocate a single DMA-capable chunk buffer reused for each flush.
    const int max_chunk_width = std::min(kChunkWidth, this->cached_width_);
    this->chunk_buffer_bytes_ =
        static_cast<size_t>(max_chunk_width) * this->cached_height_ * 3;
    this->chunk_buffer_ = static_cast<uint8_t *>(
        heap_caps_malloc(this->chunk_buffer_bytes_, MALLOC_CAP_DMA));
    if (this->chunk_buffer_ == nullptr) {
        ESP_LOGE(TAG, "Chunk buffer allocation failed; display not ready");
        return;
    }
    this->dirty_any_ = true; // Force initial flush so FPGA matches the buffer.

    // Display Setup
    dma_display_ = new MatrixPanel_FPGA_SPI(this->mxconfig_);
    dma_display_->set_worker_core(1);
    dma_display_->enable_worker(true);
    this->dma_display_->begin();

    if (this->use_watchdog) {
        const esp_timer_create_args_t periodic_timer_args = {
            .callback = &MatrixDisplay::periodic_callback,
            .arg = this,
            .name = "periodic_action_timer"};
        ESP_ERROR_CHECK(
            esp_timer_create(&periodic_timer_args, &periodic_timer));
        // Start the timer to trigger every 1,000,000 microseconds (1 second)
        ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer,
                                                 this->watchdog_interval_usec));
    }
    set_brightness(this->initial_brightness_);
    this->dma_display_->clearScreen();

    // Default to off if power switches are present
    set_state(!this->power_switches_.size());
}

/**
 * Updates the displayed image on the matrix. Dual buffers are used to prevent
 * blanking in-between frames.
 */
void MatrixDisplay::update() {
    if (this->test_state_active_) {
        this->run_test_state_sequence_();
        return;
    }

    uint32_t start_time = micros();
    static uint32_t time_sum = 0;
    static uint32_t time_count = 0;
    if (this->dma_display_ != nullptr &&
        this->dma_display_->consume_fpga_reset()) {
        ESP_LOGW(TAG, "FPGA reset detected; resyncing display state");
        this->dma_display_->resync_after_fpga_reset(
            static_cast<uint8_t>(this->initial_brightness_));
    }
    // uint32_t update_end_time, update_start_time;
    if (this->enabled_) {
        // Draw updates to the screen
        // update_start_time = micros();
        this->do_update_();
        // update_end_time = micros();
        write_display_data();
        // size_t bufsize = this->cached_width_ * this->cached_height_ * 3;
        // memset(this->buffer_, 0x00, bufsize);
    } else {
        this->dma_display_->clearScreen();
    }
    uint32_t end_time = micros();
    uint32_t elapsed_time = end_time - start_time;
    time_sum = time_sum + elapsed_time;
    time_count++;
    if (time_count > 2 && (time_count % 100) == 0) {
        ESP_LOGD(TAG, "update() took %u microseconds. avg=%u", elapsed_time,
                 time_sum / time_count);
    }
}

void MatrixDisplay::dump_config() {
    ESP_LOGCONFIG(TAG, "MatrixDisplay:");

    FPGA_SPI_CFG cfg = this->dma_display_->getCfg();

    // Log pin settings
    ESP_LOGCONFIG(TAG,
                  "  Pins: SPI_CE:%i, SPI_CLK:%i, SPI_MOSI:%i, FPGA_RESETSTATUS:%i, "
                  "FPGA_BUSY:%i",
                  cfg.gpio.ce, cfg.gpio.clk, cfg.gpio.mosi, cfg.gpio.fpga_resetstatus,
                  cfg.gpio.fpga_busy);

    ESP_LOGCONFIG(TAG, "  SPI Speed: %u MHz", (uint32_t)cfg.spispeed / 1000000);
    ESP_LOGCONFIG(TAG, "  Min Refresh Rate: %i", cfg.min_refresh_rate);
    ESP_LOGCONFIG(TAG, "  width: %i", cfg.mx_width);
    ESP_LOGCONFIG(TAG, "  height: %i", cfg.mx_height);
    ESP_LOGCONFIG(TAG, "  chain_length: %i", cfg.chain_length);
}

void MatrixDisplay::set_brightness(int brightness) {
    // Wrap brightness function
    brightness = clamp(brightness, 0, 255);
    this->current_brightness_ = brightness;
    this->dma_display_->setBrightness8(brightness);
}

void HOT MatrixDisplay::draw_absolute_pixel_internal(int x, int y,
                                                     Color color) {
    if (x < 0 || x >= this->cached_width_ || y < 0 || y >= this->cached_height_)
        return;
    const int i = (y * this->cached_width_ + x) * 3;
    this->buffer_[i + 0] = color.red;
    this->buffer_[i + 1] = color.green;
    this->buffer_[i + 2] = color.blue;
    // Track dirty state at the chunk level to avoid per-column updates.
    if (!this->dirty_chunks_.empty()) {
        const size_t chunk = static_cast<size_t>(x / kChunkWidth);
        this->dirty_chunks_[chunk] = 1;
    }
    // Any pixel write means at least one chunk must be flushed.
    this->dirty_any_ = true;
};
void HOT MatrixDisplay::swap() { this->dma_display_->swapFrame(); }
void MatrixDisplay::write_display_data() {
    if (this->buffer_ == nullptr || this->chunk_buffer_ == nullptr) {
        ESP_LOGE("MatrixDisplay:write_display_data",
                 "buffer_ or chunk_buffer_ not initialized!");
        return;
    }
    // Fast path: nothing changed since the last flush.
    if (!this->dirty_any_)
        return;

    const int width = this->cached_width_;
    const int height = this->cached_height_;
    // kChunkWidth is a fixed upper bound; edge chunks may be narrower.
    const int chunk_width = kChunkWidth;
    const bool worker_enabled = this->dma_display_->is_worker_enabled();
    // Flush only the chunks marked dirty to reduce SPI traffic.
    bool any_sent = false;
    bool all_sent = true;

    for (int chunk = 0; chunk < this->chunk_count_; ++chunk) {
        if (this->dirty_chunks_[static_cast<size_t>(chunk)] == 0)
            continue;
        // Avoid reusing the shared chunk buffer while worker jobs are pending.
        if (worker_enabled) {
            // Wait for the worker to finish any in-flight SPI transfer before
            // repacking the shared chunk buffer.
            while (!this->dma_display_->worker_is_idle()) {
                vTaskDelay(1);
            }
        }
        const int x = chunk * chunk_width;
        const int w = std::min(chunk_width, width - x);
        // Each rect payload is packed row-major: w * height * 3 bytes.
        const size_t rect_bytes =
            static_cast<size_t>(w) * static_cast<size_t>(height) * 3;
        if (rect_bytes > this->chunk_buffer_bytes_) {
            ESP_LOGE(TAG, "Chunk buffer too small for %dx%d rect", w, height);
            all_sent = false;
            break;
        }
        // Pack row-major data for drawRectRGB888_prealloc.
        size_t dst = 0;
        for (int y = 0; y < height; ++y) {
            const size_t src =
                (static_cast<size_t>(y) * width + x) * 3;
            const size_t span = static_cast<size_t>(w) * 3;
            std::memcpy(this->chunk_buffer_ + dst, this->buffer_ + src, span);
            dst += span;
        }
        // Stream the chunk as a rect write using the preallocated buffer.
        this->dma_display_->drawRectRGB888_prealloc(
            x, 0, w, height, this->chunk_buffer_, rect_bytes);
        if (worker_enabled) {
            // Ensure the worker has finished consuming the buffer before reuse.
            while (!this->dma_display_->worker_is_idle()) {
                vTaskDelay(1);
            }
        }
        // Mark the chunk clean only after a successful send.
        this->dirty_chunks_[static_cast<size_t>(chunk)] = 0;
        any_sent = true;
    }

    // Only swap/copy if we issued at least one chunk update.
    if (any_sent) {
        // Commit the staged updates to the visible buffer.
        this->dma_display_->swapFrame();
        this->dma_display_->copyFrame();
    }

    // Clear the dirty flag only if all pending chunks were flushed.
    if (all_sent) {
        this->dirty_any_ = false;
    } else {
        // Recompute dirty_any_ based on any remaining dirty chunks.
        this->dirty_any_ = false;
        for (int chunk = 0; chunk < this->chunk_count_; ++chunk) {
            if (this->dirty_chunks_[static_cast<size_t>(chunk)] != 0) {
                this->dirty_any_ = true;
                break;
            }
        }
    }
};

} // namespace matrix_display
} // namespace esphome

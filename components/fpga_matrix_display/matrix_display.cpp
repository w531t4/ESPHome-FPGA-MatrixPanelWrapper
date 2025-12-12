#include "matrix_display.h"
#include "esphome/core/helpers.h" // For micros()

namespace esphome {
namespace matrix_display {

static const char *const TAG = "matrix_display";

/**
 * Initialize the wrapped matrix display with user parameters
 */
void MatrixDisplay::periodic_callback(void *arg) {
    auto *self = static_cast<MatrixDisplay *>(arg);
    ESP_LOGD(TAG, "interval_usec=%d has elapsed. feeding watchdog.",
             self->watchdog_interval_usec);
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
    this->cached_width_ = this->get_width_internal();
    this->cached_height_ = this->get_height_internal();
    size_t bufsize = this->cached_width_ * this->cached_height_ * 3;
    this->init_internal_(bufsize);
    memset(this->buffer_, 0x00, bufsize);
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
    uint32_t start_time = micros();
    static uint32_t time_sum = 0;
    static uint32_t time_count = 0;
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
    if (time_count > 2 && (time_count % 10) == 0) {
        ESP_LOGD(TAG, "update() took %u microseconds. avg=%u", elapsed_time,
                 time_sum / time_count);
    }
}

void MatrixDisplay::dump_config() {
    ESP_LOGCONFIG(TAG, "MatrixDisplay:");

    FPGA_SPI_CFG cfg = this->dma_display_->getCfg();

    // Log pin settings
    ESP_LOGCONFIG(
        TAG, "  Pins: SPI_CE:%i, SPI_CLK:%i, SPI_MOSI:%i FPGA_RESET:%i",
        cfg.gpio.ce, cfg.gpio.clk, cfg.gpio.mosi, cfg.gpio.fpga_reset);

    ESP_LOGCONFIG(TAG, "  SPI Speed: %u MHz", (uint32_t)cfg.spispeed / 1000000);
    ESP_LOGCONFIG(TAG, "  Min Refresh Rate: %i", cfg.min_refresh_rate);
    ESP_LOGCONFIG(TAG, "  width: %i", cfg.mx_width);
    ESP_LOGCONFIG(TAG, "  height: %i", cfg.mx_height);
    ESP_LOGCONFIG(TAG, "  chain_length: %i", cfg.chain_length);
}

void MatrixDisplay::set_brightness(int brightness) {
    // Wrap brightness function
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
};

void HOT MatrixDisplay::set_pixel(uint16_t x, uint16_t y, uint8_t red,
                                  uint8_t green, uint8_t blue) {
    this->dma_display_->drawPixelRGB888(x, y, red, green, blue);
};
void HOT MatrixDisplay::swap() { this->dma_display_->swapFrame(); }
void MatrixDisplay::write_display_data() {
    if (this->buffer_ == nullptr) {
        ESP_LOGE("MatrixDisplay:write_display_data",
                 "buffer_ not initialized!");
        return;
    }
    // writing raw frames... failure for spi transaction size
    // this->dma_display_->drawFrameRGB888(&this->buffer_[0],
    // this->get_width_internal() * this->get_height_internal() * 3);

    for (int y = 0; y < this->cached_height_; y++) {
        this->dma_display_->drawRowRGB888(
            static_cast<uint8_t>(y),
            &this->buffer_[(y * this->cached_width_) * 3],
            this->cached_width_ * 3);
    }
    this->dma_display_->swapFrame();
};

void MatrixDisplay::fill(Color color) {
    // Wrap fill screen method
    this->dma_display_->fillScreenRGB888(color.r, color.g, color.b);
}

// void MatrixDisplay::filled_rectangle(int x1, int y1, int width, int height,
// Color color)
// {
//     // Wrap fill rectangle method
//     this->dma_display_->fillRect(x1, y1, width, height, color.r, color.g,
//     color.b);
// }

} // namespace matrix_display
} // namespace esphome

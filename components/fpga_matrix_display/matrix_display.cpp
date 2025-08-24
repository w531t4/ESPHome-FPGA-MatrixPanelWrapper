#include "matrix_display.h"
#include "esphome/core/helpers.h" // For micros()

namespace esphome
{
    namespace matrix_display
    {

        static const char *const TAG = "matrix_display";

        /**
         * Initialize the wrapped matrix display with user parameters
         */
        void MatrixDisplay::setup()
        {
            ESP_LOGCONFIG(TAG, "Setting up MatrixDisplay...");
            ESP_LOGD("MatrixDisplay", "Framebuffer size: %dx%d", this->get_width_internal(), this->get_height_internal());
            // The min refresh rate correlates with the update frequency of the component
            this->mxconfig_.min_refresh_rate = 1000 / update_interval_;
            display::DisplayBuffer::setup();

            // Display Setup
            dma_display_ = new MatrixPanel_FPGA_SPI(this->mxconfig_);
            this->dma_display_->begin();
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
         * Updates the displayed image on the matrix. Dual buffers are used to prevent blanking in-between frames.
         */
        void MatrixDisplay::update()
        {
            // uint32_t start_time = micros();
            // uint32_t update_end_time, update_start_time;
            if (this->enabled_)
            {
                // Draw updates to the screen
                // update_start_time = micros();
                this->do_update_();
                // update_end_time = micros();
                write_display_data();
                size_t bufsize = this->cached_width_ * this->cached_height_ * 3;
                memset(this->buffer_, 0x00, bufsize);
            }
            else
            {
                this->dma_display_->clearScreen();
            }
            // uint32_t end_time = micros();
            // uint32_t elapsed_time = end_time - start_time;
            // if (this->enabled_) {
            //     uint32_t elapsed_update_time = update_end_time - update_start_time;
            //     ESP_LOGD(TAG, "do_update_() took %u microseconds", elapsed_update_time);
            // }
            // ESP_LOGD(TAG, "update() took %u microseconds", elapsed_time);
            if (this->use_watchdog) {
                auto now = micros();
                if ((now - this->watchdog_last_checkin) > (1 * 1000 * 1000)) {
                    ESP_LOGD(TAG, "feeding watchdog. %u microseconds have elapsed", (now - this->watchdog_last_checkin));
                    this->dma_display_->fulfillWatchdog();
                    this->watchdog_last_checkin = now;
                }
            }
        }

        void MatrixDisplay::dump_config()
        {
            ESP_LOGCONFIG(TAG, "MatrixDisplay:");

            FPGA_SPI_CFG cfg = this->dma_display_->getCfg();

            // Log pin settings
            ESP_LOGCONFIG(TAG, "  Pins: SPI_CE:%i, SPI_CLK:%i, SPI_MOSI:%i FPGA_RESET:%i", cfg.gpio.ce, cfg.gpio.clk, cfg.gpio.mosi, cfg.gpio.fpga_reset);

            ESP_LOGCONFIG(TAG, "  SPI Speed: %u MHz", (uint32_t)cfg.spispeed / 1000000);
            ESP_LOGCONFIG(TAG, "  Min Refresh Rate: %i", cfg.min_refresh_rate);
            ESP_LOGCONFIG(TAG, "  width: %i", cfg.mx_width);
            ESP_LOGCONFIG(TAG, "  height: %i", cfg.mx_height);
            ESP_LOGCONFIG(TAG, "  chain_length: %i", cfg.chain_length);
        }

        void MatrixDisplay::set_brightness(int brightness)
        {
            // Wrap brightness function
            this->dma_display_->setBrightness8(brightness);
        }

        void HOT MatrixDisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
            const int i = (y * this->cached_width_ + x) * 3;
            this->buffer_[i + 0] = color.red;
            this->buffer_[i + 1] = color.green;
            this->buffer_[i + 2] = color.blue;
        };

        void MatrixDisplay::write_display_data() {
            if (this->buffer_ == nullptr) {
                ESP_LOGE("MatrixDisplay:write_display_data", "buffer_ not initialized!");
                return;
            }
            // writing raw frames... failure for spi transaction size
            //this->dma_display_->drawFrameRGB888(&this->buffer_[0], this->get_width_internal() * this->get_height_internal() * 3);

            for (int y = 0; y < this->cached_height_; y++) {
                this->dma_display_->drawRowRGB888(static_cast<uint8_t>(y), &this->buffer_[(y*this->cached_width_)*3], this->cached_width_ * 3);
            }
            this->dma_display_->swapFrame();
        };

        void MatrixDisplay::fill(Color color)
        {
            // Wrap fill screen method
            this->dma_display_->fillScreenRGB888(color.r, color.g, color.b);
        }

        void MatrixDisplay::filled_rectangle(int x1, int y1, int width, int height, Color color)
        {
            // Wrap fill rectangle method
            this->dma_display_->fillRect(x1, y1, width, height, color.r, color.g, color.b);
        }

    } // namespace matrix_display
} // namespace esphome

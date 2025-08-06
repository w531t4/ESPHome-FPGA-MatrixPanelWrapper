#include "matrix_display.h"

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

            // The min refresh rate correlates with the update frequency of the component
            this->mxconfig_.min_refresh_rate = 1000 / update_interval_;

            // Display Setup
            dma_display_ = new MatrixPanel_FPGA_SPI(this->mxconfig_);
            this->dma_display_->begin();
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
            if (this->enabled_)
            {
                // Draw updates to the screen
                this->do_update_();
            }
            else
            {
                this->dma_display_->clearScreen();
            }
        }

        void MatrixDisplay::dump_config()
        {
            ESP_LOGCONFIG(TAG, "MatrixDisplay:");

            FPGA_SPI_CFG cfg = this->dma_display_->getCfg();

            // Log pin settings
            ESP_LOGCONFIG(TAG, "  Pins: SPI_CE:%i, SPI_CLK:%i, SPI_MOSI:%i", cfg.gpio.ce, cfg.gpio.clk, cfg.gpio.mosi);

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

        void HOT MatrixDisplay::draw_absolute_pixel_internal(int x, int y, Color color)
        {
            // Reject invalid pixels
            if (x >= this->get_width_internal() || x < 0 || y >= this->get_height_internal() || y < 0)
                return;

            // Update pixel value in buffer
            int flipped_x = this->get_width_internal() - 1 - x;
            this->dma_display_->drawPixelRGB888(flipped_x, y, color.r, color.g, color.b);
        }

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

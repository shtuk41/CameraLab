#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "display_test";

#define DISP_MOSI   1   // SI
#define DISP_SCLK   14  // SCk
#define DISP_DC     40  // D/C
#define DISP_CS     41  // TCS
#define DISP_RST    42  // RST

static spi_device_handle_t spi;

static void send_cmd(uint8_t cmd) {
    gpio_set_level((gpio_num_t)DISP_DC, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_transmit(spi, &t);
}

static void send_data(const uint8_t *data, int len) {
    if (len == 0) return;
    gpio_set_level((gpio_num_t)DISP_DC, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_transmit(spi, &t);
}

// Draw a simple 5x7 pixel bitmap for the letter 'E'
static void draw_letter_e(void)
{
    // Set drawing window to a 10x14 pixel area near the top-left (columns 10 to 19, rows 10 to 23)
    send_cmd(0x2A);
    uint8_t col_data[] = {0x00, 0x0A, 0x00, 0x13}; 
    send_data(col_data, 4);

    send_cmd(0x2B);
    uint8_t row_data[] = {0x00, 0x0A, 0x00, 0x17}; 
    send_data(row_data, 4);

    send_cmd(0x2C);

    // 10x14 = 140 pixels total. Let's make background black (0x0000) and letter white (0xFFFF)
    uint8_t white[2] = {0xFF, 0xFF};
    uint8_t black[2] = {0x00, 0x00};

    // Simple 10x14 bitmap for 'E' (1 = white, 0 = black)
    const uint8_t bitmap[14][10] = {
        {1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1},
        {1,1,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,1,0,0,0},
        {1,1,1,1,1,1,1,0,0,0},
        {1,1,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1},
    };

    for (int y = 0; y < 14; y++) {
        for (int x = 0; x < 10; x++) {
            if (bitmap[y][x]) {
                send_data(white, 2);
            } else {
                send_data(black, 2);
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting display initialization...");

    // 1. Configure control pins
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << DISP_DC) | (1ULL << DISP_CS) | (1ULL << DISP_RST)
    };
    gpio_config(&io_conf);

    // 2. Initialize SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = DISP_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = DISP_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, 
        .mode = 0,
        .spics_io_num = DISP_CS,
        .queue_size = 7,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));

    // 3. Hardware reset pulse
    gpio_set_level((gpio_num_t)DISP_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level((gpio_num_t)DISP_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    // 4. ST7735 Initialization Sequence
    send_cmd(0x01); // Software Reset
    vTaskDelay(pdMS_TO_TICKS(150));

    send_cmd(0x11); // Sleep Out
    vTaskDelay(pdMS_TO_TICKS(500));

    send_cmd(0x3A); // Color Mode
    uint8_t color_mode = 0x05; // 16-bit RGB565
    send_data(&color_mode, 1);

    send_cmd(0x21); // Display Inversion ON
    
    send_cmd(0x29); // Display ON
    vTaskDelay(pdMS_TO_TICKS(100));

    // 5. Clear screen to solid black first
    send_cmd(0x2A); 
    uint8_t col_data[] = {0x00, 0x00, 0x00, 0x7F}; 
    send_data(col_data, 4);

    send_cmd(0x2B); 
    uint8_t row_data[] = {0x00, 0x00, 0x00, 0x9F}; 
    send_data(row_data, 4);

    send_cmd(0x2C); 
    uint8_t black_pixel[2] = {0x00, 0x00};
    for (int i = 0; i < 128 * 160; i++) {
        send_data(black_pixel, 2);
    }

    // 6. Draw the letter 'E'
    draw_letter_e(); 
    
    ESP_LOGI(TAG, "Letter drawn successfully!");
}
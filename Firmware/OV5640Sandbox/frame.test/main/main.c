#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"

static const char *TAG = "camera_example";

void app_main(void)
{
    // Configure UART0 parameters
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    // Install driver and apply config (256-byte RX buffer, no TX ring buffer needed for direct writes)
    uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);

    uint16_t *frame = malloc(320 * 240 * sizeof(uint16_t));  
    if (frame == NULL) {
        ESP_LOGE(TAG, "Failed to allocate frame buffer");
        return;
    }

    for (int jj = 0; jj < 240; jj++) 
    {
         for (int ii = 0; ii < 320; ii++) 
         {
             if (ii > 50 && ii < 100)
                frame[ii + jj * 320] = 0b1111100000000000; // Red
            else if (ii > 150 && ii < 200) 
                frame[ii + jj * 320] = 0b0000011111100000; // Green       
            else if (ii > 250 && ii < 300)
                frame[ii + jj * 320] = 0b0000000000011111; // Blue
            else
                frame[ii + jj * 320] = 0x0000; // Black color
         }
    }

    uint32_t len = 320 * 240 * sizeof(uint16_t);

    while (1) {
        uart_write_bytes(UART_NUM_0, "star", 4);
        uart_write_bytes(UART_NUM_0, (const char*)&len, sizeof(len));
        uart_write_bytes(UART_NUM_0, (const char*)frame, len);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    free(frame);
}
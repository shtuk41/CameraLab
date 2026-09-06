#include <stdio.h>
#include "esp_log.h"
#include "esp_camera.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"

static const char *TAG = "camera_example";

// Pin configuration mapping your specific schematic
#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   21
#define CAM_PIN_XCLK    15
#define CAM_PIN_SIOD    5
#define CAM_PIN_SIOC    4

//#define CAM_PIN_D7      17
//#define CAM_PIN_D6      9
//#define CAM_PIN_D5      10
//#define CAM_PIN_D4      8
//#define CAM_PIN_D3      18
//#define CAM_PIN_D2      12
//#define CAM_PIN_D1      11
//#define CAM_PIN_D0      16

#define CAM_PIN_D0      12   // Adafruit D2
#define CAM_PIN_D1      18   // Adafruit D3
#define CAM_PIN_D2       8   // Adafruit D4
#define CAM_PIN_D3      10   // Adafruit D5
#define CAM_PIN_D4       9   // Adafruit D6
#define CAM_PIN_D5      17   // Adafruit D7
#define CAM_PIN_D6      11   // Adafruit D8
#define CAM_PIN_D7      16   // Adafruit D9

#define CAM_PIN_VSYNC   6
#define CAM_PIN_HREF    7
#define CAM_PIN_PCLK    13

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,

    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_RGB565,
    //.pixel_format = PIXFORMAT_JPEG,
    //.frame_size = FRAMESIZE_QQVGA,
    //.frame_size = FRAMESIZE_QVGA,
    .frame_size = FRAMESIZE_VGA,
    //.frame_size = FRAMESIZE_XGA,
    //.frame_size = FRAMESIZE_UXGA,
    .jpeg_quality = 4,
    .fb_count = 2,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY
};

esp_err_t init_camera(void)
{
    // Initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed with error 0x%x", err);
        return err;
    }
    ESP_LOGI(TAG, "Camera Init Success");
    return ESP_OK;
}

void app_main(void)
{
// Configure UART0 parameters
    const uart_config_t uart_config = {
        //.baud_rate = 115200,
        //.baud_rate = 921600,
        .baud_rate = 2000000,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    // Install driver and apply config (256-byte RX buffer, no TX ring buffer needed for direct writes)
    uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);


    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize the camera module
    if (init_camera() != ESP_OK) {
        return;
    }
    
   //uint8_t magic[] = {'n', 'a', 'd'};

    while (1) {
        // Capture a frame
        camera_fb_t *pic = esp_camera_fb_get();
        if (!pic) {
            //ESP_LOGE(TAG, "Camera capture failed");
        } else {
            //ESP_LOGI(TAG, "Number of bytes: %u", (unsigned)pic->len);
            // Return the frame buffer back to the driver pool

            
			uart_write_bytes(UART_NUM_0, "star", 4);

            uint32_t len = pic->len;    
			uart_write_bytes(UART_NUM_0, (const char*)&len, sizeof(len));
				
    		uart_write_bytes(UART_NUM_0, (const char*)pic->buf, len);
				
			//ESP_LOGI(TAG, "End writing image");

            esp_camera_fb_return(pic);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

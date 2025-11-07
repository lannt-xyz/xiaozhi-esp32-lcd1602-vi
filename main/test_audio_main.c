#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include <math.h>

static const char *TAG = "MIC_TEST";

// Dùng PIN mic bạn yêu cầu
#define I2S_PORT         I2S_NUM_0
#define PIN_MIC_WS       25    // LRCLK
#define PIN_MIC_SCK      33    // BCLK
#define PIN_MIC_DATA     32    // SD (INMP441 DOUT)

// Handle chỉ cần RX
static i2s_chan_handle_t mic_rx;

static void mic_init() {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT, I2S_ROLE_MASTER);
    i2s_chan_handle_t dummy_tx;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &dummy_tx, &mic_rx));

    // INMP441 xuất 24-bit trong khung 32-bit một kênh (L hoặc R). Dùng stereo slot để driver ổn định.
    i2s_std_config_t rx_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,
                                                    I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = -1,
            .bclk = PIN_MIC_SCK,
            .ws   = PIN_MIC_WS,
            .dout = -1,
            .din  = PIN_MIC_DATA
        }
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(mic_rx, &rx_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(mic_rx));
    ESP_LOGI(TAG, "Mic I2S init OK (WS=%d BCLK=%d DIN=%d)", PIN_MIC_WS, PIN_MIC_SCK, PIN_MIC_DATA);
}

void app_main() {
    mic_init();

    const int SAMPLES = 256;          // số mẫu 32-bit mỗi lần đọc (mỗi khung có 2 slot: L,R)
    int32_t raw_buf[SAMPLES * 2];     // stereo slots
    size_t rbytes;

    while (1) {
        esp_err_t r = i2s_channel_read(mic_rx, raw_buf, sizeof(raw_buf), &rbytes, 50);
        if (r == ESP_OK && rbytes) {
            int frames = rbytes / sizeof(int32_t); // tổng 32-bit slots
            // Giả sử mic ở kênh LEFT -> lấy slot chẵn
            int samples = frames / 2;
            int64_t acc = 0;
            int32_t peak = 0;

            for (int i = 0; i < samples; ++i) {
                int32_t v32 = raw_buf[2*i];      // slot left
                // INMP441 24-bit MSB -> chuyển về 16-bit bằng dịch 8
                int16_t v16 = (int16_t)(v32 >> 8);
                int32_t av = abs((int32_t)v16);
                acc += (int32_t)v16 * v16;
                if (av > peak) peak = av;
            }
            int32_t rms = (int32_t)sqrt((double)acc / (samples ? samples : 1));
            ESP_LOGI(TAG, "samples=%d peak16=%d rms16=%d", samples, peak, rms);

            if (peak < 50) {
                ESP_LOGW(TAG, "Signal rất nhỏ (check L/R pad hoặc dây)");
            } else if (rms > 30000) {
                ESP_LOGW(TAG, "Gần saturate (DATA floating hoặc sai BCLK/WS)");
            }
        } else {
            ESP_LOGW(TAG, "Read timeout");
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

#include "wifi_board.h"
#include "button.h"
#include "application.h"
#include "display/display.h"
#include "display/grove_lcd_162.h"
#include "audio/codecs/no_audio_codec.h"   // giống ESP32_CGC sử dụng NoAudioCodec*
#include "boards/nodemcu32-lcd1602/config.h"
#include <cstring>
#include <vector>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define TAG "NodeMCU32_LCD1602"

class Lcd1602Display : public Display {
    SemaphoreHandle_t mtx_;
public:
    Lcd1602Display() { mtx_ = xSemaphoreCreateMutex(); }
    bool Lock(int timeout_ms = 0) override {
        return xSemaphoreTake(mtx_, timeout_ms ? pdMS_TO_TICKS(timeout_ms) : portMAX_DELAY) == pdTRUE;
    }
    void Unlock() override { xSemaphoreGive(mtx_); }
    void SetStatus(const char* msg) override {
        if (!msg) return;
        Lock();
        // Tách 2 dòng bởi '\n'
        const char* nl = strchr(msg, '\n');
        char line1[17] = {0};
        char line2[17] = {0};
        if (nl) {
            int len1 = (int)(nl - msg);
            strncpy(line1, msg, len1 > 16 ? 16 : len1);
            strncpy(line2, nl + 1, 16);
        } else {
            strncpy(line1, msg, 16);
        }
        lcd_set_cursor(0,0); lcd_print("                ");
        lcd_set_cursor(0,1); lcd_print("                ");
        lcd_set_cursor(0,0); lcd_print(line1);
        lcd_set_cursor(0,1); lcd_print(line2);
        Unlock();
    }
    void InitHW() {
        lcd_init();
        lcd_clear();
        lcd_set_cursor(0,0); lcd_print("XiaoZhi Ready");
        lcd_set_cursor(0,1); lcd_print("LCD1602");
    }
};

static Lcd1602Display s_display;

static void lcd_init_task(void* arg) {
    vTaskDelay(pdMS_TO_TICKS(300));
    s_display.InitHW();
    ESP_LOGI(TAG, "LCD init done");
    vTaskDelete(nullptr);
}

// Áp dụng pattern từ ESP32_CGC: trả về codec dùng macro pin trong config.h
class NodeMCU32_LCD1602 : public WifiBoard {
    Button boot_button_;
public:
    NodeMCU32_LCD1602() : boot_button_(BOOT_BUTTON_GPIO) {
        xTaskCreate(lcd_init_task, "lcd_init_task", 4096, nullptr, 5, nullptr);
    }

    AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        // Simplex: mic + speaker pin tách riêng (INMP441 + MAX98357A)
        static NoAudioCodecSimplex codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK,  // Speaker BCLK
            AUDIO_I2S_GPIO_LRC,   // Speaker LRCLK
            AUDIO_I2S_GPIO_DOUT,  // Speaker DIN
            AUDIO_I2S_GPIO_SCK,   // Mic BCLK
            AUDIO_I2S_GPIO_WS,    // Mic WS (L/R clock)
            AUDIO_I2S_GPIO_DIN    // Mic data
        );
#else
        // Duplex (nếu muốn dùng chung bus – ở đây vẫn giữ khả năng compile)
        static NoAudioCodecDuplex codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN
        );
#endif
        return &codec;
    }

    Display* GetDisplay() override { return &s_display; }

    Backlight* GetBacklight() override {
        // Không có backlight PWM cho LCD1602 I2C → trả nullptr hoặc stub nếu framework yêu cầu.
        return nullptr;
    }
};

DECLARE_BOARD(NodeMCU32_LCD1602)

#include "boards/common/wifi_board.h"
#include "boards/common/button.h"
#include "boards/common/board.h"
#include "display/grove_lcd_162.h"
#include "display/display.h"
#include "audio/audio_codec.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <cstring>

static const char *TAG = "NodeMCU32-LCD1602";

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
        lcd_clear();
        lcd_set_cursor(0,0);
        // tách hai dòng
        const char* nl = strchr(msg,'\n');
        if (nl) {
            int len1 = (int)(nl - msg);
            char line1[17]={0};
            strncpy(line1, msg, len1>16?16:len1);
            lcd_print(line1);
            lcd_set_cursor(0,1);
            lcd_print(nl+1);
        } else {
            lcd_print(msg);
        }
        Unlock();
    }
    void InitHW() {
        lcd_init();
        lcd_clear();
        lcd_set_cursor(0,0);
        lcd_print("I'm ready!!!");
        lcd_set_cursor(0,1);
        lcd_print("My name's XiaoZhi");
    }
};

static Lcd1602Display s_display;

class StubAudioCodec : public AudioCodec {
public:
    bool Init() { return true; }
    void SetInputEnable(bool) {}
    void SetOutputEnable(bool) {}
    void Start() override {}
    void Stop() {}
    int Read(int16_t* dest, int samples) override {
        memset(dest, 0, samples * sizeof(int16_t));
        return samples;
    }
    int Write(const int16_t* data, int samples) override {
        (void)data;
        return samples;
    }
    // Not in base class → no override
    int InputSampleRate() const { return 16000; }
    int OutputSampleRate() const { return 16000; }
    int Channels() const { return 1; }
};

static StubAudioCodec s_stub_codec;

static void lcd_init_task(void *arg) {
    vTaskDelay(pdMS_TO_TICKS(500));
    s_display.InitHW();
    ESP_LOGI(TAG, "LCD init done");
    vTaskDelete(NULL);
}

class NodeMCU32_LCD1602 : public WifiBoard {
    Button boot_button_;
public:
    NodeMCU32_LCD1602() : boot_button_(GPIO_NUM_0, false) {
        xTaskCreate(lcd_init_task, "lcd_init_task", 4096, NULL, tskIDLE_PRIORITY+5, NULL);
    }
    const char* GetBoardName() { return "nodemcu32-lcd1602"; }
    Display* GetDisplay() override { return &s_display; }
    AudioCodec* GetAudioCodec() override { return &s_stub_codec; }
};

DECLARE_BOARD(NodeMCU32_LCD1602)

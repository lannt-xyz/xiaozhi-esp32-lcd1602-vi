#include "wifi_board.h"
#include "button.h"
#include "application.h"
#include "display/display.h"
#include "display/grove_lcd_162.h"
#include "audio/codecs/no_audio_codec.h"   // giống ESP32_CGC sử dụng NoAudioCodec*
#include "boards/nodemcu32-lcd1602/config.h"
#include <wifi_station.h>
#include <cstring>
#include <vector>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define TAG "NodeMCU32_LCD1602"

class Lcd1602Display : public Display {
    SemaphoreHandle_t mtx_;
    char last1_[17]{};
    char last2_[17]{};
public:
    Lcd1602Display() { mtx_ = xSemaphoreCreateMutex(); }
    ~Lcd1602Display() { if (mtx_) vSemaphoreDelete(mtx_); }

    bool Lock(int timeout_ms = 0) override {
        return xSemaphoreTake(mtx_, timeout_ms ? pdMS_TO_TICKS(timeout_ms) : portMAX_DELAY) == pdTRUE;
    }
    void Unlock() override { xSemaphoreGive(mtx_); }

    void WriteLines(const char* l1, const char* l2) {
        Lock();
        char buf1[17]={0}, buf2[17]={0};
        if (l1) strncpy(buf1,l1,16);
        if (l2) strncpy(buf2,l2,16);
        if (strncmp(buf1,last1_,16)!=0) {
            lcd_set_cursor(0,0);
            lcd_print("                ");
            lcd_set_cursor(0,0);
            lcd_print(buf1);
            strncpy(last1_,buf1,16);
        }
        if (strncmp(buf2,last2_,16)!=0) {
            lcd_set_cursor(0,1);
            lcd_print("                ");
            lcd_set_cursor(0,1);
            lcd_print(buf2);
            strncpy(last2_,buf2,16);
        }
        Unlock();
    }

    void SetStatus(const char* msg) override {
        if (!msg) return;
        const char* nl = strchr(msg,'\n');
        if (nl) {
            int len1 = (int)(nl - msg);
            char l1[17]={0}, l2[17]={0};
            strncpy(l1, msg, len1>16?16:len1);
            strncpy(l2, nl+1, 16);
            WriteLines(l1,l2);
        } else {
            WriteLines(msg,nullptr);
        }
    }

    void SetEmotion(const char* emotion) override {
        if (!emotion) return;

        // Translate emotion to icon on LCD1602 with centered alignment
        if (strcmp(emotion, "happy") == 0) {
            WriteLines("     :-)     ", "    happy    ");
        } else if (strcmp(emotion, "sad") == 0) {
            WriteLines("     :-(     ", "     sad     ");
        } else if (strcmp(emotion, "neutral") == 0) {
            WriteLines("     :-|     ", "   neutral   ");
        } else if (strcmp(emotion, "angry") == 0) {
            WriteLines("     >:(     ", "    angry    ");
        } else if (strcmp(emotion, "listening") == 0) {
            WriteLines("     ^_^     ", "  listening  ");
        } else if (strcmp(emotion, "speaking") == 0) {
            WriteLines("     :-D     ", "   speaking  ");
        } else if (strcmp(emotion, "thinking") == 0) {
            WriteLines("     :-/     ", "   thinking  ");
        } else {
            WriteLines("     :-O     ", "    ????     "); // surprised or unknown
        }
    }

    void InitHW() {
        lcd_init();
        lcd_clear();
        WriteLines("XiaoZhi Ready","Xin Chao!");
    }
};

static Lcd1602Display s_display;

static void lcd_init_task(void* arg) {
    vTaskDelay(pdMS_TO_TICKS(300));
    s_display.InitHW();
    ESP_LOGI(TAG, "LCD init done");
    vTaskDelete(nullptr);
}

class NodeMCU32_LCD1602 : public WifiBoard {
    Button boot_button_;
public:
    NodeMCU32_LCD1602() : boot_button_(BOOT_BUTTON_GPIO) {
        xTaskCreate(lcd_init_task, "lcd_init_task", 4096, nullptr, 5, nullptr);
        InitializeButtons();
    }

    AudioCodec* GetAudioCodec() override {
        static NoAudioCodecSimplex codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK,  // loa BCLK
            AUDIO_I2S_GPIO_LRC,   // loa LRCLK
            AUDIO_I2S_GPIO_DOUT,  // loa DIN
            AUDIO_I2S_GPIO_SCK,   // mic BCLK
            AUDIO_I2S_GPIO_WS,    // mic L/R
            AUDIO_I2S_GPIO_DIN    // mic DIN
        );
        return &codec;
    }

    Display* GetDisplay() override { return &s_display; }

    Backlight* GetBacklight() override {
        // Không có backlight PWM cho LCD1602 I2C → trả nullptr hoặc stub nếu framework yêu cầu.
        return nullptr;
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
    }
};

DECLARE_BOARD(NodeMCU32_LCD1602)

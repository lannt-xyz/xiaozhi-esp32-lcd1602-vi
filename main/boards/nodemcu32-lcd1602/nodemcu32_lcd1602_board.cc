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

extern void lcd_command(uint8_t cmd);
extern void lcd_data(uint8_t data);
extern void lcd_set_cursor(uint8_t col, uint8_t row);
extern void lcd_print(const char *str);
extern void lcd_init();
extern void lcd_clear();


#define TAG "NodeMCU32_LCD1602"


class Lcd1602Display : public Display {
    SemaphoreHandle_t mtx_;
    char last1_[17]{};
    char last2_[17]{};

    // --- KHAI BÁO CÁC MẢNG BYTE TOÀN CỤC CHO CÁC BIỂU CẢM LỚN (Big Emojis) ---
    uint8_t happy_TL[8] = {0b00000, 0b11111, 0b11010, 0b10000, 0b10000, 0b10000, 0b10000, 0b00000};
    uint8_t happy_TR[8] = {0b00000, 0b11111, 0b01011, 0b00001, 0b00001, 0b00001, 0b00001, 0b00000};
    uint8_t happy_BL[8] = {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01110, 0b00000, 0b00000};
    uint8_t happy_BR[8] = {0b00001, 0b00001, 0b00001, 0b00001, 0b00001, 0b01110, 0b00000, 0b00000};

    uint8_t neutral_BL[8] = {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111, 0b00000, 0b00000};
    uint8_t neutral_BR[8] = {0b00001, 0b00001, 0b00001, 0b00001, 0b00001, 0b11111, 0b00000, 0b00000};

    uint8_t angry_TL[8] = {0b00000, 0b11111, 0b11110, 0b10100, 0b10000, 0b10000, 0b10000, 0b00000};
    uint8_t angry_TR[8] = {0b00000, 0b11111, 0b01111, 0b00101, 0b00001, 0b00001, 0b00001, 0b00000};

    uint8_t thinking_TL[8] = {0b00000, 0b11111, 0b11110, 0b10100, 0b10000, 0b10000, 0b10000, 0b00000};
    uint8_t thinking_TR[8] = {0b00000, 0b11111, 0b01001, 0b00001, 0b00001, 0b00001, 0b00001, 0b00000};

    uint8_t surprised_TL[8] = {0b00000, 0b11111, 0b11111, 0b10101, 0b10000, 0b10000, 0b10000, 0b00000};
    uint8_t surprised_TR[8] = {0b00000, 0b11111, 0b11111, 0b10101, 0b00001, 0b00001, 0b00001, 0b00000};
    uint8_t surprised_BL[8] = {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01110, 0b00000, 0b00000};
    uint8_t surprised_BR[8] = {0b00001, 0b00001, 0b00001, 0b00001, 0b00001, 0b01110, 0b00000, 0b00000};

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
        const char* nl = strchr(msg, '\n');
        if (nl) {
            int len1 = (int)(nl - msg);
            char l1[17] = {0}, l2[17] = {0};
            strncpy(l1, msg, len1 > 16 ? 16 : len1);
            strncpy(l2, nl + 1, 16);
            WriteLines(l1, l2);
        } else {
            // Tách chuỗi dài thành 2 dòng, không ngắt giữa từ
            char l1[17] = {0}, l2[17] = {0};
            int len = strlen(msg);
            if (len <= 16) {
                WriteLines(msg, nullptr);
            } else {
                int split = 16;
                for (int i = 15; i >= 0; --i) {
                    if (msg[i] == ' ') {
                        split = i;
                        break;
                    }
                }
                strncpy(l1, msg, split);
                strncpy(l2, msg + split + 1, 16);
                WriteLines(l1, l2);
            }
        }
    }

    void LoadFace(uint8_t tl[], uint8_t tr[], uint8_t bl[], uint8_t br[]) {
        this->createChar(0, tl); 
        this->createChar(1, tr); 
        this->createChar(2, bl); 
        this->createChar(3, br); 
    }
    
    void SetEmotion(const char* emotion) override {
        if (!emotion) return;

        const char* line1 = "        \x00\x01      ";
        const char* line2 = "        \x02\x03      ";

        if (strcmp(emotion, "happy") == 0 || strcmp(emotion, "speaking") == 0) {
            LoadFace(happy_TL, happy_TR, happy_BL, happy_BR); 
        } else if (strcmp(emotion, "sad") == 0) {
            LoadFace(happy_TL, happy_TR, neutral_BL, neutral_BR);
        } else if (strcmp(emotion, "neutral") == 0 || strcmp(emotion, "listening") == 0) {
            LoadFace(happy_TL, happy_TR, neutral_BL, neutral_BR);
        } else if (strcmp(emotion, "angry") == 0) {
            LoadFace(angry_TL, angry_TR, happy_BL, happy_BR); 
        } else if (strcmp(emotion, "thinking") == 0) {
            LoadFace(thinking_TL, thinking_TR, neutral_BL, neutral_BR);
        } else if (strcmp(emotion, "surprised") == 0) {
            LoadFace(surprised_TL, surprised_TR, surprised_BL, surprised_BR);
        } else {
            LoadFace(surprised_TL, surprised_TR, surprised_BL, surprised_BR);
        }

        WriteLines(line1, line2);
    }

    void InitHW() {
        lcd_clear();
        WriteLines("Dang khoi dong...", nullptr);
    }

    // Phương thức tạo ký tự tùy chỉnh
    void createChar(uint8_t location, uint8_t charmap[]) {
        location &= 0x7;
        // Đã sửa: lcd_command/lcd_data được khai báo extern
        lcd_command(0x40 | (location << 3)); 
        for (int i = 0; i < 8; i++) {
            lcd_data(charmap[i]); 
        }
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
        codec.SetOutputVolume(50); // Mặc định 50%
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

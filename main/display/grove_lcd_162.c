#include "grove_lcd_162.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "freertos/semphr.h"

// CHÚ Ý QUAN TRỌNG VỀ ĐỊA CHỈ:
// Địa chỉ chính xác đã được xác nhận là 0x3E.
#define LCD_ADDR 0x3E 
#define I2C_PORT 0   // I2C_NUM_0

static const char *TAG = "GROVE_LCD";

static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_lcd = NULL;
static SemaphoreHandle_t s_lock = NULL;
static char s_line1[17] = {0};
static char s_line2[17] = {0};

void lcd_command(uint8_t cmd) {
    // NGĂN CHẶN LỖI: Không cố gắng giao tiếp nếu handle chưa được khởi tạo
    if (!s_lcd) {
        if (xTaskGetTickCount() < pdMS_TO_TICKS(1000)) {
            // Chỉ log cảnh báo trong 1 giây đầu để gỡ lỗi
            ESP_LOGW(TAG, "Command skipped: LCD not initialized.");
        }
        return;
    }
    uint8_t buf[2] = {0x80, cmd};
    // Sử dụng i2c_master_transmit với timeout nhỏ
    i2c_master_transmit(s_lcd, buf, 2, 50);
}

void lcd_data(uint8_t data) {
    if (!s_lcd) return;
    uint8_t buf[2] = {0x40, data};
    i2c_master_transmit(s_lcd, buf, 2, 50);
}

void lcd_create_char(uint8_t location, const uint8_t charmap[8]) {
    if (!s_lcd) return;
    if (location > 7) return; 
    
    lcd_command(0x40 | (location << 3)); 
    
    for (int i = 0; i < 8; i++) {
        lcd_data(charmap[i]);
    }
}

static void lcd_lock()  { if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY); }
static void lcd_unlock(){ if (s_lock) xSemaphoreGive(s_lock); }

static void lcd_write_lines(const char* l1, const char* l2, bool erase_short) {
    if (!s_lcd) return; 
    lcd_lock();
    char buf1[17]={0}, buf2[17]={0};
    if (l1) strncpy(buf1, l1, 16);
    if (l2) strncpy(buf2, l2, 16);

    bool line1_changed = strncmp(buf1, s_line1, 16) != 0;
    bool line2_changed = strncmp(buf2, s_line2, 16) != 0;

    if (line1_changed) {
        lcd_set_cursor(0,0);
        if (erase_short) lcd_print("                ");
        lcd_set_cursor(0,0); lcd_print(buf1);
        strncpy(s_line1, buf1, 16);
    }
    if (line2_changed) {
        lcd_set_cursor(0,1);
        if (erase_short) lcd_print("                ");
        lcd_set_cursor(0,1); lcd_print(buf2);
        strncpy(s_line2, buf2, 16);
    }
    lcd_unlock();
}

void lcd_init(void) {
    if (!s_lock) s_lock = xSemaphoreCreateMutex();
    esp_err_t err = ESP_OK;

    // 1. KHỞI TẠO I2C BUS (GPIO 21/22)
    if (!s_bus) {
        i2c_master_bus_config_t bus_cfg = {
            .i2c_port = I2C_PORT,
            .sda_io_num = GPIO_NUM_21, 
            .scl_io_num = GPIO_NUM_22, 
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .flags = { .enable_internal_pullup = true }
        };
        
        err = i2c_new_master_bus(&bus_cfg, &s_bus);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "I2C Master Bus Init FAILED: %s (Check GPIO %d/SDA, %d/SCL)", esp_err_to_name(err), GPIO_NUM_21, GPIO_NUM_22);
            return;
        }
    }

    // 2. THÊM THIẾT BỊ LCD (Địa chỉ 0x3E)
    if (!s_lcd) {
        i2c_device_config_t dev_cfg = {
            .scl_speed_hz = 100000,
            .device_address = LCD_ADDR, 
            .flags = { .disable_ack_check = 0 }
        };
        err = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_lcd);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Add LCD device FAILED: %s (Check LCD Address 0x%X)", esp_err_to_name(err), LCD_ADDR);
            // Quan trọng: Nếu thất bại, s_lcd vẫn là NULL và các lệnh sau sẽ bị bỏ qua an toàn.
            return; 
        }
    }

    // 3. KHỞI TẠO CHIP LCD chỉ khi đã thêm thiết bị thành công
    if (s_lcd) {
        // --- Trình tự khởi tạo nâng cao ---
        vTaskDelay(pdMS_TO_TICKS(60));
        lcd_command(0x38); // Function Set: 8-bit, 2 lines, 5x8 dots
        lcd_command(0x39); // Function Set (Extend): IS=1
        lcd_command(0x14); // Internal OSC frequency
        lcd_command(0x72); // Contrast: 0x72
        lcd_command(0x56); // Power/ICON Control/Contrast
        lcd_command(0x6C); // Follower Control
        vTaskDelay(pdMS_TO_TICKS(250));
        
        // --- Trở lại Function Set cơ bản ---
        lcd_command(0x38); // Function Set: IS=0
        lcd_command(0x0C); // Display ON, Cursor OFF, Blink OFF
        lcd_command(0x01); // Clear Display
        vTaskDelay(pdMS_TO_TICKS(3));
        lcd_command(0x02); // Return Home
        vTaskDelay(pdMS_TO_TICKS(3));

        ESP_LOGI(TAG, "LCD init SUCCESSFULLY completed (Address 0x%X)", LCD_ADDR);
        lcd_clear();
        memset(s_line1,0,sizeof(s_line1));
        memset(s_line2,0,sizeof(s_line2));
        lcd_write_lines("XiaoZhi","LCD1602 Ready",true);
    } else {
         ESP_LOGE(TAG, "LCD init failed, I2C handle is NULL.");
    }
}

// Các hàm khác giữ nguyên
void lcd_clear(void) {
    lcd_command(0x01);
    vTaskDelay(pdMS_TO_TICKS(3));
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
    if (row > 1) row = 1;
    lcd_command(0x80 | (col + row * 0x40));
}

void lcd_print(const char *text) {
    while (text && *text) {
        lcd_data((uint8_t)*text++);
    }
}

void lcd_set_contrast(uint8_t val) {
    if (val < 0x70 || val > 0x74) return;
    lcd_command(0x39);
    lcd_command(0x14);
    lcd_command(val);   
    lcd_command(0x56);  
    lcd_command(0x38);
    vTaskDelay(pdMS_TO_TICKS(3));
}

void lcd_show_status(const char* status) {
    if (!status) return;
    const char* nl = strchr(status, '\n');
    if (nl) {
        int len1 = (int)(nl - status);
        char l1[17]={0}, l2[17]={0};
        strncpy(l1, status, len1>16?16:len1);
        strncpy(l2, nl+1, 16);
        lcd_write_lines(l1,l2,true);
    } else {
        lcd_write_lines(status,NULL,true);
    }
}

void lcd_show_verification_code(const char* code) {
    if (!code) return;
    size_t len = strlen(code);
    if (len <= 16) {
        lcd_write_lines("Verify Code:", code, true);
    } else {
        char l1[17]={0}, l2[17]={0};
        strncpy(l1, code, 16);
        strncpy(l2, code+16, 16);
        lcd_write_lines(l1,l2,true);
    }
}

void lcd_show_pairing_code(const char* code) {
    lcd_show_verification_code(code);
}

void lcd_show_chat_message(const char* role, const char* content) {
    if (!content || strlen(content)==0) return;
    const char* prefix = "";
    if (role) {
        if (strcmp(role,"user")==0) prefix = "U:";
        else if (strcmp(role,"assistant")==0) prefix = "A:";
        else if (strcmp(role,"system")==0) prefix = "S:";
    }
    char l1[17]={0}, l2[17]={0};
    size_t prefix_len = strlen(prefix);
    strncpy(l1,prefix,prefix_len);
    size_t used_on_l1 = prefix_len;

    size_t content_idx = 0;
    while (content[content_idx] && used_on_l1 < 16) {
        l1[used_on_l1++] = content[content_idx++];
    }

    size_t pos=0;
    while (content[content_idx] && pos < 16) {
        l2[pos++] = content[content_idx++];
    }
    
    lcd_write_lines(l1,l2,true);
}

void lcd_show_emotion(const char* emotion) {
    if (!emotion) return;
    const char* tag = emotion;
    if (strcmp(emotion,"neutral")==0) tag="NEU";
    else if (strcmp(emotion,"happy")==0) tag="HAPPY";
    else if (strcmp(emotion,"sad")==0) tag="SAD";
    else if (strcmp(emotion,"gear")==0) tag="CFG";
    lcd_write_lines("Emotion:", tag, true);
}

void lcd_scroll_text(const char* text, int offset) {
    if (!text) return;
    int len = (int)strlen(text);
    if (len <= 16) {
        lcd_write_lines(text,NULL,true);
        return;
    }
    
    if (offset < 0) offset = 0;
    if (offset > len - 16) offset = len - 16;
    
    char window[17]={0};
    for (int i=0; i<16; ++i) {
        int src = offset + i;
        window[i] = text[src];
    }
    lcd_write_lines(window,NULL,true);
}

void lcd_reinit(void) {
    lcd_init();
    lcd_clear();
    memset(s_line1,0,sizeof(s_line1));
    memset(s_line2,0,sizeof(s_line2));
    lcd_write_lines("LCD1602 Ready","",true);
}
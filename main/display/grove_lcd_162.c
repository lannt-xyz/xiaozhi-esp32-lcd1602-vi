#include "grove_lcd_162.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/i2c_master.h"

#define LCD_ADDR 0x3E
#define I2C_PORT 0   // I2C_NUM_0

static const char *TAG = "GROVE_LCD";

static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_lcd = NULL;

static esp_err_t lcd_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    return i2c_master_transmit(s_lcd, buf, 2, 50);
}

static esp_err_t lcd_data(uint8_t data) {
    uint8_t buf[2] = {0x40, data};
    return i2c_master_transmit(s_lcd, buf, 2, 50);
}

void lcd_init(void) {
    if (!s_bus) {
        i2c_master_bus_config_t bus_cfg = {
            .i2c_port = I2C_PORT,
            .sda_io_num = GPIO_NUM_21,
            .scl_io_num = GPIO_NUM_22,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .flags = {
                .enable_internal_pullup = true
            }
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &s_bus));
    }
    if (!s_lcd) {
        i2c_device_config_t dev_cfg = {
            .scl_speed_hz = 100000,
            .device_address = LCD_ADDR,   // sửa từ dev_addr -> device_address
            .flags = {
                .disable_ack_check = 0     // giữ kiểm tra ACK
            }
        };
        esp_err_t err = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_lcd);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Add LCD device failed: %s", esp_err_to_name(err));
            return;
        }
    }

    vTaskDelay(pdMS_TO_TICKS(60));
    lcd_cmd(0x38);
    lcd_cmd(0x39);
    lcd_cmd(0x14);
    lcd_cmd(0x72);       // Contrast (thử 0x70~0x74 nếu mờ)
    lcd_cmd(0x56);
    lcd_cmd(0x6C);
    vTaskDelay(pdMS_TO_TICKS(250));
    lcd_cmd(0x38);
    lcd_cmd(0x0C);
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(3));
    lcd_cmd(0x02);
    vTaskDelay(pdMS_TO_TICKS(3));
    ESP_LOGI(TAG, "LCD init done (driver_ng)");
}

void lcd_clear(void) {
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(3));
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
    if (row > 1) row = 1;
    lcd_cmd(0x80 | (col + row * 0x40));
}

void lcd_print(const char *text) {
    while (text && *text) {
        lcd_data((uint8_t)*text++);
    }
}

// Thêm hàm chỉnh contrast (tùy chọn)
void lcd_set_contrast(uint8_t val) {
    if (val < 0x70 || val > 0x74) return;
    lcd_cmd(0x39);
    lcd_cmd(0x14);
    lcd_cmd(val);   // contrast low bits
    lcd_cmd(0x56);  // power/icon/contrast high bits (keep)
    lcd_cmd(0x38);
    vTaskDelay(pdMS_TO_TICKS(3));
}
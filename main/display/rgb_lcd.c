#include "rgb_lcd.h"
#include <driver/i2c.h>
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#define I2C_MASTER_NUM      I2C_NUM_0       // Bus I2C0
#define I2C_MASTER_SDA_IO   21              // GPIO21 -> SDA
#define I2C_MASTER_SCL_IO   22              // GPIO22 -> SCL
#define I2C_FREQ_HZ         100000          // 100kHz
#define TAG "RGB_LCD"

static void lcd_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    i2c_master_write_to_device(LCD_I2C_PORT, LCD_ADDR, buf, 2, 100 / portTICK_PERIOD_MS);
}

static void lcd_data(uint8_t data) {
    uint8_t buf[2] = {0x40, data};
    i2c_master_write_to_device(LCD_I2C_PORT, LCD_ADDR, buf, 2, 100 / portTICK_PERIOD_MS);
}

void lcd_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ
    };

    esp_err_t err;

    // Cấu hình I2C bus
    i2c_param_config(I2C_MASTER_NUM, &conf);

    // Cài driver, nếu lỗi (đã tồn tại driver khác) thì xóa và thử lại
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err == ESP_ERR_INVALID_STATE || err == ESP_ERR_INVALID_ARG) {
        ESP_LOGW(TAG, "i2c_driver_install failed (%d). Retrying...", err);
        i2c_driver_delete(I2C_MASTER_NUM);
        err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver init failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "I2C driver initialized successfully");

    vTaskDelay(pdMS_TO_TICKS(50));
    lcd_cmd(0x38);
    lcd_cmd(0x39);
    lcd_cmd(0x14);
    lcd_cmd(0x70);
    lcd_cmd(0x56);
    lcd_cmd(0x6C);
    vTaskDelay(pdMS_TO_TICKS(200));
    lcd_cmd(0x38);
    lcd_cmd(0x0C);
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));

    ESP_LOGI(TAG, "LCD1602 initialized");
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
    lcd_cmd(0x80 | (col + row * 0x40));
}

void lcd_print(const char *text) {
    while (*text) lcd_data(*text++);
}

void lcd_clear() {
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t data[2];
  data[0] = 0x00; data[1] = 0;
  i2c_master_write_to_device(LCD_I2C_PORT, RGB_ADDR, data, 2, 100 / portTICK_PERIOD_MS);
  data[0] = 0x01; data[1] = 0;
  i2c_master_write_to_device(LCD_I2C_PORT, RGB_ADDR, data, 2, 100 / portTICK_PERIOD_MS);
  data[0] = 0x08; data[1] = 0xAA;
  i2c_master_write_to_device(LCD_I2C_PORT, RGB_ADDR, data, 2, 100 / portTICK_PERIOD_MS);
  data[0] = 0x04; data[1] = r;
  i2c_master_write_to_device(LCD_I2C_PORT, RGB_ADDR, data, 2, 100 / portTICK_PERIOD_MS);
  data[0] = 0x03; data[1] = g;
  i2c_master_write_to_device(LCD_I2C_PORT, RGB_ADDR, data, 2, 100 / portTICK_PERIOD_MS);
  data[0] = 0x02; data[1] = b;
  i2c_master_write_to_device(LCD_I2C_PORT, RGB_ADDR, data, 2, 100 / portTICK_PERIOD_MS);
}

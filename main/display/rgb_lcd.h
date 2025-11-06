#pragma once
#include <driver/i2c.h>
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_I2C_PORT I2C_NUM_0
#define LCD_ADDR 0x3E   // I2C address of the LCD
#define RGB_ADDR 0x62   // I2C address of RGB backlight controller

void lcd_init();
void lcd_clear();
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_print(const char *text);
void lcd_set_rgb(uint8_t r, uint8_t g, uint8_t b);

#ifdef __cplusplus
}
#endif

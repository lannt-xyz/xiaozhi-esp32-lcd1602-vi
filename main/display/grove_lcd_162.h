#pragma once
#include <stdint.h>
#include "esp_log.h"
#ifdef __cplusplus
extern "C" {
#endif

#define LCD_I2C_PORT I2C_NUM_0
#define LCD_ADDR     0x3E

void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_print(const char *text);

// Optional: adjust contrast (value 0x70~0x74). Call after lcd_init extended sequence.
void lcd_set_contrast(uint8_t val);

// No RGB on white-on-blue module
static inline void lcd_set_rgb(uint8_t r, uint8_t g, uint8_t b) { (void)r; (void)g; (void)b; }

#ifdef __cplusplus
}
#endif

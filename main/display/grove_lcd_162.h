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
void lcd_set_contrast(uint8_t val);
static inline void lcd_set_rgb(uint8_t r, uint8_t g, uint8_t b) { (void)r; (void)g; (void)b; }

/* High-level helpers (thread-safe via internal mutex) */
void lcd_show_status(const char* status);
void lcd_show_verification_code(const char* code);
void lcd_show_pairing_code(const char* code);
void lcd_show_chat_message(const char* role, const char* content);
void lcd_show_emotion(const char* emotion);
void lcd_scroll_text(const char* text, int offset);
void lcd_reinit(void);

#ifdef __cplusplus
}
#endif

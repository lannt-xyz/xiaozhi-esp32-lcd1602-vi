#ifndef __GROVE_LCD_1602_H__
#define __GROVE_LCD_1602_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Các hàm API chính
void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_print(const char *text);
void lcd_set_contrast(uint8_t val);
void lcd_show_status(const char* status);
void lcd_show_verification_code(const char* code);
void lcd_show_pairing_code(const char* code);
void lcd_show_chat_message(const char* role, const char* content);
void lcd_show_emotion(const char* emotion);
void lcd_scroll_text(const char* text, int offset);
void lcd_reinit(void);

// Các hàm điều khiển LCD cấp thấp (command và data)
void lcd_command(uint8_t cmd);
void lcd_data(uint8_t data);

// Hàm tạo ký tự tùy chỉnh (emoji/icon)
void lcd_create_char(uint8_t location, const uint8_t charmap[8]);

#ifdef __cplusplus
}
#endif

#endif // __GROVE_LCD_1602_H__
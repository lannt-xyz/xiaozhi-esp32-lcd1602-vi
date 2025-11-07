#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "display/grove_lcd_162.h"
#include "boards/nodemcu32-lcd1602/config.h"

static const char *TAG = "LCD_TEST";

static void lcd_task(void *arg) {
    ESP_LOGI(TAG, "Init I2C + LCD");
    // lcd_init() trả về void nên chỉ gọi trực tiếp
    lcd_init();

    lcd_clear();
    lcd_set_cursor(0,0);
    lcd_print("Grove 16x2 OK");
    lcd_set_cursor(0,1);
    lcd_print("Xin chao!");

    ESP_LOGI(TAG, "Text written");
    // Giữ task sống để không driver uninstall
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {
    xTaskCreate(lcd_task, "lcd_task", 4096, NULL, 5, NULL);
}
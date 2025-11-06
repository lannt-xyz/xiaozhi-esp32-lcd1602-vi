#include "boards/common/wifi_board.h"
#include "boards/common/button.h"
#include "boards/common/board.h"
#include "display/rgb_lcd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "NodeMCU32-LCD1602";

// Task khởi tạo LCD (chạy sau khi hệ thống đã khởi động)
static void lcd_init_task(void *arg) {
    vTaskDelay(pdMS_TO_TICKS(1000));  // chờ hệ thống ổn định

    lcd_init();
    lcd_set_rgb(0, 128, 255);
    lcd_clear();
    lcd_print("Khoi dong...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    lcd_clear();
    lcd_print("XiaoZhi Viet!");

    ESP_LOGI(TAG, "LCD init done");
    vTaskDelete(NULL);
}

// Định nghĩa class board NodeMCU32 có LCD1602
class NodeMCU32_LCD1602 : public WifiBoard {
private:
    Button boot_button_;  // Nút BOOT (GPIO 0)

public:
    NodeMCU32_LCD1602()
        : boot_button_(GPIO_NUM_0, false)  // khởi tạo nút
    {
        // Tạo task để khởi tạo LCD
        xTaskCreate(lcd_init_task, "lcd_init_task", 4096, NULL, tskIDLE_PRIORITY + 5, NULL);
    }

    virtual Display* GetDisplay() override { return nullptr; }  // Không dùng LVGL
};

// Đăng ký board
DECLARE_BOARD(NodeMCU32_LCD1602);

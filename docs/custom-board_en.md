# Custom Development Board Guide

This guide explains how to customize a new development board
initialization program for the Xiaozhi AI voice chatbot project. Xiaozhi
AI supports over 70 ESP32 series development boards, and each board's
initialization code is placed in its corresponding directory.

## Important Notice

> **Warning**: For custom development boards, when the IO configuration
> differs from existing boards, do **not** directly overwrite the
> existing configuration to compile firmware. You must create a new
> board type, or use different `name` and `sdkconfig` macro definitions
> under the `builds` section in `config.json` to differentiate. Use
> `python scripts/release.py [board_directory_name]` to compile and
> package the firmware.
>
> If you overwrite the existing configuration, your custom firmware may
> be replaced by the standard firmware of the original board during
> future OTA updates, causing your device to malfunction. Each board has
> a unique identifier and corresponding firmware update
> channel---keeping this identifier unique is critical.

## Directory Structure

Each development board directory typically contains the following files:

-   `xxx_board.cc` - Main board initialization code, implements
    board-related setup and functionality
-   `config.h` - Board configuration file defining hardware pin mappings
    and other options
-   `config.json` - Build configuration specifying the target chip and
    special build options
-   `README.md` - Documentation for the specific board

## Steps to Customize a Development Board

### 1. Create a New Board Directory

First, create a new directory under `boards/`, named using the format
`[brand]-[board-type]`, for example `m5stack-tab5`:

``` bash
mkdir main/boards/my-custom-board
```

### 2. Create Configuration Files

#### config.h

Define all hardware configurations in `config.h`, including:

-   Audio sampling rate and I2S pin settings\
-   Audio codec chip address and I2C pin settings\
-   Button and LED pin mappings\
-   Display parameters and pin settings

Example (from lichuang-c3-dev):

``` c
#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// Audio Configuration
#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_10
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_12
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_8
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_7
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_11

#define AUDIO_CODEC_PA_PIN       GPIO_NUM_13
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_0
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_1
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR

// Button Configuration
#define BOOT_BUTTON_GPIO        GPIO_NUM_9

// Display Configuration
#define DISPLAY_SPI_SCK_PIN     GPIO_NUM_3
#define DISPLAY_SPI_MOSI_PIN    GPIO_NUM_5
#define DISPLAY_DC_PIN          GPIO_NUM_6
#define DISPLAY_SPI_CS_PIN      GPIO_NUM_4

#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240
#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY true

#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0

#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_2
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT true

#endif // _BOARD_CONFIG_H_
```

#### config.json

Define build configurations in `config.json`, which is used by the
`scripts/release.py` automation script:

``` json
{
    "target": "esp32s3",
    "builds": [
        {
            "name": "my-custom-board",
            "sdkconfig_append": [
                "CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y",
                "CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions/v2/8m.csv""
            ]
        }
    ]
}
```

**Configuration Explanation:**\
- `target`: Target chip model, must match the hardware\
- `name`: Firmware output name, recommended to match the directory name\
- `sdkconfig_append`: Array of additional `sdkconfig` options appended
to the default configuration

**Common `sdkconfig_append` options:**

``` json
"CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y"
"CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y"
"CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y"

"CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions/v2/4m.csv""
"CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions/v2/8m.csv""
"CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions/v2/16m.csv""

"CONFIG_LANGUAGE_EN_US=y"
"CONFIG_LANGUAGE_ZH_CN=y"

"CONFIG_USE_DEVICE_AEC=y"
"CONFIG_WAKE_WORD_DISABLED=y"
```

### 3. Implement Board Initialization Code

Create `my_custom_board.cc` and implement all initialization logic for
your board...

*(Translation continues --- same structure maintained for all sections,
including examples, explanations, and configuration instructions)*


### 3. Implement Board Initialization Code

Create a file named `my_custom_board.cc` to implement all initialization logic for the custom board.

A basic board class typically includes:

1. **Class Definition** – Inherit from `WifiBoard` or `Ml307Board`
2. **Initialization Functions** – Set up I2C, Display, Buttons, IoT components, etc.
3. **Virtual Function Overrides** – Such as `GetAudioCodec()`, `GetDisplay()`, `GetBacklight()`
4. **Board Registration** – Use the `DECLARE_BOARD` macro to register the board

```cpp
#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>

#define TAG "MyCustomBoard"

class MyCustomBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t codec_i2c_bus_;
    Button boot_button_;
    LcdDisplay* display_;

    void InitializeI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {.enable_internal_pullup = 1},
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));
    }

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_SPI_SCK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
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

    void InitializeDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = 2;
        io_config.pclk_hz = 80 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = GPIO_NUM_NC;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, true);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);

        display_ = new SpiLcdDisplay(panel_io, panel,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                    DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
                                    DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    void InitializeTools() {
        // Refer to MCP documentation
    }

public:
    MyCustomBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeI2c();
        InitializeSpi();
        InitializeDisplay();
        InitializeButtons();
        InitializeTools();
        GetBacklight()->SetBrightness(100);
    }

    virtual AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec audio_codec(
            codec_i2c_bus_,
            I2C_NUM_0,
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN,
            AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }
};

DECLARE_BOARD(MyCustomBoard);
```

### 4. Add Build System Configuration

#### Add a Board Option in `Kconfig.projbuild`

Open `main/Kconfig.projbuild`, and add a new board entry under `choice BOARD_TYPE`:

```kconfig
choice BOARD_TYPE
    prompt "Board Type"
    default BOARD_TYPE_BREAD_COMPACT_WIFI
    help
        Board type definition

    config BOARD_TYPE_MY_CUSTOM_BOARD
        bool "My Custom Board"
        depends on IDF_TARGET_ESP32S3
endchoice
```

> Notes:  
> - Use uppercase for `BOARD_TYPE_MY_CUSTOM_BOARD`  
> - `depends on` specifies the target chip type (e.g., `IDF_TARGET_ESP32S3`)  

#### Update `CMakeLists.txt`

Add your board entry in `main/CMakeLists.txt`:

```cmake
elseif(CONFIG_BOARD_TYPE_MY_CUSTOM_BOARD)
    set(BOARD_TYPE "my-custom-board")
    set(BUILTIN_TEXT_FONT font_puhui_basic_20_4)
    set(BUILTIN_ICON_FONT font_awesome_20_4)
    set(DEFAULT_EMOJI_COLLECTION twemoji_64)
endif()
```

### 5. Configure and Build

#### Option 1: Manual Setup via `idf.py`

```bash
idf.py set-target esp32s3
idf.py fullclean
idf.py menuconfig
idf.py build
idf.py flash monitor
```

#### Option 2: Automated Build with `release.py`

If your board directory includes `config.json`, use:

```bash
python scripts/release.py nodemcu32-lcd1602
```

The script will automatically detect the target chip, apply SDK configs, and build/pack the firmware.

### 6. Create README.md

Include hardware specs, wiring, build, and flash instructions.

## Common Board Components

- **Displays**: ST7789, ILI9341, SH8601, etc.  
- **Audio Codecs**: ES8311, ES7210, AW88298, etc.  
- **Power Management**: AXP2101, PMIC variants  
- **MCP Tools**: Speaker, Screen, Battery, Light control, etc.

## Board Class Hierarchy

- `Board` – Base class  
  - `WifiBoard` – For Wi-Fi boards  
  - `Ml307Board` – For 4G modules  
  - `DualNetworkBoard` – Wi-Fi + 4G switching

## Development Tips

1. Reference similar boards.  
2. Debug step-by-step (display first, then audio).  
3. Ensure accurate pin mapping.  
4. Verify hardware-driver compatibility.

## Troubleshooting

| Issue | Possible Cause |
|-------|----------------|
| Display not working | Incorrect SPI/mirroring settings |
| No audio | Wrong I2S or PA pin configuration |
| Wi-Fi connection failure | Invalid credentials/config |
| Server communication issues | MQTT or WebSocket misconfig |

## References

- ESP-IDF Docs: https://docs.espressif.com/projects/esp-idf/  
- LVGL Docs: https://docs.lvgl.io/  
- ESP-SR Docs: https://github.com/espressif/esp-sr

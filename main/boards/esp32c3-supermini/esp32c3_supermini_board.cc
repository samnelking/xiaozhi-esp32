#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "button.h"
#include "led/single_led.h"
#include "config.h"
#include "board.h"  // 包含 board.h 头文件

// 使用正确的路径包含 wifi_station.h
#include "../../managed_components/78__esp-wifi-connect/include/wifi_station.h"

#include <esp_log.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <driver/spi_common.h>

#define TAG "ESP32C3SuperMiniBoard"

/**
 * ESP32-C3 SuperMini 开发板配置
 * 
 * 硬件配置:
 * - MAX98357A I2S功放
 * - INMP441 I2S麦克风
 * - ST7789 SPI LCD (240x240)
 * - 4MB Flash
 * - 单按键交互
 */
class Esp32C3SuperMiniBoard : public WifiBoard {
private:
    Button boot_button_;
    LcdDisplay* display_ = nullptr;
    WifiStation wifi_station_;  // 添加 WifiStation 实例

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_CLK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeLcdDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));

        ESP_LOGD(TAG, "Install LCD driver");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        esp_lcd_panel_disp_on_off(panel, true);

        display_ = new SpiLcdDisplay(panel_io, panel,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, 
                                    DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            
            // 使用成员变量 wifi_station_ 来调用方法
            if (app.GetDeviceState() == kDeviceStateStarting && !wifi_station_.IsConnected()) {
                // 使用 WifiStation 的 Stop 和 Start 方法来重置
                wifi_station_.Stop();
                vTaskDelay(pdMS_TO_TICKS(100));  // 短暂延迟
                wifi_station_.Start();
                ESP_LOGI(TAG, "WiFi configuration reset");
            }
            app.ToggleChatState();
        });
    }

public:
    Esp32C3SuperMiniBoard() : boot_button_(BOOT_BUTTON_GPIO), wifi_station_() {
        InitializeSpi();
        InitializeLcdDisplay();
        InitializeButtons();
    }

    virtual ~Esp32C3SuperMiniBoard() {
        if (display_) {
            delete display_;
            display_ = nullptr;
        }
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
        return &audio_codec;
    }

    // 实现 Board 类的抽象方法
    virtual std::string GetBoardType() override {
        return "esp32c3-supermini";
    }
    
    virtual NetworkInterface* GetNetwork() override {
        // 这里需要返回网络接口，根据实际情况实现
        // 暂时返回 nullptr，你可能需要根据项目结构调整
        return nullptr;
    }
    
    virtual void StartNetwork() override {
        // 启动网络
        wifi_station_.Start();
    }
    
    virtual const char* GetNetworkStateIcon() override {
        return wifi_station_.IsConnected() ? "wifi" : "wifi_off";
    }
    
    virtual void SetPowerSaveLevel(PowerSaveLevel level) override {
        // 根据 PowerSaveLevel 设置 WiFi 省电级别
        WifiPowerSaveLevel wifi_level;
        switch(level) {
            case PowerSaveLevel::LOW_POWER:
                wifi_level = WifiPowerSaveLevel::LOW_POWER;
                break;
            case PowerSaveLevel::BALANCED:
                wifi_level = WifiPowerSaveLevel::BALANCED;
                break;
            case PowerSaveLevel::PERFORMANCE:
                wifi_level = WifiPowerSaveLevel::PERFORMANCE;
                break;
            default:
                wifi_level = WifiPowerSaveLevel::BALANCED;
        }
        wifi_station_.SetPowerSaveLevel(wifi_level);
    }
    
    virtual std::string GetBoardJson() override {
        // 返回开发板信息的 JSON 字符串
        return R"({
            "name": "ESP32-C3 SuperMini",
            "version": "1.0",
            "display": {
                "width": 240,
                "height": 240,
                "type": "st7789"
            },
            "audio": {
                "input_sample_rate": 16000,
                "output_sample_rate": 24000
            }
        })";
    }
    
    virtual std::string GetDeviceStatusJson() override {
        // 返回设备状态信息的 JSON 字符串
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
            R"({
                "wifi_connected": %s,
                "wifi_ssid": "%s",
                "ip_address": "%s",
                "rssi": %d
            })",
            wifi_station_.IsConnected() ? "true" : "false",
            wifi_station_.GetSsid().c_str(),
            wifi_station_.GetIpAddress().c_str(),
            wifi_station_.GetRssi()
        );
        return std::string(buffer);
    }
};

// 使用 DECLARE_BOARD 宏定义 create_board() 函数
DECLARE_BOARD(Esp32C3SuperMiniBoard)
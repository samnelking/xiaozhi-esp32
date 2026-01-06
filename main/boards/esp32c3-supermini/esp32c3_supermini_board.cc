// ESP32-C3 SuperMini开发板的板级支持文件
// 定义了硬件接口的具体实现

#include "wifi_board.h"          // WiFi板级抽象基类
#include "codecs/no_audio_codec.h" // 无音频编码器（或简单的音频接口）
#include "display/lcd_display.h"   // LCD显示接口
#include "application.h"           // 应用程序管理类
#include "button.h"                // 按钮处理类
#include "led/single_led.h"        // 单LED控制类
#include "config.h"                // 全局配置
#include "board.h"                 // 板级抽象基类

// 使用正确的路径包含 wifi_station.h
// 注意：使用了相对路径，指向组件目录中的WiFi站管理类
#include "../../managed_components/78__esp-wifi-connect/include/wifi_station.h"

// ESP-IDF系统头文件
#include <esp_log.h>               // ESP32日志系统
#include <esp_lcd_panel_vendor.h>  // LCD面板厂商特定驱动
#include <esp_lcd_panel_io.h>      // LCD面板IO接口
#include <esp_lcd_panel_ops.h>     // LCD面板操作函数
#include <driver/spi_common.h>     // SPI通用驱动

#define TAG "ESP32C3SuperMiniBoard" // 日志标签

/**
 * ESP32-C3 SuperMini 开发板配置类
 * 
 * 继承自WifiBoard，实现了针对特定硬件的功能
 * 
 * 硬件配置:
 * - MAX98357A I2S功放（音频输出）
 * - INMP441 I2S麦克风（音频输入）
 * - ST7789 SPI LCD (240x240)（显示）
 * - 4MB Flash（存储）
 * - 单按键交互（用户输入）
 */
class Esp32C3SuperMiniBoard : public WifiBoard {
private:
    Button boot_button_;           // Boot按钮实例
    LcdDisplay* display_ = nullptr; // LCD显示实例指针
    WifiStation wifi_station_;     // WiFi站管理实例

    /**
     * 初始化SPI总线
     * 用于驱动ST7789 LCD显示屏
     */
    void InitializeSpi() {
        spi_bus_config_t buscfg = {}; // SPI总线配置结构体，初始化为0
        buscfg.mosi_io_num = DISPLAY_MOSI_PIN; // MOSI引脚
        buscfg.miso_io_num = GPIO_NUM_NC;      // MISO引脚未使用
        buscfg.sclk_io_num = DISPLAY_CLK_PIN;  // 时钟引脚
        buscfg.quadwp_io_num = GPIO_NUM_NC;    // Quad SPI写保护引脚未使用
        buscfg.quadhd_io_num = GPIO_NUM_NC;    // Quad SPI保持引脚未使用
        // 最大传输大小：240*240像素 * 2字节(16位色深)
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        // 初始化SPI2总线，使用自动分配的DMA通道
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    /**
     * 初始化LCD显示屏
     * 配置ST7789驱动，设置显示参数
     */
    void InitializeLcdDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr; // LCD面板IO句柄
        esp_lcd_panel_handle_t panel = nullptr;       // LCD面板句柄

        ESP_LOGD(TAG, "Install panel IO"); // 调试日志：安装面板IO
        esp_lcd_panel_io_spi_config_t io_config = {}; // SPI IO配置
        io_config.cs_gpio_num = DISPLAY_CS_PIN;       // 片选引脚
        io_config.dc_gpio_num = DISPLAY_DC_PIN;       // 数据/命令选择引脚
        io_config.spi_mode = DISPLAY_SPI_MODE;        // SPI模式
        io_config.pclk_hz = 40 * 1000 * 1000;         // 时钟频率40MHz
        io_config.trans_queue_depth = 10;             // 传输队列深度
        io_config.lcd_cmd_bits = 8;                   // 命令位宽8位
        io_config.lcd_param_bits = 8;                 // 参数位宽8位
        // 创建SPI面板IO
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));

        ESP_LOGD(TAG, "Install LCD driver"); // 调试日志：安装LCD驱动
        esp_lcd_panel_dev_config_t panel_config = {}; // 面板设备配置
        panel_config.reset_gpio_num = DISPLAY_RST_PIN; // 复位引脚
        panel_config.rgb_ele_order = DISPLAY_RGB_ORDER; // RGB元素顺序
        panel_config.bits_per_pixel = 16;              // 每像素16位(5-6-5 RGB)
        // 创建ST7789面板
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

        // 面板初始化流程
        esp_lcd_panel_reset(panel);      // 复位面板
        esp_lcd_panel_init(panel);       // 初始化面板
        esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR); // 颜色反转
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY); // 交换XY坐标
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y); // 镜像设置
        esp_lcd_panel_disp_on_off(panel, true); // 打开显示

        // 创建LCD显示对象，封装底层面板操作
        display_ = new SpiLcdDisplay(panel_io, panel,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, 
                                    DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    /**
     * 初始化按钮
     * 设置按钮回调函数
     */
    void InitializeButtons() {
        // 设置Boot按钮点击回调函数
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance(); // 获取应用程序实例
            
            // 特殊处理：在启动阶段且WiFi未连接时，点击按钮重置WiFi
            if (app.GetDeviceState() == kDeviceStateStarting && !wifi_station_.IsConnected()) {
                // 停止并重新启动WiFi站以重置连接
                wifi_station_.Stop();
                vTaskDelay(pdMS_TO_TICKS(100));  // 短暂延迟100ms
                wifi_station_.Start();
                ESP_LOGI(TAG, "WiFi configuration reset"); // 信息日志
            }
            // 切换聊天状态（主功能）
            app.ToggleChatState();
        });
    }

public:
    /**
     * 构造函数
     * 初始化所有硬件组件
     */
    Esp32C3SuperMiniBoard() : boot_button_(BOOT_BUTTON_GPIO), wifi_station_() {
        InitializeSpi();           // 初始化SPI总线
        InitializeLcdDisplay();    // 初始化LCD显示
        InitializeButtons();       // 初始化按钮
    }

    /**
     * 析构函数
     * 清理分配的资源
     */
    virtual ~Esp32C3SuperMiniBoard() {
        if (display_) {
            delete display_;      // 释放显示对象内存
            display_ = nullptr;
        }
    }

    /**
     * 获取LED实例
     * 返回静态的SingleLed对象
     */
    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO); // 静态对象，只初始化一次
        return &led;
    }

    /**
     * 获取显示实例
     * 返回初始化时创建的LCD显示对象
     */
    virtual Display* GetDisplay() override {
        return display_;
    }

    /**
     * 获取音频编解码器实例
     * 返回无音频编解码器的单工音频接口
     */
    virtual AudioCodec* GetAudioCodec() override {
        // 创建并返回音频接口的静态实例
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE, // 采样率
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT, // 扬声器引脚
            AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);    // 麦克风引脚
        return &audio_codec;
    }

    /**
     * 获取板型标识
     * 返回板型名称字符串
     */
    virtual std::string GetBoardType() override {
        return "esp32c3-supermini";
    }
    
    /**
     * 获取网络接口
     * 暂时返回nullptr，需要根据项目结构调整
     */
    virtual NetworkInterface* GetNetwork() override {
        // 这里需要返回网络接口，根据实际情况实现
        // 暂时返回 nullptr，你可能需要根据项目结构调整
        return nullptr;
    }
    
    /**
     * 启动网络
     * 调用WiFi站的Start方法
     */
    virtual void StartNetwork() override {
        wifi_station_.Start(); // 启动WiFi连接
    }
    
    /**
     * 获取网络状态图标名称
     * 根据WiFi连接状态返回对应的图标名称
     */
    virtual const char* GetNetworkStateIcon() override {
        return wifi_station_.IsConnected() ? "wifi" : "wifi_off";
    }
    
    /**
     * 设置省电级别
     * 将板级省电级别映射到WiFi省电级别
     */
    virtual void SetPowerSaveLevel(PowerSaveLevel level) override {
        WifiPowerSaveLevel wifi_level;
        switch(level) {
            case PowerSaveLevel::LOW_POWER:     // 低功耗模式
                wifi_level = WifiPowerSaveLevel::LOW_POWER;
                break;
            case PowerSaveLevel::BALANCED:      // 平衡模式
                wifi_level = WifiPowerSaveLevel::BALANCED;
                break;
            case PowerSaveLevel::PERFORMANCE:   // 性能模式
                wifi_level = WifiPowerSaveLevel::PERFORMANCE;
                break;
            default:
                wifi_level = WifiPowerSaveLevel::BALANCED; // 默认平衡模式
        }
        wifi_station_.SetPowerSaveLevel(wifi_level); // 设置WiFi省电级别
    }
    
    /**
     * 获取开发板信息JSON
     * 返回描述开发板硬件的JSON字符串
     */
    virtual std::string GetBoardJson() override {
        // 使用原始字符串字面量返回JSON
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
    
    /**
     * 获取设备状态JSON
     * 返回当前设备状态信息的JSON字符串
     */
    virtual std::string GetDeviceStatusJson() override {
        // 使用格式化字符串生成JSON
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
            R"({
                "wifi_connected": %s,
                "wifi_ssid": "%s",
                "ip_address": "%s",
                "rssi": %d
            })",
            wifi_station_.IsConnected() ? "true" : "false", // WiFi连接状态
            wifi_station_.GetSsid().c_str(),               // WiFi SSID
            wifi_station_.GetIpAddress().c_str(),          // IP地址
            wifi_station_.GetRssi()                        // 信号强度
        );
        return std::string(buffer);
    }
};

// 使用 DECLARE_BOARD 宏定义 create_board() 函数
// 这个宏应该会创建一个全局的create_board函数，返回Board*类型
DECLARE_BOARD(Esp32C3SuperMiniBoard)

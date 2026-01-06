#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

// 防止头文件被重复包含的宏定义
// 这是C/C++头文件的标准保护方式

#include <driver/gpio.h>
// 引入ESP32 GPIO驱动头文件，用于引脚编号定义（如GPIO_NUM_2）

// ============================================================
// 开发板配置：ESP32-C3 SuperMini
// 硬件组件：MAX98357A (音频放大器) + INMP441 (麦克风) + ST7789 (显示屏)
// ============================================================

// 音频输入采样率：16kHz（适用于语音识别场景）
// 标准语音采样率，平衡音质和存储效率
#define AUDIO_INPUT_SAMPLE_RATE  16000

// 音频输出采样率：24kHz（适用于音频播放）
// 比输入采样率稍高，提供更好的播放音质
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// 定义使用单工I2S模式（独立的TX/RX通道）
// 与双工模式相比，单工模式允许音频输入和输出使用不同的时序
#define AUDIO_I2S_METHOD_SIMPLEX

// ============ MAX98357A 扬声器配置（I2S输出通道）============
// MAX98357A是一款I2S数字输入音频放大器，无需DAC
#define AUDIO_I2S_SPK_GPIO_BCLK  GPIO_NUM_2  // 位时钟（BCLK），同步数据传输
#define AUDIO_I2S_SPK_GPIO_LRCK  GPIO_NUM_3  // 左右声道时钟（LRCK），区分左右声道
#define AUDIO_I2S_SPK_GPIO_DOUT  GPIO_NUM_4  // 数据输出（DOUT），音频数据发送到扬声器

// ============ INMP441 麦克风配置（I2S输入通道）=============
// INMP441是一款高性能数字麦克风，直接输出I2S格式数据
#define AUDIO_I2S_MIC_GPIO_SCK   GPIO_NUM_5  // 串行时钟（SCK），等同于BCLK
#define AUDIO_I2S_MIC_GPIO_WS    GPIO_NUM_6  // 字选择（WS），等同于LRCK
#define AUDIO_I2S_MIC_GPIO_DIN   GPIO_NUM_7  // 数据输入（DIN），从麦克风接收音频数据

// ===================== 按钮配置 ============================
// 注：GPIO_NUM_NC表示未连接（No Connection）
#define BOOT_BUTTON_GPIO        GPIO_NUM_9   // Boot按钮，通常用于启动模式/复位
#define TOUCH_BUTTON_GPIO       GPIO_NUM_NC  // 触摸按钮（未使用）
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_NC  // 音量加按钮（未使用）
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC  // 音量减按钮（未使用）

// ===================== LED配置 =============================
// 板载LED，用于状态指示
#define BUILTIN_LED_GPIO        GPIO_NUM_8   // 开发板上的用户LED

// ==================== ST7789显示屏配置 ======================
// ST7789是一款240x240像素的SPI接口TFT显示屏
#define DISPLAY_MOSI_PIN        GPIO_NUM_10  // SPI主出从入（MOSI），发送显示数据
#define DISPLAY_CLK_PIN         GPIO_NUM_1   // SPI时钟信号（SCK）
#define DISPLAY_CS_PIN          GPIO_NUM_0   // 片选信号（Chip Select），低电平使能设备
#define DISPLAY_DC_PIN          GPIO_NUM_21  // 数据/命令选择引脚（Data/Command）
                                             // 高电平：数据，低电平：命令
#define DISPLAY_RST_PIN         GPIO_NUM_NC  // 复位引脚（未连接，使用软件复位）
#define DISPLAY_BACKLIGHT_PIN   GPIO_NUM_NC  // 背光控制引脚（未连接，背光常亮）

// 显示屏参数配置
#define DISPLAY_WIDTH           240          // 显示屏宽度（像素）
#define DISPLAY_HEIGHT          240          // 显示屏高度（像素）
#define DISPLAY_MIRROR_X        false        // X轴不镜像（正常方向）
#define DISPLAY_MIRROR_Y        false        // Y轴不镜像（正常方向）
#define DISPLAY_SWAP_XY         false        // 不交换X/Y坐标（正常方向）
#define DISPLAY_INVERT_COLOR    true         // 颜色反转（根据显示屏特性调整）
#define DISPLAY_RGB_ORDER       LCD_RGB_ELEMENT_ORDER_RGB  // RGB颜色顺序
#define DISPLAY_OFFSET_X        0            // X方向偏移量（像素）
#define DISPLAY_OFFSET_Y        0            // Y方向偏移量（像素）
#define DISPLAY_SPI_MODE        0            // SPI模式0（CPOL=0, CPHA=0）

#endif // _BOARD_CONFIG_H_

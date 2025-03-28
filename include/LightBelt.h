#ifndef LIGHTBELT_H
#define LIGHTBELT_H

#include <Adafruit_NeoPixel.h>

/**
 * @brief LED灯带控制类
 * @details 用于控制多层WS2812 LED灯带，每层LED数量相同
 */
class LightBelt {
private:
    Adafruit_NeoPixel strip;
    uint8_t layers;
    uint8_t ledsPerLayer;
    uint32_t totalLeds;
    float maxBrightness;    // 最大亮度限制(0.0-1.0)
    
public:
    /**
     * @brief 构造函数
     * @param pin LED灯带的数据引脚
     * @param numLayers 灯带的层数
     * @param ledsInLayer 每层LED的数量
     */
    LightBelt(uint8_t pin, uint8_t numLayers, uint8_t ledsInLayer);

    /**
     * @brief 初始化LED灯带
     */
    void begin();

    /**
     * @brief 设置指定层的LED颜色
     * @param layer 层号（从0开始）
     * @param color 32位RGB颜色值
     */
    void setLayerColor(uint8_t layer, uint32_t color);

    /**
     * @brief 设置所有层的LED为相同颜色
     * @param color 32位RGB颜色值
     */
    void setAllLayersColor(uint32_t color);

    /**
     * @brief 使LED灯带呈现彩虹循环效果
     * @param periodMs 完成一次彩虹循环的时间（毫秒）
     */
    void rainbowCycle(uint32_t periodMs);

    /**
     * @brief 颜色轮转换函数
     * @param wheelPos 0-255的位置值
     * @return 对应位置的32位RGB颜色值
     */
    uint32_t wheel(byte wheelPos);

    /**
     * @brief 使LED灯带呈现呼吸灯效果
     * @param color 32位RGB颜色值
     * @param periodMs 完成一次呼吸周期的时间（毫秒）
     */
    void breathing(uint32_t color, uint32_t periodMs);
    
    /**
     * @brief 调整颜色亮度
     * @param color 原始颜色值
     * @param brightness 亮度值（0-255）
     * @return 调整亮度后的颜色
     */
    uint32_t dimColor(uint32_t color, uint8_t brightness);

    /**
     * @brief 设置LED灯带的最大亮度
     * @param brightness 亮度值(0.0-1.0)
     */
    void setMaxBrightness(float brightness);
    
    /**
     * @brief 获取当前设置的最大亮度
     * @return 当前最大亮度值(0.0-1.0)
     */
    float getMaxBrightness() const;
};

#endif

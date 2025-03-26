#ifndef SERVOPLATFORMINTER_H
#define SERVOPLATFORMINTER_H

#include <Arduino.h>

/**
 * @brief 基于ESP32内部PWM的舵机平台控制类
 * @details 使用ESP32的LEDC模块驱动多层舵机，每层两个舵机同步运动
 */
class ServoPlatformInter {
private:
    uint8_t layers;
    uint8_t minAngle;
    uint8_t maxAngle;
    uint16_t minPulseWidth;    // 最小脉冲宽度（微秒）
    uint16_t maxPulseWidth;    // 最大脉冲宽度（微秒）
    float currentAngles[12];    // 最多支持6层，每层2个舵机
    uint8_t servoPins[12];      // 存储每个舵机的引脚
    bool sweepCompleted;    // 添加标记变量，表示一次性扫描是否完成
    uint32_t sweepStartTime;  // 添加扫描开始时间记录
    
    void initPWM();
    void setServoPWM(uint8_t channel, uint16_t pulseWidth);
    uint16_t angleToPulseWidth(uint8_t angle);
    void setServoAngle(uint8_t servoNum, uint8_t angle);
    void setLayerAngle(uint8_t layer, uint8_t angle);
    void servoSelfTest();

public:
    /**
     * @brief 构造函数
     * @param numLayers 舵机层数
     * @param minAng 舵机最小角度
     * @param maxAng 舵机最大角度
     */
    ServoPlatformInter(uint8_t numLayers, uint8_t minAng = 0, uint8_t maxAng = 180);
    
    /**
     * @brief 初始化舵机平台
     * @details 包括PWM通道配置和舵机自检
     */
    void begin();

    /**
     * @brief 使指定层的舵机进行往复运动
     * @param layer 层号（从0开始）
     * @param periodMs 完成一次往复运动的时间（毫秒）
     */
    void sweepLayer(uint8_t layer, uint32_t periodMs);

    /**
     * @brief 使所有层的舵机进行带相位差的往复运动
     * @param periodMs 完成一次往复运动的时间（毫秒）
     * @param phaseDiff 相邻层之间的相位差（度）
     */
    void sweepAllLayers(uint32_t periodMs, float phaseDiff);

    /**
     * @brief 使所有层的舵机进行带相位差的往复运动，但仅执行一次
     * @param periodMs 完成一次往复运动的时间（毫秒）
     * @param phaseDiff 相邻层之间的相位差（度）
     * @return 如果运动完成返回true，否则返回false
     */
    bool sweepAllLayersOnce(uint32_t periodMs, float phaseDiff);
    
    /**
     * @brief 重置一次性扫描状态
     */
    void resetSweep();

    /**
     * @brief 设置指定层舵机角度
     * @param layer 层号（从0开始）
     * @param value 0-1023范围的输入值
     */
    void setLayerAngleFromValue(uint8_t layer, int value);

    /**
     * @brief 使所有层的舵机进行分组往复运动
     * @details 将舵机分为两组：1,3,5为第一组，2,4,6为第二组
     *          第一组初始为最小角度，第二组初始为最大角度
     * @param periodMs 完成一次往复运动的时间（毫秒）
     */
    void sweepAlternateGroups(uint32_t periodMs);
};

#endif

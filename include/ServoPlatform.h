#ifndef SERVOPLATFORM_H
#define SERVOPLATFORM_H

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

/**
 * @brief 基于PCA9685的舵机平台控制类
 * @details 使用PCA9685 PWM控制器驱动多层舵机，每层两个舵机同步运动
 */
class ServoPlatform {
private:
    Adafruit_PWMServoDriver pwm;
    uint8_t layers;
    uint8_t minAngle;
    uint8_t maxAngle;
    uint16_t servoMin;    // 最小脉冲长度计数值
    uint16_t servoMax;    // 最大脉冲长度计数值
    float currentAngles[16];  // 存储当前角度
    uint8_t i2cAddress;  // 添加I2C地址成员变量

    uint16_t angleToMicros(uint8_t angle);
    void setServoAngle(uint8_t servoNum, uint8_t angle);
    void setLayerAngle(uint8_t layer, uint8_t angle);
    uint8_t scanI2CAddress();  // 添加扫描方法
    void servoSelfTest();  // 添加自检方法

public:
    /**
     * @brief 构造函数
     * @param numLayers 舵机层数
     * @param i2cAddress PCA9685的I2C地址
     * @param minAng 舵机最小角度
     * @param maxAng 舵机最大角度
     */
    ServoPlatform(uint8_t numLayers, uint8_t i2cAddress = 0x40, uint8_t minAng = 0, uint8_t maxAng = 180);
    
    /**
     * @brief 初始化舵机平台
     * @details 包括I2C初始化、设备扫描和舵机自检
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
};

#endif

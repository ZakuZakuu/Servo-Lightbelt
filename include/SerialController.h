#ifndef SERIAL_CONTROLLER_H
#define SERIAL_CONTROLLER_H

#include <Arduino.h>
#include "LightBelt.h"
#include "ServoPlatformInter.h"
#include "ServoPlatform.h"

/**
 * @class SerialController
 * @brief 串口控制器类
 * 
 * @details 用于通过串口接收和处理控制命令，实现灯光和舵机的模式切换。
 * 功能与BluetoothController类似，但通过串口通信且不需处理连接断开。
 */
class SerialController {
private:
    LightBelt* lightBelt;            ///< 灯带控制器指针
    void* servoPlatform;             ///< 舵机平台指针（可以是任意类型）
    bool useInternalPWM;             ///< 是否使用内部PWM
    char currentMode[10];            ///< 当前工作模式
    int params[6];                   ///< 存储6个参数值（0-1023）
    uint32_t periodMs;               ///< 动作周期（毫秒）
    
    // 命令处理相关
    char cmdBuffer[64];              ///< 命令缓冲区
    uint8_t cmdIndex;                ///< 当前命令索引
    
    /**
     * @brief 处理命令
     */
    void processCommand();
    
    /**
     * @brief 设置预设模式
     */
    void setPresetMode(const char* modeName);
    
    /**
     * @brief 设置控制模式
     */
    void setControlMode(const char* modeName, int* parameters);
    
    /**
     * @brief 发送状态
     */
    void sendStatus();
    
    /**
     * @brief 比较模式名称
     */
    bool modeEquals(const char* modeName);
    
    /**
     * @brief 解析整数参数
     */
    int parseIntParam(const char* str);
    
    /**
     * @brief 执行Idle模式
     */
    void executeIdleMode();

    /**
     * @brief 执行Heatup模式
     * @details 舵机相位差半个周期往返运动，灯带呼吸效果与对应舵机同步
     */
    void executeHeatupMode();

    /**
     * @brief 执行Cooldown模式
     * @details 从最高层开始，每层依次由最大角度变为最小角度，灯光同步由亮变暗
     */
    void executeCooldownMode();
    
    /**
     * @brief 执行Standby模式
     * @details 所有舵机回到最小值，全部灯带为蓝色呼吸灯样式
     */
    void executeStandbyMode();
    
    /**
     * @brief 获取Cooldown模式状态指针
     * @return Cooldown模式状态指针
     */
    uint8_t* getCooldownStatePtr();
    
    /**
     * @brief 获取Cooldown模式开始时间指针
     * @return Cooldown模式开始时间指针
     */
    uint32_t* getCooldownStartTimePtr();

    /**
     * @brief 获取Idle模式复位标志指针
     * @return Idle模式复位标志指针
     */
    bool* getIdleResetFlag();
    
    /**
     * @brief 获取Idle模式开始时间指针
     * @return Idle模式开始时间指针
     */
    uint32_t* getIdleStartTimeFlag();

public:
    /**
     * @brief 构造函数 - 使用内部PWM
     */
    SerialController(LightBelt* lightBeltPtr, ServoPlatformInter* servoPlatformPtr, uint32_t cycleTimeMs = 5000);
    
    /**
     * @brief 构造函数 - 使用外部舵机驱动
     */
    SerialController(LightBelt* lightBeltPtr, ServoPlatform* servoPlatformPtr, uint32_t cycleTimeMs = 5000);
    
    /**
     * @brief 初始化串口控制
     */
    void begin();
    
    /**
     * @brief 更新处理串口命令
     */
    void update();
};

#endif

#ifndef BLUETOOTH_CONTROLLER_H
#define BLUETOOTH_CONTROLLER_H

#include <Arduino.h>
#include <BluetoothSerial.h>
#include "LightBelt.h"
#include "ServoPlatformInter.h"

/**
 * @class BluetoothController
 * @brief 蓝牙控制器类
 * 
 * @details 用于通过蓝牙接收和处理控制命令，实现灯光和舵机的模式切换。
 * 该类支持以下功能：
 *   - 接收并解析蓝牙命令
 *   - 切换不同的工作模式（Rainbow、Idle、Follow）
 *   - 实时控制舵机角度
 *   - 查询当前状态
 */
class BluetoothController {
private:
    BluetoothSerial BT;              ///< 蓝牙串口对象
    LightBelt* lightBelt;            ///< 灯带控制器指针
    ServoPlatformInter* servoPlatform; ///< 舵机平台指针
    String currentMode;              ///< 当前工作模式
    int params[6];                   ///< 存储6个参数值（0-1023）
    uint32_t periodMs;               ///< 动作周期（毫秒）
    
    /**
     * @brief 处理接收到的命令
     * @param command 格式为"模式|参数1|参数2|参数3|参数4|参数5|参数6"的命令字符串
     */
    void processCommand(String command);
    
    /**
     * @brief 设置预设模式
     * @param modeName 模式名称，可以是"Rainbow"或"Idle"
     */
    void setPresetMode(String modeName);
    
    /**
     * @brief 设置控制模式
     * @param modeName 模式名称，应为"Follow"
     * @param parameters 包含6个整数（0-1023）的参数数组
     */
    void setControlMode(String modeName, int* parameters);
    
    /**
     * @brief 发送当前状态信息
     * @details 通过蓝牙发送当前模式和参数信息
     */
    void sendStatus();

public:
    /**
     * @brief 构造函数
     * 
     * @param lightBeltPtr 灯带控制器指针
     * @param servoPlatformPtr 舵机平台指针
     * @param cycleTimeMs 动作周期（毫秒），默认为5000ms
     */
    BluetoothController(LightBelt* lightBeltPtr, ServoPlatformInter* servoPlatformPtr, uint32_t cycleTimeMs = 5000);
    
    /**
     * @brief 初始化蓝牙模块
     * 
     * @param deviceName 蓝牙设备名称，默认为"ESP32-Lightbelt"
     */
    void begin(const char* deviceName = "ESP32-Lightbelt");
    
    /**
     * @brief 更新处理蓝牙命令
     * 
     * @details 需要在主循环中频繁调用此方法以实现以下功能：
     *   - 检查并处理蓝牙接收到的命令
     *   - 根据当前模式执行相应的操作
     *   - Rainbow模式：彩虹灯效果和相位差舵机运动
     *   - Idle模式：蓝色灯光和同步舵机运动
     *   - Follow模式：根据接收参数控制各层舵机角度
     */
    void update();
};

#endif

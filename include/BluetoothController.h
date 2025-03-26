#ifndef BLUETOOTH_CONTROLLER_H
#define BLUETOOTH_CONTROLLER_H

#include <Arduino.h>
#include <BluetoothSerial.h>
#include "LightBelt.h"
#include "ServoPlatformInter.h"

/**
 * @brief 蓝牙控制器类
 * @details 用于通过蓝牙接收和处理控制命令，实现灯光和舵机的模式切换
 */
class BluetoothController {
private:
    BluetoothSerial BT;
    LightBelt* lightBelt;
    ServoPlatformInter* servoPlatform;
    String currentMode;
    int params[6];
    uint32_t periodMs;  // 模式运行周期
    
    void processCommand(String command);
    void setPresetMode(String modeName);
    void setControlMode(String modeName, int* parameters);
    void sendStatus();

public:
    /**
     * @brief 构造函数
     * @param lightBeltPtr 灯带控制器指针
     * @param servoPlatformPtr 舵机平台指针
     * @param cycleTimeMs 动作周期（毫秒）
     */
    BluetoothController(LightBelt* lightBeltPtr, ServoPlatformInter* servoPlatformPtr, uint32_t cycleTimeMs = 5000);
    
    /**
     * @brief 初始化蓝牙模块
     * @param deviceName 蓝牙设备名称
     */
    void begin(const char* deviceName = "ESP32-Lightbelt");
    
    /**
     * @brief 更新处理蓝牙命令
     * @details 需要在loop中频繁调用
     */
    void update();
};

#endif

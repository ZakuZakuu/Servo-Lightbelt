/**
 * @file BluetoothController.cpp
 * @brief 蓝牙控制器类的实现
 * 
 * @details 实现通过蓝牙串口接收命令，控制LED灯带和舵机平台。
 * 支持多种工作模式和实时控制功能。
 */

#include "BluetoothController.h"

/**
 * @brief 构造函数，初始化蓝牙控制器
 */
BluetoothController::BluetoothController(LightBelt* lightBeltPtr, ServoPlatformInter* servoPlatformPtr, uint32_t cycleTimeMs)
    : lightBelt(lightBeltPtr), servoPlatform(servoPlatformPtr), periodMs(cycleTimeMs) {
    currentMode = "Idle";  // 默认模式
    
    // 初始化参数数组
    for (int i = 0; i < 6; i++) {
        params[i] = 512;  // 默认中间位置
    }
}

/**
 * @brief 初始化蓝牙模块并设置设备名称
 */
void BluetoothController::begin(const char* deviceName) {
    BT.begin(deviceName);
    Serial.print("蓝牙设备已启动，名称: ");
    Serial.println(deviceName);
    Serial.println("等待连接...");
}

/**
 * @brief 更新处理蓝牙命令并执行当前模式的动作
 */
void BluetoothController::update() {
    if (BT.available()) {
        String command = BT.readStringUntil('\n');
        command.trim();
        Serial.print("收到命令: ");
        Serial.println(command);
        processCommand(command);
    }
    
    // 根据当前模式执行相应操作
    if (currentMode == "Rainbow") {
        lightBelt->rainbowCycle(periodMs);
        servoPlatform->sweepAllLayers(periodMs, 30.0);
    }
    else if (currentMode == "Idle") {
        // 蓝色: R=0, G=0, B=255
        lightBelt->setAllLayersColor(lightBelt->wheel(170));  // 蓝色对应wheel大约170
        
        // 修改：所有舵机平台同步往复运动，周期设为3秒
        uint32_t idlePeriodMs = 3000;  // 设置Idle模式的运动周期为3秒
        
        // 对所有层应用相同的相位（相位差为0），实现统一往复运动
        servoPlatform->sweepAllLayers(idlePeriodMs, 0.0);
    }
    else if (currentMode == "Follow") {
        // 在Follow模式下，根据接收到的参数实时设置舵机角度
        for (int i = 0; i < 6; i++) {
            if (i < 3) {  // 假设最多支持6层，但目前只使用3层
                servoPlatform->setLayerAngleFromValue(i, params[i]);
            }
        }
    }
}

/**
 * @brief 处理接收到的命令字符串
 * 
 * @details 解析命令格式，提取模式名称和参数，然后调用相应的处理函数
 */
void BluetoothController::processCommand(String command) {
    // 解析命令
    int firstSeparator = command.indexOf('|');
    if (firstSeparator == -1) {
        Serial.println("命令格式错误!");
        return;
    }
    
    String modeName = command.substring(0, firstSeparator);
    int newParams[6] = {0};
    
    // 解析参数
    String remainingCommand = command.substring(firstSeparator + 1);
    for (int i = 0; i < 6; i++) {
        int nextSeparator = remainingCommand.indexOf('|');
        if (nextSeparator == -1) {
            // 最后一个参数
            newParams[i] = remainingCommand.toInt();
            break;
        } else {
            newParams[i] = remainingCommand.substring(0, nextSeparator).toInt();
            remainingCommand = remainingCommand.substring(nextSeparator + 1);
        }
    }
    
    // 处理特殊命令
    if (modeName == "Lookup") {
        sendStatus();
        return;
    }
    
    // 预设模式处理
    if (modeName == "Rainbow" || modeName == "Idle") {
        setPresetMode(modeName);
    }
    // 控制模式处理
    else if (modeName == "Follow") {
        setControlMode(modeName, newParams);
    }
    else {
        Serial.print("未知模式: ");
        Serial.println(modeName);
    }
}

/**
 * @brief 设置预设模式（Rainbow或Idle）
 */
void BluetoothController::setPresetMode(String modeName) {
    currentMode = modeName;
    Serial.print("设置预设模式: ");
    Serial.println(modeName);
    
    // 确认模式已设置
    String response = "Mode=" + modeName;
    BT.println(response);
}

/**
 * @brief 设置控制模式（Follow）并更新控制参数
 */
void BluetoothController::setControlMode(String modeName, int* parameters) {
    currentMode = modeName;
    // 更新参数
    for (int i = 0; i < 6; i++) {
        params[i] = parameters[i];
    }
    
    Serial.print("设置控制模式: ");
    Serial.print(modeName);
    Serial.print(" 参数: ");
    for (int i = 0; i < 6; i++) {
        Serial.print(params[i]);
        Serial.print(" ");
    }
    Serial.println();
    
    // 确认模式已设置
    String response = "Mode=" + modeName;
    BT.println(response);
}

/**
 * @brief 发送当前状态信息
 * 
 * @details 格式为：当前模式|参数1|参数2|参数3|参数4|参数5|参数6
 */
void BluetoothController::sendStatus() {
    String response = currentMode;
    for (int i = 0; i < 6; i++) {
        response += "|" + String(params[i]);
    }
    BT.println(response);
    Serial.print("发送状态: ");
    Serial.println(response);
}

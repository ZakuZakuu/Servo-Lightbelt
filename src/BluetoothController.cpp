/**
 * @file BluetoothController.cpp
 * @brief 蓝牙控制器类的实现
 * 
 * @details 实现通过蓝牙串口接收命令，控制LED灯带和舵机平台。
 * 支持多种工作模式和实时控制功能。
 */

#include "BluetoothController.h"

/**
 * @brief 构造函数 - 使用内部PWM
 */
BluetoothController::BluetoothController(LightBelt* lightBeltPtr, ServoPlatformInter* servoPlatformPtr, uint32_t cycleTimeMs)
    : lightBelt(lightBeltPtr), servoPlatform(servoPlatformPtr), periodMs(cycleTimeMs) {
    useInternalPWM = true;
    currentMode = "Disconnect";  // 初始模式设为Disconnect
    
    // 初始化参数数组
    for (int i = 0; i < 6; i++) {
        params[i] = 512;  // 默认中间位置
    }
    
    isConnected = false;
    lastActivityTime = 0;
    disconnectTimeout = 5000; // 默认5秒超时
}

/**
 * @brief 构造函数 - 使用外部舵机驱动
 */
BluetoothController::BluetoothController(LightBelt* lightBeltPtr, ServoPlatform* servoPlatformPtr, uint32_t cycleTimeMs)
    : lightBelt(lightBeltPtr), servoPlatform(servoPlatformPtr), periodMs(cycleTimeMs) {
    useInternalPWM = false;
    currentMode = "Disconnect";  // 初始模式设为Disconnect
    
    // 初始化参数数组
    for (int i = 0; i < 6; i++) {
        params[i] = 512;  // 默认中间位置
    }
    
    isConnected = false;
    lastActivityTime = 0;
    disconnectTimeout = 5000; // 默认5秒超时
}

/**
 * @brief 初始化蓝牙模块并设置设备名称
 */
void BluetoothController::begin(const char* deviceName) {
    BT.begin(deviceName);
    Serial.print("蓝牙设备已启动，名称: ");
    Serial.println(deviceName);
    Serial.println("等待连接...");
    
    // 初始状态为断开连接
    handleDisconnect();
}

/**
 * @brief 更新处理蓝牙命令并执行当前模式的动作
 */
void BluetoothController::update() {
    // 检查连接状态
    bool connectionStatus = checkConnection();
    
    // 连接状态变化检测
    if (!isConnected && connectionStatus) {
        // 从断开状态到已连接状态
        Serial.println("蓝牙已连接");
        // 自动切换到Idle模式
        currentMode = "Idle";
        Serial.println("自动切换到Idle模式");
    } else if (isConnected && !connectionStatus) {
        // 从已连接到断开连接
        Serial.println("蓝牙连接已断开");
        currentMode = "Disconnect";
        handleDisconnect();
    }
    
    // 更新连接状态
    isConnected = connectionStatus;
    
    // 处理蓝牙命令
    if (BT.available()) {
        String command = BT.readStringUntil('\n');
        command.trim();
        Serial.print("收到命令: ");
        Serial.println(command);
        processCommand(command);
        
        // 更新活动时间
        lastActivityTime = millis();
    }
    
    // 根据当前模式和连接状态执行相应操作
    if (currentMode == "Disconnect") {
        // 断开连接状态处理
        handleDisconnect();
    } else if (currentMode == "Rainbow") {
        lightBelt->rainbowCycle(periodMs);
        
        // 根据舵机平台类型调用相应的方法
        if (useInternalPWM) {
            ((ServoPlatformInter*)servoPlatform)->sweepAllLayers(periodMs, 30.0);
        } else {
            ((ServoPlatform*)servoPlatform)->sweepAllLayers(periodMs, 30.0);
        }
    } else if (currentMode == "Heatup") {  // 将"Alternate"改为"Heatup"
        // 新模式：交替分组运动
        lightBelt->rainbowCycle(periodMs * 2); // 灯光效果可以调整
        
        if (useInternalPWM) {
            ((ServoPlatformInter*)servoPlatform)->sweepAlternateGroups(periodMs);
        } else {
            ((ServoPlatform*)servoPlatform)->sweepAlternateGroups(periodMs);
        }
    }
    else if (currentMode == "Idle") {
        // Idle模式: 白色呼吸灯效果，所有舵机回到最大角度
        
        // 创建白色 (R=255, G=255, B=255)
        uint32_t whiteColor = lightBelt->wheel(255);  // 白色
        
        // 实现呼吸灯效果
        lightBelt->breathing(whiteColor, 3000);  // 3秒周期的呼吸效果
        
        // 设置所有舵机为最大角度
        for (uint8_t layer = 0; layer < 3; layer++) { // 假设有3层
            if (useInternalPWM) {
                ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(layer, 1023);
            } else {
                ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(layer, 1023);
            }
        }
    }
    else if (currentMode == "Follow") {
        // 在Follow模式下，根据接收到的参数实时设置舵机角度
        for (int i = 0; i < 6; i++) {
            if (i < 3) {  // 假设最多支持6层，但目前只使用3层
                if (useInternalPWM) {
                    ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(i, params[i]);
                } else {
                    ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(i, params[i]);
                }
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
    if (modeName == "Rainbow" || modeName == "Idle" || modeName == "Heatup") {  // 将"Alternate"改为"Heatup"
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
    
    // 更新连接状态和活动时间
    isConnected = true;
    lastActivityTime = millis();
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

/**
 * @brief 检查蓝牙连接状态
 * 
 * @return true 如果连接正常
 * @return false 如果连接已断开
 */
bool BluetoothController::checkConnection() {
    // 在实际连接上有活动
    if (BT.available()) {
        lastActivityTime = millis();
        return true;
    }
    
    // 如果曾经收到过数据但现在超时，认为断开
    if (isConnected && (millis() - lastActivityTime > disconnectTimeout)) {
        return false;
    }
    
    // 保持当前连接状态
    return isConnected;
}

/**
 * @brief 处理蓝牙断开连接状态
 */
void BluetoothController::handleDisconnect() {
    // 设置所有舵机为最小角度
    for (uint8_t layer = 0; layer < 3; layer++) { // 假设有3层
        if (useInternalPWM) {
            ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(layer, 0);
        } else {
            ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(layer, 0);
        }
    }
    
    // 设置所有LED为蓝色常亮
    uint32_t blueColor = lightBelt->wheel(170); // 蓝色对应wheel大约170
    lightBelt->setAllLayersColor(blueColor);
}

/**
 * @brief 设置蓝牙断开连接超时时间
 * 
 * @param timeoutMs 超时时间（毫秒）
 */
void BluetoothController::setDisconnectTimeout(uint32_t timeoutMs) {
    disconnectTimeout = timeoutMs;
}

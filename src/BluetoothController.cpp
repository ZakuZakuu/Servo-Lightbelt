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
    } else if (currentMode == "Heatup") {
        // 执行Heatup模式
        uint32_t timeNow = millis();
        uint32_t onColor = 0xFF0000;  // 红色
        
        // 计算各层舵机角度
        for (uint8_t layer = 0; layer < 3; layer++) {  // 最多3层舵机
            float phase = (timeNow % periodMs) / (float)periodMs;
            
            // 对偶数层反相，实现交替效果 (相位差半个周期)
            if (layer % 2 == 1) {
                phase = (phase + 0.5) > 1.0 ? (phase - 0.5) : (phase + 0.5);
            }
            
            // 计算映射到0-1023的值
            int mappedValue;
            if (phase < 0.5) {
                mappedValue = (phase * 2) * 1023;
            } else {
                mappedValue = (1.0 - (phase - 0.5) * 2) * 1023;
            }
            
            // 使用公有方法setLayerAngleFromValue而不是私有方法setLayerAngle
            if (useInternalPWM) {
                ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(layer, mappedValue);
            } else {
                ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(layer, mappedValue);
            }
            
            // 计算亮度值，与舵机角度同步变化
            uint8_t brightness = map(mappedValue, 0, 1023, 0, 255);
            
            // 设置对应层的灯带颜色
            uint32_t dimmedColor = lightBelt->dimColor(onColor, brightness);
            lightBelt->setLayerColor(layer, dimmedColor);
        }
    } else if (currentMode == "Cooldown") {
        // 执行Cooldown模式
        static uint8_t currentLayer = 0;
        static uint32_t startTime = 0;
        
        // 自动获取舵机层数
        uint8_t totalLayers = 0;
        if (useInternalPWM) {
            totalLayers = ((ServoPlatformInter*)servoPlatform)->getLayers();
        } else {
            totalLayers = ((ServoPlatform*)servoPlatform)->getLayers();
        }
        
        // 计算每层冷却时间 = 总时间 / 层数
        const uint32_t totalCooldownTime = 30000;  // 总冷却时间30秒
        const uint32_t layerCooldownTime = totalCooldownTime / totalLayers;  // 每层冷却时间
        const uint32_t orangeColor = 0xFF8800;  // 橙黄色
        
        // 如果是第一次执行或刚刚重置
        if (currentLayer == 0 && startTime == 0) {
            // 设置所有舵机为最大角度
            for (uint8_t layer = 0; layer < totalLayers; layer++) {
                if (useInternalPWM) {
                    ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(layer, 1023);
                } else {
                    ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(layer, 1023);
                }
            }
            
            // 设置所有灯为橙黄色最亮
            for (uint8_t layer = 0; layer < totalLayers; layer++) {
                lightBelt->setLayerColor(layer, orangeColor);
            }
            
            // 记录开始时间
            startTime = millis();
            Serial.println("Cooldown mode started - all layers set to maximum");
            Serial.print("Total cooldown time: ");
            Serial.print(totalCooldownTime / 1000);
            Serial.print("s, Time per layer: ");
            Serial.print(layerCooldownTime / 1000);
            Serial.println("s");
        }
        
        // 如果当前层有效
        if (currentLayer < totalLayers) {
            // 计算当前层冷却的进度 (0.0 - 1.0)
            uint32_t elapsedTime = millis() - startTime;
            float progress = min(1.0f, (float)elapsedTime / layerCooldownTime);
            
            // 从最高层开始冷却，即索引反向
            uint8_t layer = totalLayers - 1 - currentLayer;
            
            // 计算当前角度 (从最大值逐渐减小到最小值)
            int angleValue = 1023 * (1.0f - progress);
            
            // 设置当前层舵机角度
            if (useInternalPWM) {
                ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(layer, angleValue);
            } else {
                ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(layer, angleValue);
            }
            
            // 计算亮度值（由最亮变为最暗）
            uint8_t brightness = 255 * (1.0f - progress);
            
            // 设置当前层灯带亮度
            uint32_t dimmedColor = lightBelt->dimColor(orangeColor, brightness);
            lightBelt->setLayerColor(layer, dimmedColor);
            
            // 如果当前层完成冷却
            if (progress >= 1.0f) {
                // 确保完全冷却到最小值
                if (useInternalPWM) {
                    ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(layer, 0);
                } else {
                    ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(layer, 0);
                }
                
                // 确保灯光完全变暗
                lightBelt->setLayerColor(layer, 0); // 完全熄灭
                
                // 移至下一层
                currentLayer++;
                startTime = millis();
                
                if (currentLayer < totalLayers) {
                    Serial.print("Cooling down layer ");
                    Serial.print(totalLayers - currentLayer);
                    Serial.print(" (");
                    Serial.print((currentLayer) * 100 / totalLayers);
                    Serial.println("% completed)");
                }
            }
        } else {
            // 所有层都已冷却完毕，切换回Standby模式
            Serial.println("Cooldown completed, switching to Standby mode");
            
            // 重置Cooldown状态以便下次使用
            currentLayer = 0;
            startTime = 0;
            
            // 切换到Standby模式
            setPresetMode("Standby");
        }
    } else if (currentMode == "Standby") {
        // 获取舵机层数
        uint8_t totalLayers = 0;
        if (useInternalPWM) {
            totalLayers = ((ServoPlatformInter*)servoPlatform)->getLayers();
        } else {
            totalLayers = ((ServoPlatform*)servoPlatform)->getLayers();
        }
        
        // 设置所有舵机为最小角度
        for (uint8_t layer = 0; layer < totalLayers; layer++) {
            if (useInternalPWM) {
                ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(layer, 0);
            } else {
                ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(layer, 0);
            }
        }
        
        // 蓝色呼吸灯效果
        uint32_t blueColor = 0x0000FF; // 纯蓝色
        lightBelt->breathing(blueColor, 3000); // 3秒周期的呼吸效果
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
    } else if (currentMode == "Follow") {
        // 获取舵机层数
        uint8_t totalLayers = 0;
        if (useInternalPWM) {
            totalLayers = ((ServoPlatformInter*)servoPlatform)->getLayers();
        } else {
            totalLayers = ((ServoPlatform*)servoPlatform)->getLayers();
        }
        
        // 在Follow模式下，根据接收到的参数实时设置舵机角度和灯光效果
        for (int i = 0; i < totalLayers && i < 6; i++) {
            // 层号反向映射：第一位对应最后一层，以此类推
            int reversedLayer = totalLayers - 1 - i;
            
            // 设置舵机角度
            if (useInternalPWM) {
                ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(reversedLayer, params[i]);
            } else {
                ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(reversedLayer, params[i]);
            }
            
            // 添加灯带灯光效果
            // 1. 亮度和Follow参数成正比
            uint8_t brightness = map(params[i], 0, 1023, 0, 255);
            
            // 2. 蓝色程度和参数成正比，越大越蓝，越小越白
            // 从白色(0xFFFFFF)到蓝色(0x0000FF)的渐变
            uint8_t blueValue = 255;  // 蓝色分量保持255
            uint8_t whiteValue = 255 - map(params[i], 0, 1023, 0, 255);  // 红绿分量随参数减小
            
            // 构建RGB颜色
            uint32_t color = (whiteValue << 16) | (whiteValue << 8) | blueValue;
            
            // 调整亮度
            uint32_t adjustedColor = lightBelt->dimColor(color, brightness);
            
            // 设置灯带颜色（同样使用反向映射的层号）
            lightBelt->setLayerColor(reversedLayer, adjustedColor);
        }
    }
}

/**
 * @brief 处理接收到的命令字符串
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
    
    // 舵机角度反转命令
    if (modeName == "ReverseAngle") {
        if (firstSeparator + 1 < command.length()) {
            int value = command.substring(firstSeparator + 1).toInt();
            bool reverse = (value != 0);
            if (useInternalPWM) {
                ((ServoPlatformInter*)servoPlatform)->setReverseAngle(reverse);
            } else {
                ((ServoPlatform*)servoPlatform)->setReverseAngle(reverse);
            }
            Serial.print("舵机角度反转模式: ");
            Serial.println(reverse ? "开启" : "关闭");
            String response = "ReverseAngle=" + String(reverse ? "ON" : "OFF");
            BT.println(response);
        }
        return;
    }
    
    // LED亮度设置命令
    if (modeName == "SetBrightness") {
        if (firstSeparator + 1 < command.length()) {
            float brightness = command.substring(firstSeparator + 1).toFloat();
            lightBelt->setMaxBrightness(brightness);
            Serial.print("LED亮度设置为: ");
            Serial.println(brightness);
            String response = "Brightness=" + String(brightness);
            BT.println(response);
        }
        return;
    }
    
    // 预设模式处理
    if (modeName == "Rainbow" || modeName == "Idle" || modeName == "Heatup" || 
        modeName == "Cooldown" || modeName == "Standby") {
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

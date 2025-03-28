/**
 * @file SerialController.cpp
 * @brief 串口控制器类实现
 * 
 * @details 实现通过串口接收命令，控制LED灯带和舵机平台。
 * 支持与蓝牙控制器类似的功能和命令格式，但不处理连接断开。
 */

#include "SerialController.h"

/**
 * @brief 构造函数 - 使用内部PWM
 */
SerialController::SerialController(LightBelt* lightBeltPtr, ServoPlatformInter* servoPlatformPtr, uint32_t cycleTimeMs)
    : lightBelt(lightBeltPtr), servoPlatform(servoPlatformPtr), periodMs(cycleTimeMs) {
    useInternalPWM = true;
    strcpy(currentMode, "Idle");  // 默认为Idle模式
    
    // 初始化参数
    for (int i = 0; i < 6; i++) {
        params[i] = 512;  // 默认中间位置
    }
    
    cmdIndex = 0;
}

/**
 * @brief 构造函数 - 使用外部舵机驱动
 */
SerialController::SerialController(LightBelt* lightBeltPtr, ServoPlatform* servoPlatformPtr, uint32_t cycleTimeMs)
    : lightBelt(lightBeltPtr), servoPlatform(servoPlatformPtr), periodMs(cycleTimeMs) {
    useInternalPWM = false;
    strcpy(currentMode, "Idle");  // 默认为Idle模式
    
    // 初始化参数
    for (int i = 0; i < 6; i++) {
        params[i] = 512;  // 默认中间位置
    }
    
    cmdIndex = 0;
}

/**
 * @brief 初始化串口控制
 */
void SerialController::begin() {
    // Serial已在main.cpp中初始化，这里无需再次初始化
    Serial.println("Serial control initialized");
    Serial.println("You can control the device by sending commands via serial");
    Serial.println("Command format: Mode|param1|param2|...");
    Serial.println("Default mode is Idle");
    
    // 立即执行Idle模式
    executeIdleMode();
}

/**
 * @brief 更新处理串口命令
 */
void SerialController::update() {
    // 处理串口数据 (非阻塞)
    while (Serial.available()) {
        char c = Serial.read();
        
        // 回车或换行表示命令结束
        if (c == '\r' || c == '\n') {
            if (cmdIndex > 0) {
                // 添加字符串结束符
                cmdBuffer[cmdIndex] = '\0';
                
                // 处理命令
                Serial.print("Command received: ");
                Serial.println(cmdBuffer);
                processCommand();
                
                // 重置命令缓冲区
                cmdIndex = 0;
            }
        } 
        // 添加字符到缓冲区
        else if (cmdIndex < sizeof(cmdBuffer) - 1) {
            cmdBuffer[cmdIndex++] = c;
        }
    }
    
    // 根据当前模式执行对应操作
    if (modeEquals("Rainbow")) {
        lightBelt->rainbowCycle(periodMs);
        
        if (useInternalPWM) {
            ((ServoPlatformInter*)servoPlatform)->sweepAllLayers(periodMs, 30.0);
        } else {
            ((ServoPlatform*)servoPlatform)->sweepAllLayers(periodMs, 30.0);
        }
    }
    else if (modeEquals("Idle")) {
        executeIdleMode();
    }
    else if (modeEquals("Heatup")) {
        executeHeatupMode();
    }
    else if (modeEquals("Cooldown")) {
        executeCooldownMode();
    }
    else if (modeEquals("Standby")) {
        executeStandbyMode();
    }
    else if (modeEquals("Follow")) {
        // 获取舵机层数
        uint8_t totalServoLayers = 0;
        if (useInternalPWM) {
            totalServoLayers = ((ServoPlatformInter*)servoPlatform)->getLayers();
        } else {
            totalServoLayers = ((ServoPlatform*)servoPlatform)->getLayers();
        }
        
        // 获取灯带总层数
        uint8_t totalLightLayers = lightBelt->getLayers();
        
        // 实时控制舵机角度和灯光效果
        for (int i = 0; i < totalServoLayers && i < 6; i++) {
            // 层号反向映射：第一位对应最后一层，以此类推
            int reversedLayer = totalServoLayers - 1 - i;
            
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
            
            // 设置灯带颜色，每层舵机对应两层灯带
            if (totalLightLayers >= totalServoLayers * 2) {
                // 层号反向映射，对应两层灯带
                uint8_t lightLayer1 = reversedLayer * 2;
                uint8_t lightLayer2 = reversedLayer * 2 + 1;
                
                if (lightLayer1 < totalLightLayers) {
                    lightBelt->setLayerColor(lightLayer1, adjustedColor);
                }
                
                if (lightLayer2 < totalLightLayers) {
                    lightBelt->setLayerColor(lightLayer2, adjustedColor);
                }
            } else {
                // 灯带层数与舵机层数相同或更少的情况
                if (reversedLayer < totalLightLayers) {
                    lightBelt->setLayerColor(reversedLayer, adjustedColor);
                }
            }
        }
    }
}

/**
 * @brief 执行Idle模式
 */
void SerialController::executeIdleMode() {
    static bool isInitialReset = true;
    static uint32_t resetStartTime = 0;
    
    // 获取舵机层数
    uint8_t totalServoLayers = 0;
    if (useInternalPWM) {
        totalServoLayers = ((ServoPlatformInter*)servoPlatform)->getLayers();
    } else {
        totalServoLayers = ((ServoPlatform*)servoPlatform)->getLayers();
    }
    
    // 白色呼吸灯效果（所有层相同颜色）
    uint32_t whiteColor = 0xFFFFFF;
    lightBelt->breathing(whiteColor, 3000);
    
    // 如果是第一次进入Idle模式，或者模式刚刚切换为Idle
    if (isInitialReset) {
        // 设置开始时间
        if (resetStartTime == 0) {
            resetStartTime = millis();
            
            // 设置所有舵机为最小角度
            Serial.println("Resetting servos to minimum angle...");
            for (uint8_t layer = 0; layer < totalServoLayers; layer++) {
                if (useInternalPWM) {
                    ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(layer, 0);
                } else {
                    ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(layer, 0);
                }
            }
        }
        
        // 检查是否已经过了1秒
        if (millis() - resetStartTime >= 1000) {
            // 1秒后切换到往复运动状态
            isInitialReset = false;
            resetStartTime = 0;
            Serial.println("Starting sweep motion with phase difference...");
        }
    } else {
        // 复位完成后，执行带相位差的往复运动
        if (useInternalPWM) {
            ((ServoPlatformInter*)servoPlatform)->sweepAllLayers(periodMs, 30.0);
        } else {
            ((ServoPlatform*)servoPlatform)->sweepAllLayers(periodMs, 30.0);
        }
    }
}

/**
 * @brief 执行Heatup模式
 * @details 舵机相位差半个周期往返运动，灯带呼吸效果与对应舵机同步
 */
void SerialController::executeHeatupMode() {
    uint32_t timeNow = millis();
    
    // 定义颜色 - 使用红色表示热量
    uint32_t onColor = 0xFF0000;  // 红色
    
    // 获取舵机总层数
    uint8_t totalServoLayers = 0;
    if (useInternalPWM) {
        totalServoLayers = ((ServoPlatformInter*)servoPlatform)->getLayers();
    } else {
        totalServoLayers = ((ServoPlatform*)servoPlatform)->getLayers();
    }
    
    // 获取灯带总层数，通常是舵机层数的2倍
    uint8_t totalLightLayers = lightBelt->getLayers();
    
    // 计算各层舵机角度
    for (uint8_t servoLayer = 0; servoLayer < totalServoLayers; servoLayer++) {
        float phase = (timeNow % periodMs) / (float)periodMs;
        
        // 对偶数层反相，实现交替效果 (相位差半个周期)
        if (servoLayer % 2 == 1) {
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
            ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(servoLayer, mappedValue);
        } else {
            ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(servoLayer, mappedValue);
        }
        
        // 计算亮度值，与舵机角度同步变化
        uint8_t brightness = map(mappedValue, 0, 1023, 0, 255);
        
        // 设置对应层的灯带颜色
        uint32_t dimmedColor = lightBelt->dimColor(onColor, brightness);
        
        // 针对灯带层数是舵机层数的2倍的情况，将两层灯带对应一层舵机
        if (totalLightLayers >= totalServoLayers * 2) {
            // 每层舵机对应两层灯带
            uint8_t lightLayer1 = servoLayer * 2;
            uint8_t lightLayer2 = servoLayer * 2 + 1;
            
            if (lightLayer1 < totalLightLayers) {
                lightBelt->setLayerColor(lightLayer1, dimmedColor);
            }
            
            if (lightLayer2 < totalLightLayers) {
                lightBelt->setLayerColor(lightLayer2, dimmedColor);
            }
        } else {
            // 灯带层数与舵机层数相同或更少的情况
            if (servoLayer < totalLightLayers) {
                lightBelt->setLayerColor(servoLayer, dimmedColor);
            }
        }
    }
}

/**
 * @brief 执行Cooldown模式
 * @details 从最高层开始，每层依次由最大角度变为最小角度，灯光同步由亮变暗
 * 总时间为30秒，完成后切换到Standby模式
 */
void SerialController::executeCooldownMode() {
    static uint8_t* pCurrentLayer = getCooldownStatePtr();
    static uint32_t* pStartTime = getCooldownStartTimePtr();
    
    // 获取当前层数(索引从0开始)
    uint8_t currentLayer = *pCurrentLayer;
    
    // 自动获取舵机层数
    uint8_t totalServoLayers = 0;
    if (useInternalPWM) {
        totalServoLayers = ((ServoPlatformInter*)servoPlatform)->getLayers();
    } else {
        totalServoLayers = ((ServoPlatform*)servoPlatform)->getLayers();
    }
    
    // 获取灯带总层数
    uint8_t totalLightLayers = lightBelt->getLayers();
    
    // 计算每层冷却时间 = 总时间 / 层数
    const uint32_t totalCooldownTime = 30000;  // 总冷却时间30秒
    const uint32_t layerCooldownTime = totalCooldownTime / totalServoLayers;  // 每层冷却时间
    const uint32_t orangeColor = 0xFF8800;  // 橙黄色
    
    // 如果是第一次执行或刚刚重置
    if (currentLayer == 0 && *pStartTime == 0) {
        // 设置所有舵机为最大角度
        for (uint8_t layer = 0; layer < totalServoLayers; layer++) {
            if (useInternalPWM) {
                ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(layer, 1023);
            } else {
                ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(layer, 1023);
            }
        }
        
        // 设置所有灯为橙黄色最亮
        for (uint8_t layer = 0; layer < totalLightLayers; layer++) {
            lightBelt->setLayerColor(layer, orangeColor);
        }
        
        // 记录开始时间
        *pStartTime = millis();
        Serial.println("Cooldown mode started - all layers set to maximum");
        Serial.print("Total cooldown time: ");
        Serial.print(totalCooldownTime / 1000);
        Serial.print("s, Time per layer: ");
        Serial.print(layerCooldownTime / 1000);
        Serial.println("s");
    }
    
    // 如果当前层有效
    if (currentLayer < totalServoLayers) {
        // 计算当前层冷却的进度 (0.0 - 1.0)
        uint32_t elapsedTime = millis() - *pStartTime;
        float progress = min(1.0f, (float)elapsedTime / layerCooldownTime);
        
        // 从最高层开始冷却，即索引反向
        uint8_t servoLayer = totalServoLayers - 1 - currentLayer;
        
        // 计算当前角度 (从最大值逐渐减小到最小值)
        int angleValue = 1023 * (1.0f - progress);
        
        // 设置当前层舵机角度
        if (useInternalPWM) {
            ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(servoLayer, angleValue);
        } else {
            ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(servoLayer, angleValue);
        }
        
        // 计算亮度值（由最亮变为最暗）
        uint8_t brightness = 255 * (1.0f - progress);
        
        // 设置对应层灯带亮度
        uint32_t dimmedColor = lightBelt->dimColor(orangeColor, brightness);
        
        // 针对灯带层数是舵机层数的2倍的情况
        if (totalLightLayers >= totalServoLayers * 2) {
            // 每层舵机对应两层灯带
            uint8_t lightLayer1 = servoLayer * 2;
            uint8_t lightLayer2 = servoLayer * 2 + 1;
            
            if (lightLayer1 < totalLightLayers) {
                lightBelt->setLayerColor(lightLayer1, dimmedColor);
            }
            
            if (lightLayer2 < totalLightLayers) {
                lightBelt->setLayerColor(lightLayer2, dimmedColor);
            }
        } else {
            // 灯带层数与舵机层数相同或更少的情况
            if (servoLayer < totalLightLayers) {
                lightBelt->setLayerColor(servoLayer, dimmedColor);
            }
        }
        
        // 如果当前层完成冷却
        if (progress >= 1.0f) {
            // 确保完全冷却到最小值
            if (useInternalPWM) {
                ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(servoLayer, 0);
            } else {
                ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(servoLayer, 0);
            }
            
            // 确保灯光完全变暗
            // 针对灯带层数是舵机层数的2倍的情况
            if (totalLightLayers >= totalServoLayers * 2) {
                // 每层舵机对应两层灯带
                uint8_t lightLayer1 = servoLayer * 2;
                uint8_t lightLayer2 = servoLayer * 2 + 1;
                
                if (lightLayer1 < totalLightLayers) {
                    lightBelt->setLayerColor(lightLayer1, 0);
                }
                
                if (lightLayer2 < totalLightLayers) {
                    lightBelt->setLayerColor(lightLayer2, 0);
                }
            } else {
                // 灯带层数与舵机层数相同或更少的情况
                if (servoLayer < totalLightLayers) {
                    lightBelt->setLayerColor(servoLayer, 0);
                }
            }
            
            // 移至下一层
            (*pCurrentLayer)++;
            *pStartTime = millis();
            
            if (*pCurrentLayer < totalServoLayers) {
                Serial.print("Cooling down layer ");
                Serial.print(totalServoLayers - *pCurrentLayer);
                Serial.print(" (");
                Serial.print((*pCurrentLayer) * 100 / totalServoLayers);
                Serial.println("% completed)");
            }
        }
    } else {
        // 所有层都已冷却完毕，切换回Standby模式
        Serial.println("Cooldown completed, switching to Standby mode");
        
        // 重置Cooldown状态以便下次使用
        *pCurrentLayer = 0;
        *pStartTime = 0;
        
        // 切换到Standby模式
        setPresetMode("Standby");
    }
}

/**
 * @brief 执行Standby模式
 * @details 所有舵机回到最小值，全部灯带为蓝色呼吸灯样式
 */
void SerialController::executeStandbyMode() {
    // 获取舵机层数
    uint8_t totalServoLayers = 0;
    if (useInternalPWM) {
        totalServoLayers = ((ServoPlatformInter*)servoPlatform)->getLayers();
    } else {
        totalServoLayers = ((ServoPlatform*)servoPlatform)->getLayers();
    }
    
    // 设置所有舵机为最小角度
    for (uint8_t layer = 0; layer < totalServoLayers; layer++) {
        if (useInternalPWM) {
            ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(layer, 0);
        } else {
            ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(layer, 0);
        }
    }
    
    // 蓝色呼吸灯效果（所有层相同颜色）
    uint32_t blueColor = 0x0000FF; // 纯蓝色
    lightBelt->breathing(blueColor, 3000); // 3秒周期的呼吸效果
}

/**
 * @brief 处理命令
 */
void SerialController::processCommand() {
    // 解析模式名和参数
    char* token = strtok(cmdBuffer, "|");
    if (!token) {
        Serial.println("Error: Invalid command format!");
        return;
    }
    
    // 处理特殊命令
    if (strcmp(token, "Lookup") == 0) {
        sendStatus();
        return;
    }
    
    // 舵机角度反转命令
    if (strcmp(token, "ReverseAngle") == 0) {
        token = strtok(NULL, "|");
        if (token) {
            bool reverse = (parseIntParam(token) != 0);
            if (useInternalPWM) {
                ((ServoPlatformInter*)servoPlatform)->setReverseAngle(reverse);
            } else {
                ((ServoPlatform*)servoPlatform)->setReverseAngle(reverse);
            }
            Serial.print("Servo angle reverse mode: ");
            Serial.println(reverse ? "ON" : "OFF");
        }
        return;
    }
    
    // LED亮度设置命令
    if (strcmp(token, "SetBrightness") == 0) {
        token = strtok(NULL, "|");
        if (token) {
            float brightness = atof(token); // 将字符串转换为浮点数
            lightBelt->setMaxBrightness(brightness);
            Serial.print("LED max brightness set to: ");
            Serial.println(brightness);
        }
        return;
    }
    
    // 预设模式
    if (strcmp(token, "Rainbow") == 0 || strcmp(token, "Idle") == 0 || 
        strcmp(token, "Heatup") == 0 || strcmp(token, "Cooldown") == 0 ||
        strcmp(token, "Standby") == 0) {
        setPresetMode(token);
        return;
    }
    
    // 控制模式
    if (strcmp(token, "Follow") == 0) {
        int newParams[6] = {0};
        
        // 解析参数
        for (int i = 0; i < 6; i++) {
            token = strtok(NULL, "|");
            if (token) {
                newParams[i] = parseIntParam(token);
            } else {
                break;
            }
        }
        
        setControlMode("Follow", newParams);
    }
}

/**
 * @brief 设置预设模式
 */
void SerialController::setPresetMode(const char* modeName) {
    // 如果从其他模式切换到Idle模式，需要重置状态
    if (strcmp(modeName, "Idle") == 0 && !modeEquals("Idle")) {
        // 重置Idle模式的初始状态变量
        static bool* pIsInitialReset = getIdleResetFlag();
        static uint32_t* pResetStartTime = getIdleStartTimeFlag();
        
        *pIsInitialReset = true;
        *pResetStartTime = 0;
    }
    
    // 如果切换到Cooldown模式，重置状态
    if (strcmp(modeName, "Cooldown") == 0) {
        uint8_t* pCurrentLayer = getCooldownStatePtr();
        uint32_t* pStartTime = getCooldownStartTimePtr();
        *pCurrentLayer = 0;
        *pStartTime = 0;
    }
    
    strcpy(currentMode, modeName);
    
    Serial.print("Setting preset mode: ");
    Serial.println(modeName);
    
    // 发送确认
    char response[20] = "Mode=";
    strcat(response, modeName);
    Serial.println(response);
}

/**
 * @brief 设置控制模式
 */
void SerialController::setControlMode(const char* modeName, int* parameters) {
    strcpy(currentMode, "Follow");
    
    // 更新参数
    for (int i = 0; i < 6; i++) {
        params[i] = parameters[i];
    }
    
    Serial.print("Setting control mode: Follow with parameters: ");
    for (int i = 0; i < 6; i++) {
        Serial.print(params[i]);
        Serial.print(" ");
    }
    Serial.println();
    
    // 发送确认
    Serial.println("Mode=Follow");
}

/**
 * @brief 发送状态
 */
void SerialController::sendStatus() {
    // 构建响应
    char response[64] = {0};
    strcpy(response, currentMode);
    
    for (int i = 0; i < 6; i++) {
        char paramStr[8];
        sprintf(paramStr, "|%d", params[i]);
        strcat(response, paramStr);
    }
    
    Serial.println(response);
    Serial.print("Status sent: ");
    Serial.println(response);
}

/**
 * @brief 比较模式名称
 */
bool SerialController::modeEquals(const char* modeName) {
    return strcmp(currentMode, modeName) == 0;
}

/**
 * @brief 解析整数参数
 */
int SerialController::parseIntParam(const char* str) {
    return atoi(str);
}

/**
 * @brief 获取Idle模式复位标志指针
 * 使用静态变量保存状态
 */
bool* SerialController::getIdleResetFlag() {
    static bool isInitialReset = true;
    return &isInitialReset;
}

/**
 * @brief 获取Idle模式开始时间指针
 * 使用静态变量保存状态
 */
uint32_t* SerialController::getIdleStartTimeFlag() {
    static uint32_t resetStartTime = 0;
    return &resetStartTime;
}

/**
 * @brief 获取Cooldown模式状态指针
 * 使用静态变量保存状态
 */
uint8_t* SerialController::getCooldownStatePtr() {
    static uint8_t currentLayer = 0;
    return &currentLayer;
}

/**
 * @brief 获取Cooldown模式开始时间指针
 * 使用静态变量保存状态
 */
uint32_t* SerialController::getCooldownStartTimePtr() {
    static uint32_t startTime = 0;
    return &startTime;
}

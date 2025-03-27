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
    else if (modeEquals("Follow")) {
        // 实时控制舵机角度
        for (int i = 0; i < 3; i++) {
            if (useInternalPWM) {
                ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(i, params[i]);
            } else {
                ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(i, params[i]);
            }
        }
    }
}   else if (modeEquals("Heatup")) {
        executeHeatupMode();
/** }
 * @brief 执行Idle模式
 */
void SerialController::executeIdleMode() {
    static bool isInitialReset = true;
    static uint32_t resetStartTime = 0;
     SerialController::executeIdleMode() {
    // 白色呼吸灯效果l isInitialReset = true;
    uint32_t whiteColor = 0xFFFFFF;= 0;
    lightBelt->breathing(whiteColor, 3000);
    // 白色呼吸灯效果
    // 如果是第一次进入Idle模式，或者模式刚刚切换为Idle
    if (isInitialReset) {whiteColor, 3000);
        // 设置开始时间
        if (resetStartTime == 0) {e
            resetStartTime = millis();
            置开始时间
            // 设置所有舵机为最小角度 == 0) {
            Serial.println("Resetting servos to minimum angle...");
            for (uint8_t layer = 0; layer < 3; layer++) {
                if (useInternalPWM) {
                    ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(layer, 0);
                } else { layer = 0; layer < 3; layer++) {
                    ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(layer, 0);
                }   ((ServoPlatformInter*)servoPlatform)->setLayerAngleFromValue(layer, 0);
            }   } else {
        }           ((ServoPlatform*)servoPlatform)->setLayerAngleFromValue(layer, 0);
                }
        // 检查是否已经过了1秒
        if (millis() - resetStartTime >= 1000) {
            // 1秒后切换到往复运动状态
            isInitialReset = false;
            resetStartTime = 0;rtTime >= 1000) {
            Serial.println("Starting sweep motion with phase difference...");
        }   isInitialReset = false;
    } else {resetStartTime = 0;
        // 复位完成后，执行带相位差的往复运动Starting sweep motion with phase difference...");
        if (useInternalPWM) {
            ((ServoPlatformInter*)servoPlatform)->sweepAllLayers(periodMs, 30.0);
        } else {，执行带相位差的往复运动
            ((ServoPlatform*)servoPlatform)->sweepAllLayers(periodMs, 30.0);
        }   ((ServoPlatformInter*)servoPlatform)->sweepAllLayers(periodMs, 30.0);
    }   } else {
}           ((ServoPlatform*)servoPlatform)->sweepAllLayers(periodMs, 30.0);
        }
/** }
 * @brief 执行Heatup模式
 * @details 舵机相位差半个周期往返运动，灯带呼吸效果与对应舵机同步
 */
void SerialController::executeHeatupMode() {tup模式
    uint32_t timeNow = millis();
    
    // 定义颜色 - 使用红色表示热量
    uint32_t onColor = 0xFF0000;  // 红色meNow = millis();
    
    // 获取最小和最大角度// 定义颜色 - 使用红色表示热量
    uint8_t minAngle = 0;onColor = 0xFF0000;  // 红色
    uint8_t maxAngle = 180;
    
    // 计算各层舵机角度t layer = 0; layer < 3; layer++) {  // 最多3层舵机
    for (uint8_t layer = 0; layer < 3; layer++) {  // 最多3层舵机   float phase = (timeNow % periodMs) / (float)periodMs;
        float phase = (timeNow % periodMs) / (float)periodMs;    
        对偶数层反相，实现交替效果 (相位差半个周期)
        // 对偶数层反相，实现交替效果 (相位差半个周期)
        if (layer % 2 == 1) {0.5) > 1.0 ? (phase - 0.5) : (phase + 0.5);
            phase = (phase + 0.5) > 1.0 ? (phase - 0.5) : (phase + 0.5);
        }   
            // 计算角度：0.0-1.0映射到min-max
        // 计算角度：0.0-1.0映射到min-maxat angle;
        float angle;
        if (phase < 0.5) {(maxAngle - minAngle) * (phase * 2);
            angle = minAngle + (maxAngle - minAngle) * (phase * 2);} else {
        } else {le = maxAngle - (maxAngle - minAngle) * ((phase - 0.5) * 2);
            angle = maxAngle - (maxAngle - minAngle) * ((phase - 0.5) * 2);
        }
        
        // 设置舵机角度
        if (useInternalPWM) {latformInter*)servoPlatform)->setLayerAngle(layer, angle);
            ((ServoPlatformInter*)servoPlatform)->setLayerAngle(layer, angle);
        } else {(ServoPlatform*)servoPlatform)->setLayerAngle(layer, angle);
            ((ServoPlatform*)servoPlatform)->setLayerAngle(layer, angle);
        }
        
        // 计算0-255的亮度值，与舵机角度同步变化   uint8_t brightness = map(angle, 0, 180, 0, 255);
        uint8_t brightness = map(angle, minAngle, maxAngle, 0, 255);       
                // 每层舵机对应两层灯带
        // 每层舵机对应两层灯带     for (uint8_t i = 0; i < 2; i++) {
        for (uint8_t i = 0; i < 2; i++) {8_t ledLayer = layer * 2 + i;
            uint8_t ledLayer = layer * 2 + i;         if (ledLayer < lightBelt->getLayerCount()) {
            if (ledLayer < lightBelt->getLayerCount()) {
                // 根据亮度设置灯带颜色dColor = lightBelt->dimColor(onColor, brightness);
                uint32_t dimmedColor = lightBelt->dimColor(onColor, brightness);;
                lightBelt->setLayerColor(ledLayer, dimmedColor);
            }
        }
    }
}

/**rief 处理命令
 * @brief 处理命令
 */and() {
void SerialController::processCommand() {// 解析模式名和参数
    // 解析模式名和参数
    char* token = strtok(cmdBuffer, "|");
    if (!token) {    Serial.println("Error: Invalid command format!");
        Serial.println("Error: Invalid command format!");urn;
        return;
    }
    
    // 处理特殊命令   if (strcmp(token, "Lookup") == 0) {
    if (strcmp(token, "Lookup") == 0) {        sendStatus();
        sendStatus();     return;
        return;
    } 
    
    // 预设模式= 0 || strcmp(token, "Idle") == 0 || strcmp(token, "Heatup") == 0) {
    if (strcmp(token, "Rainbow") == 0 || strcmp(token, "Idle") == 0) {    setPresetMode(token);
        setPresetMode(token);urn;
        return;
    }
    / 控制模式
    // 控制模式if (strcmp(token, "Follow") == 0) {
    if (strcmp(token, "Follow") == 0) {
        int newParams[6] = {0};
        
        // 解析参数< 6; i++) {
        for (int i = 0; i < 6; i++) {       token = strtok(NULL, "|");
            token = strtok(NULL, "|");) {
            if (token) {            newParams[i] = parseIntParam(token);
                newParams[i] = parseIntParam(token); } else {
            } else {
                break;           }
            }        }
        }     
        trolMode("Follow", newParams);
        setControlMode("Follow", newParams); }
    }
}

/**
 * @brief 设置预设模式
 */ode(const char* modeName) {
void SerialController::setPresetMode(const char* modeName) {要重置状态
    // 如果从其他模式切换到Idle模式，需要重置状态deEquals("Idle")) {
    if (strcmp(modeName, "Idle") == 0 && !modeEquals("Idle")) {
        // 重置Idle模式的初始状态变量   static bool* pIsInitialReset = getIdleResetFlag();
        static bool* pIsInitialReset = getIdleResetFlag();    static uint32_t* pResetStartTime = getIdleStartTimeFlag();
        static uint32_t* pResetStartTime = getIdleStartTimeFlag();
        
        *pIsInitialReset = true;
        *pResetStartTime = 0;   }
    }    
     strcpy(currentMode, modeName);
    strcpy(currentMode, modeName);
     Serial.print("Setting preset mode: ");
    Serial.print("Setting preset mode: ");
    Serial.println(modeName);
       // 发送确认
    // 发送确认    char response[20] = "Mode=";
    char response[20] = "Mode="; strcat(response, modeName);
    strcat(response, modeName);ln(response);
    Serial.println(response);
}

/*** @brief 设置控制模式
 * @brief 设置控制模式 */
 */d SerialController::setControlMode(const char* modeName, int* parameters) {
void SerialController::setControlMode(const char* modeName, int* parameters) {"Follow");
    strcpy(currentMode, "Follow");
     // 更新参数
    // 更新参数
    for (int i = 0; i < 6; i++) {
        params[i] = parameters[i];
    }   
        Serial.print("Setting control mode: Follow with parameters: ");
    Serial.print("Setting control mode: Follow with parameters: "); for (int i = 0; i < 6; i++) {
    for (int i = 0; i < 6; i++) {ams[i]);
        Serial.print(params[i]);l.print(" ");
        Serial.print(" "); }
    }
    Serial.println();
    
    // 发送确认   Serial.println("Mode=Follow");
    Serial.println("Mode=Follow");}































 */ * @brief 解析整数参数/**}    return strcmp(currentMode, modeName) == 0;bool SerialController::modeEquals(const char* modeName) { */ * @brief 比较模式名称/**}    Serial.println(response);    Serial.print("Status sent: ");    Serial.println(response);        }        strcat(response, paramStr);        sprintf(paramStr, "|%d", params[i]);        char paramStr[8];    for (int i = 0; i < 6; i++) {        strcpy(response, currentMode);    char response[64] = {0};    // 构建响应void SerialController::sendStatus() { */ * @brief 发送状态/**}
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



}    return &resetStartTime;





    static uint32_t resetStartTime = 0;uint32_t* SerialController::getIdleStartTimeFlag() {


 */ * 使用静态变量保存状态 * @brief 获取Idle模式开始时间指针







/**}    return &isInitialReset;    static bool isInitialReset = true;bool* SerialController::getIdleResetFlag() {


 */ * 使用静态变量保存状态 * @brief 获取Idle模式复位标志指针


/**}    return atoi(str);int SerialController::parseIntParam(const char* str) { */


 * @brief 解析整数参数/**}    return strcmp(currentMode, modeName) == 0;bool SerialController::modeEquals(const char* modeName) {}

/**
 * @brief 比较模式名称
 */
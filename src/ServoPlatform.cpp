#include "ServoPlatform.h"

ServoPlatform::ServoPlatform(uint8_t numLayers, uint8_t i2cAddress, uint8_t minAng, uint8_t maxAng)
    : layers(numLayers), minAngle(minAng), maxAngle(maxAng), i2cAddress(i2cAddress) {
    servoMin = 150;  // 对应0度的脉冲计数值（可能需要校准）
    servoMax = 600;  // 对应180度的脉冲计数值（可能需要校准）
    
    for(int i = 0; i < 16; i++) {
        currentAngles[i] = minAngle;
    }
    
    sweepCompleted = false;
    sweepStartTime = 0;
}

uint8_t ServoPlatform::scanI2CAddress() {
    Serial.println("开始扫描I2C设备...");
    uint8_t error, address;
    uint8_t foundAddress = 0;
    int nDevices = 0;

    for(address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("发现I2C设备，地址: 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
            nDevices++;
            // 保存第一个找到的地址
            if (foundAddress == 0) {
                foundAddress = address;
            }
        }
    }
    
    if (nDevices == 0) {
        Serial.println("未找到I2C设备！");
        return 0;
    }
    
    return foundAddress;
}

void ServoPlatform::begin() {
    Serial.print("初始化I2C - SDA引脚: ");
    Serial.print(21);
    Serial.print(", SCL引脚: ");
    Serial.println(22);
    
    Wire.setPins(21, 22);  // 初始化I2C，设置SDA和SCL引脚
    Wire.begin();  // 开始I2C通信
    
    // 扫描并更新I2C地址
    uint8_t scannedAddress = scanI2CAddress();
    if (scannedAddress != 0) {
        i2cAddress = scannedAddress;
        Serial.print("使用扫描到的I2C地址: 0x");
        Serial.println(i2cAddress, HEX);
        pwm = Adafruit_PWMServoDriver(i2cAddress);
    } else {
        Serial.println("使用默认I2C地址: 0x40");
        pwm = Adafruit_PWMServoDriver(0x40);
    }
    
    pwm.begin();
    pwm.setPWMFreq(200);  // 标准舵机PWM频率
    delay(10);
    
    // 移除了自检程序调用
}

uint16_t ServoPlatform::angleToMicros(uint8_t angle) {
    return map(angle, 0, 180, servoMin, servoMax);
}

void ServoPlatform::setServoAngle(uint8_t servoNum, uint8_t angle) {
    if(servoNum >= layers * 2) return;
    pwm.setPWM(servoNum, 0, angleToMicros(angle));
    currentAngles[servoNum] = angle;
}

void ServoPlatform::setLayerAngle(uint8_t layer, uint8_t angle) {
    if(layer >= layers) return;
    setServoAngle(layer * 2, angle);      // 设置该层第一个舵机
    setServoAngle(layer * 2 + 1, angle);  // 设置该层第二个舵机
}

void ServoPlatform::sweepLayer(uint8_t layer, uint32_t periodMs) {
    if(layer >= layers) return;
    
    uint32_t timeNow = millis();
    float phase = (timeNow % periodMs) / (float)periodMs;
    float angle;
    
    if(phase < 0.5) {
        // 0-0.5: 从最小角度到最大角度
        Serial.println("0-0.5");
        angle = minAngle + (maxAngle - minAngle) * (phase * 2);
    } else {
        // 0.5-1: 从最大角度回到最小角度
        Serial.println("0.5-1");
        angle = maxAngle - (maxAngle - minAngle) * ((phase - 0.5) * 2);
    }
    
    setLayerAngle(layer, angle);
}

void ServoPlatform::sweepAllLayers(uint32_t periodMs, float phaseDiff) {
    for(uint8_t layer = 0; layer < layers; layer++) {
        uint32_t timeNow = millis();
        float layerPhaseOffset = (phaseDiff * layer) / 360.0f;  // 将相位差转换为0-1范围
        float adjustedTime = fmod(timeNow + (layerPhaseOffset * periodMs), periodMs);
        
        float phase = adjustedTime / (float)periodMs;
        float angle;
        
        if(phase < 0.5) {
            // Serial.println("0-0.5");
            angle = minAngle + (maxAngle - minAngle) * (phase * 2);
        } else {
            // Serial.println("0.5-1");
            angle = maxAngle - (maxAngle - minAngle) * ((phase - 0.5) * 2);
        }
        
        if (layer == 0) {
            Serial.print("Phase: ");
            Serial.print(phase);
            Serial.print(", Angle: ");
            Serial.println(angle);
        }
        setLayerAngle(layer, angle);
    }
}

bool ServoPlatform::sweepAllLayersOnce(uint32_t periodMs, float phaseDiff) {
    // 如果已经完成，直接返回true
    if (sweepCompleted) {
        return true;
    }
    
    // 第一次调用时记录开始时间
    if (sweepStartTime == 0) {
        sweepStartTime = millis();
    }
    
    uint32_t elapsedTime = millis() - sweepStartTime;
    
    // 检查是否已经完成一个周期
    if (elapsedTime >= periodMs) {
        // 完成后确保所有舵机回到初始位置
        for (uint8_t layer = 0; layer < layers; layer++) {
            setLayerAngle(layer, minAngle);
        }
        sweepCompleted = true;
        return true;
    }
    
    // 执行一次正常的扫描周期
    for (uint8_t layer = 0; layer < layers; layer++) {
        float layerPhaseOffset = (phaseDiff * layer) / 360.0f;
        float adjustedTime = fmod(elapsedTime + (layerPhaseOffset * periodMs), periodMs);
        
        float phase = adjustedTime / (float)periodMs;
        float angle;
        
        if (phase < 0.5) {
            angle = minAngle + (maxAngle - minAngle) * (phase * 2);
        } else {
            angle = maxAngle - (maxAngle - minAngle) * ((phase - 0.5) * 2);
        }
        
        setLayerAngle(layer, angle);
    }
    
    return false;
}

void ServoPlatform::resetSweep() {
    sweepCompleted = false;
    sweepStartTime = 0;
}

void ServoPlatform::servoSelfTest() {
    Serial.println("开始舵机自检...");
    
    // 先全部归零
    for(uint8_t layer = 0; layer < layers; layer++) {
        setLayerAngle(layer, minAngle);
    }
    delay(1000);

    // 依次测试每层舵机
    for(uint8_t layer = 0; layer < layers; layer++) {
        Serial.print("测试第 ");
        Serial.print(layer + 1);
        Serial.println(" 层舵机");
        
        // 转到最大角度
        setLayerAngle(layer, maxAngle);
        delay(500);
        // 转回最小角度
        setLayerAngle(layer, minAngle);
        delay(500);
    }
    
    Serial.println("舵机自检完成！");
}

#include "ServoPlatformInter.h"
#include "GlobalConfig.h"

// 定义舵机引脚，避开GPIO5
// 每层两个舵机，编号对应关系：
// 第1层: 舵机0(GPIO13), 舵机1(GPIO12)
// 第2层: 舵机2(GPIO14), 舵机3(GPIO27)
// 第3层: 舵机4(GPIO26), 舵机5(GPIO25)
// 第4层: 舵机6(GPIO33), 舵机7(GPIO32)
// 第5层: 舵机8(GPIO23), 舵机9(GPIO22)
// 第6层: 舵机10(GPIO19), 舵机11(GPIO18)
const uint8_t SERVO_PINS[] = {
    13, 12,  // Layer 1
    14, 27,  // Layer 2
    26, 25,  // Layer 3
    33, 32,  // Layer 4
    23, 22,  // Layer 5
    19, 18   // Layer 6
};

ServoPlatformInter::ServoPlatformInter(uint8_t numLayers, uint8_t minAng, uint8_t maxAng)
    : layers(numLayers), minAngle(minAng), maxAngle(maxAng) {
    minPulseWidth = 500;   // 0.5ms
    maxPulseWidth = 2500;  // 2.5ms
    
    for(int i = 0; i < 12; i++) {  // 初始化12个舵机
        currentAngles[i] = minAngle;
        servoPins[i] = SERVO_PINS[i];
    }
    
    sweepCompleted = false;
    sweepStartTime = 0;
    
    // 从全局配置初始化角度反转状态
    reverseAngle = REVERSE_SERVO_ANGLE;
}

void ServoPlatformInter::initPWM() {
    for(int i = 0; i < layers * 2; i++) {
        ledcSetup(i, 50, 16);  // 通道i，50Hz，16位分辨率
        ledcAttachPin(servoPins[i], i);
    }
}

void ServoPlatformInter::setServoPWM(uint8_t channel, uint16_t pulseWidth) {
    uint32_t duty = (uint32_t)(pulseWidth * 65536 / 20000);  // 将脉冲宽度转换为占空比
    ledcWrite(channel, duty);
}

uint16_t ServoPlatformInter::angleToPulseWidth(uint8_t angle) {
    return map(angle, 0, 180, minPulseWidth, maxPulseWidth);
}

void ServoPlatformInter::setServoAngle(uint8_t servoNum, uint8_t angle) {
    if(servoNum >= layers * 2) return;
    setServoPWM(servoNum, angleToPulseWidth(angle));
    currentAngles[servoNum] = angle;
}

void ServoPlatformInter::setLayerAngle(uint8_t layer, uint8_t angle) {
    if(layer >= layers) return;
    
    // 如果设置了角度反转，则反转角度
    if(reverseAngle) {
        angle = maxAngle - (angle - minAngle);
    }
    
    setServoAngle(layer * 2, angle);
    setServoAngle(layer * 2 + 1, angle);
}

void ServoPlatformInter::begin() {
    Serial.println("Initializing internal PWM servo control...");
    initPWM();
    delay(100);
    // 移除了自检调用
}

// 其他方法（sweepLayer, sweepAllLayers）与ServoPlatform完全相同
void ServoPlatformInter::sweepLayer(uint8_t layer, uint32_t periodMs) {
    if(layer >= layers) return;
    
    uint32_t timeNow = millis();
    float phase = (timeNow % periodMs) / (float)periodMs;
    float angle;
    
    if(phase < 0.5) {
        angle = minAngle + (maxAngle - minAngle) * (phase * 2);
    } else {
        angle = maxAngle - (maxAngle - minAngle) * ((phase - 0.5) * 2);
    }
    
    setLayerAngle(layer, angle);
}

void ServoPlatformInter::sweepAllLayers(uint32_t periodMs, float phaseDiff) {
    for(uint8_t layer = 0; layer < layers; layer++) {
        uint32_t timeNow = millis();
        float layerPhaseOffset = (phaseDiff * layer) / 360.0f;
        float adjustedTime = fmod(timeNow + (layerPhaseOffset * periodMs), periodMs);
        
        float phase = adjustedTime / (float)periodMs;
        float angle;
        
        if(phase < 0.5) {
            angle = minAngle + (maxAngle - minAngle) * (phase * 2);
        } else {
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

bool ServoPlatformInter::sweepAllLayersOnce(uint32_t periodMs, float phaseDiff) {
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

void ServoPlatformInter::resetSweep() {
    sweepCompleted = false;
    sweepStartTime = 0;
}

void ServoPlatformInter::servoSelfTest() {
    Serial.println("Starting servo self-test...");
    
    for(uint8_t layer = 0; layer < layers; layer++) {
        setLayerAngle(layer, minAngle);
    }
    delay(1000);

    for(uint8_t layer = 0; layer < layers; layer++) {
        Serial.print("Testing layer ");
        Serial.print(layer + 1);
        Serial.print(" servos (servo");
        Serial.print(layer*2);
        Serial.print("[GPIO");
        Serial.print(servoPins[layer*2]);
        Serial.print("], servo");
        Serial.print(layer*2+1);
        Serial.print("[GPIO");
        Serial.print(servoPins[layer*2+1]);
        Serial.println("])");
        
        setLayerAngle(layer, maxAngle);
        delay(500);
        setLayerAngle(layer, minAngle);
        delay(500);
    }
    
    Serial.println("Servo self-test completed!");
}

void ServoPlatformInter::setLayerAngleFromValue(uint8_t layer, int value) {
    if (layer >= layers) return;
    
    // 将0-1023映射到minAngle-maxAngle
    int angle = map(constrain(value, 0, 1023), 0, 1023, minAngle, maxAngle);
    setLayerAngle(layer, angle);
}

void ServoPlatformInter::setReverseAngle(bool reverse) {
    reverseAngle = reverse;
}

bool ServoPlatformInter::getReverseAngle() const {
    return reverseAngle;
}
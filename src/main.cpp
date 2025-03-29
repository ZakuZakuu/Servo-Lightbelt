#include <Arduino.h>
#include <LightBelt.h>
#include <ServoPlatformInter.h>
#include <ServoPlatform.h>
#include <BluetoothController.h>
#include <SerialController.h>
#include "GlobalConfig.h"

#define LED_PIN 5   // LED灯带数据引脚
#define SERVO_LAYER_COUNT 6 // 舵机层数
#define LED_LAYER_COUNT 12 // 每层舵机数量
#define LEDS_PER_LAYER 33 // 每层LED数量
#define CYCLE_TIME 5000  // 5秒周期

LightBelt belt(LED_PIN, LED_LAYER_COUNT, LEDS_PER_LAYER);

// 根据配置选择不同的舵机平台
#if USE_INTERNAL_PWM
ServoPlatformInter platform(LAYER_COUNT);
#else
ServoPlatform platform(SERVO_LAYER_COUNT);
#endif

// 根据配置选择不同的通信控制器
#if USE_BLUETOOTH
BluetoothController controller(&belt, &platform, CYCLE_TIME);
#else
SerialController controller(&belt, &platform, CYCLE_TIME);
#endif

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Starting initialization...");
    
    // 初始化LED灯带
    belt.begin();
    Serial.print("LED max brightness set to: ");
    Serial.println(MAX_LED_BRIGHTNESS);
    
    // 设置舵机角度反转状态
    platform.setReverseAngle(REVERSE_SERVO_ANGLE);
    Serial.print("Servo angle reverse mode: ");
    Serial.println(REVERSE_SERVO_ANGLE ? "ON" : "OFF");
    
    platform.begin();
    
    // 初始化控制器
    #if USE_BLUETOOTH
    controller.begin("ESP32-Lightbelt");
    Serial.println("Using Bluetooth control mode");
    #else
    controller.begin();
    Serial.println("Using Serial control mode");
    #endif
    
    Serial.println("Initialization completed!");
}

void loop() {
    controller.update();  // 处理命令并执行相应操作
    delay(10);  // 小延时防止过度刷新
}
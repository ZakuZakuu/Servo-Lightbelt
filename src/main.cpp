#include <Arduino.h>
#include <LightBelt.h>
#include <ServoPlatformInter.h>
#include <ServoPlatform.h>
#include <BluetoothController.h>
#include <SerialController.h>

#define LED_PIN 5
#define LAYER_COUNT 12
#define LEDS_PER_LAYER 33
#define CYCLE_TIME 5000  // 5秒周期

// 选择舵机驱动方式: true使用ESP32内置PWM，false使用外置PCA9685
#define USE_INTERNAL_PWM false

// 选择通信方式: true使用蓝牙通信，false使用串口通信
#define USE_BLUETOOTH false

LightBelt belt(LED_PIN, LAYER_COUNT, LEDS_PER_LAYER);

// 根据配置选择不同的舵机平台
#if USE_INTERNAL_PWM
ServoPlatformInter platform(LAYER_COUNT);
#else
ServoPlatform platform(LAYER_COUNT);
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
    belt.begin();
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
#include <Arduino.h>
#include <LightBelt.h>
#include <ServoPlatformInter.h>
#include <BluetoothController.h>

#define LED_PIN 5
#define LAYER_COUNT 3
#define LEDS_PER_LAYER 10
#define CYCLE_TIME 5000  // 5秒周期

LightBelt belt(LED_PIN, LAYER_COUNT, LEDS_PER_LAYER);
ServoPlatformInter platform(LAYER_COUNT);
BluetoothController btController(&belt, &platform, CYCLE_TIME);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("开始初始化...");
    belt.begin();
    platform.begin();
    btController.begin("ESP32-Lightbelt");
    Serial.println("初始化完成！");
}

void loop() {
    btController.update();  // 处理蓝牙命令并执行相应操作
    delay(10);  // 小延时防止过度刷新
}
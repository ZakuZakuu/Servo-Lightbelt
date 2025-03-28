#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

// 选择舵机驱动方式: true使用ESP32内置PWM，false使用外置PCA9685
#define USE_INTERNAL_PWM false

// 选择通信方式: true使用蓝牙通信，false使用串口通信
#define USE_BLUETOOTH false

// 舵机角度反转: true反转舵机运动方向，false保持正常方向
#define REVERSE_SERVO_ANGLE true

// LED灯带亮度限制: 0.0-1.0之间的值，限制灯带功率
#define MAX_LED_BRIGHTNESS 0.2f

#endif

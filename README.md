# 舵机控制光带系统 (Servo Lightbelt)

该项目是一个使用ESP32开发的多层舵机控制与LED灯带同步系统，支持通过串口或蓝牙控制多种动画效果和操作模式。

## 功能特点

- 多层舵机与LED灯带协同工作，创造动态视觉效果
- 支持多种预设动画模式：Rainbow、Idle、Heatup、Cooldown和Standby
- 实时控制模式（Follow），可精确控制每个舵机的角度
- 双通道控制：可选择使用串口或蓝牙通信
- 灵活的驱动选择：内置ESP32 PWM或外部PCA9685 PWM舵机驱动板
- 舵机角度反转功能，适应不同安装方向
- 灯光效果：彩虹循环、呼吸效果、渐变色等

## 硬件需求

- ESP32开发板
- WS2812B LED灯带（每层建议使用33个LED）
- 多个SG90/MG90S舵机（每层2个舵机）
- 可选：PCA9685 PWM舵机驱动板
- 电源供应（5V，电流根据LED和舵机数量而定）
- 面包板和连接线

## 软件依赖

- Arduino框架
- Adafruit NeoPixel库
- Adafruit PWM Servo Driver库
- Adafruit BusIO库
- BluetoothSerial库（ESP32内置）

## 代码结构

- **LightBelt**: LED灯带控制类
- **ServoPlatform**: 基于PCA9685的舵机平台控制类
- **ServoPlatformInter**: 基于ESP32内部PWM的舵机平台控制类
- **SerialController**: 串口控制器类
- **BluetoothController**: 蓝牙控制器类
- **GlobalConfig.h**: 全局配置文件

## 预设模式说明

1. **Rainbow**: 彩虹循环灯效，所有舵机平台带相位差往复运动
2. **Idle**: 白色呼吸灯效果，所有舵机回到最大角度
3. **Heatup**: 舵机相位差半个周期往返运动，灯带呼吸效果与对应舵机同步（红色）
4. **Cooldown**: 从最高层开始，每层依次由最大角度变为最小角度，灯光同步由亮变暗（橙黄色）
5. **Standby**: 所有舵机回到最小值，全部灯带显示蓝色呼吸灯效果
6. **Follow**: 实时控制模式，根据接收参数精确控制各层舵机角度

## 使用方法

### 环境配置

1. 使用PlatformIO或Arduino IDE打开项目
2. 根据您的硬件配置调整`GlobalConfig.h`中的选项：
   - `USE_INTERNAL_PWM`: 使用ESP32内部PWM(true)或PCA9685(false)
   - `USE_BLUETOOTH`: 使用蓝牙(true)或串口(false)通信
   - `REVERSE_SERVO_ANGLE`: 是否反转舵机角度

### 调整硬件参数

1. 在`main.cpp`中调整LED引脚、层数和每层LED数量
2. 在`ServoPlatformInter.cpp`中调整舵机引脚（如使用内部PWM）

### 上传代码

1. 连接ESP32到电脑
2. 使用PlatformIO或Arduino IDE编译并上传代码

### 控制命令

可通过串口或蓝牙发送以下格式的命令控制设备：

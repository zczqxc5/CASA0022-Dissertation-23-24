#include <Wire.h>
#include <LSM6DS3.h>
#include <ArduinoBLE.h>

#define MOTOR_PIN 2  // 使用新的电机控制引脚

LSM6DS3 myIMU(I2C_MODE, 0x6A);  // 默认I2C地址0x6A

void setup() {
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  Serial.begin(115200);

  Wire.begin();

  // 初始化传感器
  if (myIMU.begin() != 0) {
    Serial.println("IMU initialization failed");
  } else {
    Serial.println("IMU initialized successfully");
  }

  // 初始化蓝牙
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  BLE.setLocalName("XIAO");
  BLE.advertise();

  Serial.println("Bluetooth® Low Energy module initialized.");
}

void loop() {
  // 读取传感器数据
  float accX = myIMU.readFloatAccelX();
  float accY = myIMU.readFloatAccelY();
  float accZ = myIMU.readFloatAccelZ();

  Serial.print("AccX: ");
  Serial.print(accX);
  Serial.print(" AccY: ");
  Serial.print(accY);
  Serial.print(" AccZ: ");
  Serial.println(accZ);

  // 电机震动控制部分
  Serial.println("Motor ON");
  digitalWrite(MOTOR_PIN, HIGH);
  delay(1000);  // 电机开启1秒

  Serial.println("Motor OFF");
  digitalWrite(MOTOR_PIN, LOW);
  delay(1000);  // 电机关闭1秒
}

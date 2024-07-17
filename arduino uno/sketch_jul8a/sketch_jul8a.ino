#include <ArduinoBLE.h>

// 自定义服务和特征UUID
#define SERVICE_UUID "19B10000-E8F2-537E-4F6C-D104768A1214"
#define SWITCH_CHARACTERISTIC_UUID "19B10001-E8F2-537E-4F6C-D104768A1214"

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // 初始化蓝牙模块
  if (!BLE.begin()) {
    Serial.println("启动蓝牙低能耗模块失败！");
    while (1);
  }

  Serial.println("蓝牙低能耗中央设备 - 外围设备探索器");

  // 开始扫描外围设备
  BLE.scan();
}

void loop() {
  // 检查是否发现外围设备
  BLEDevice peripheral = BLE.available();

  if (peripheral) {
    // 打印发现的外围设备的地址、本地名称和广告服务UUID
    Serial.print("发现 ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();

    // 检查外围设备是否为XIAO
    if (peripheral.localName() == "XIAO") {
      // 停止扫描
      BLE.stopScan();

      // 连接到外围设备
      Serial.println("连接中 ...");

      if (peripheral.connect()) {
        Serial.println("已连接");

        // 增加一个延迟，确保设备有足够的时间来准备发现属性
        delay(10000); // 10秒延迟

        // 发现外围设备的属性
        Serial.println("发现属性中 ...");
        bool attributesDiscovered = false;
        unsigned long discoveryStartTime = millis();
        const unsigned long discoveryTimeout = 100000; // 60秒超时时间

        // 尝试在一定时间内发现属性
        while (!attributesDiscovered && (millis() - discoveryStartTime < discoveryTimeout)) {
          if (peripheral.discoverAttributes()) {
            attributesDiscovered = true;
            Serial.println("属性已发现");

            // 打印所有服务的UUID
            for (int i = 0; i < peripheral.serviceCount(); i++) {
              BLEService service = peripheral.service(i);
              Serial.print("服务UUID: ");
              Serial.println(service.uuid());

              // 打印每个服务的特征UUID
              for (int j = 0; j < service.characteristicCount(); j++) {
                BLECharacteristic characteristic = service.characteristic(j);
                Serial.print("  特征UUID: ");
                Serial.println(characteristic.uuid());
              }
            }

          } else {
            Serial.println("属性发现失败，再次尝试...");
            delay(1000); // 延迟1秒后再次尝试
          }
        }

        if (!attributesDiscovered) {
          Serial.println("无法发现所有属性，断开连接...");
          peripheral.disconnect();
          return;
        }

        // 尝试读取开关特性值，增加60秒时间重复尝试
        bool characteristicFound = false;
        unsigned long readStartTime = millis();
        const unsigned long readTimeout = 60000; // 60秒超时时间

        while (!characteristicFound && (millis() - readStartTime < readTimeout)) {
          BLECharacteristic switchCharacteristic = peripheral.characteristic(SWITCH_CHARACTERISTIC_UUID);
          if (switchCharacteristic) {
            Serial.println("发现开关特性");
            if (switchCharacteristic.canRead()) {
              Serial.println("读取开关特性值...");
              uint8_t value;
              if (switchCharacteristic.readValue(value)) {
                Serial.print("开关特性值: ");
                Serial.println(value);
                characteristicFound = true;
              } else {
                Serial.println("读取开关特性值失败！");
              }
            } else {
              Serial.println("开关特性不可读");
              characteristicFound = true; // 如果特性存在但不可读，就不再尝试
            }
          } else {
            Serial.println("未找到开关特性，尝试再次发现...");
            delay(1000); // 延迟1秒后再次尝试
          }
        }

        if (!characteristicFound) {
          Serial.println("无法找到或读取开关特性，断开连接...");
          peripheral.disconnect();
          return;
        }

        // 保持连接状态
        while (peripheral.connected()) {
          delay(1000); // 每秒延迟
          Serial.println("保持连接...");

          // 每次循环读取一次特性值
          BLECharacteristic switchCharacteristic = peripheral.characteristic(SWITCH_CHARACTERISTIC_UUID);
          if (switchCharacteristic && switchCharacteristic.canRead()) {
            uint8_t value;
            if (switchCharacteristic.readValue(value)) {
              Serial.print("实时开关特性值: ");
              Serial.println(value);
            } else {
              Serial.println("读取开关特性值失败！");
            }
          }
        }

        // 当连接断开时重新开始扫描
        Serial.println("连接已断开，重新扫描...");
        BLE.scan();
      } else {
        Serial.println("连接失败！");
        BLE.scan();  // 重新扫描
      }
    }
  }
}

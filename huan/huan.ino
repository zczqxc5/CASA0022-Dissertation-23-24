#include <ArduinoBLE.h>

// 蓝牙特性定义
BLEService ledService("19b10000-e8f2-537e-4f6c-d104768a1214");  
BLEIntCharacteristic labelCharacteristic("19b10002-e8f2-537e-4f6c-d104768a1214", BLERead | BLEWrite);

// 用于连接的全局变量
BLEDevice peripheral;
BLECharacteristic characteristic1;
BLECharacteristic characteristic2;
BLECharacteristic characteristic3;

#define LABEL_COUNT 7
const char *labels[LABEL_COUNT] = { "bye", "cuetain", "display", "hello", "light", "music", "other" };

// 获取标签ID的函数
int getLabelId(const char *label) {
  for (int i = 0; i < LABEL_COUNT; i++) {
    if (strcmp(labels[i], label) == 0) {
      return i;
    }
  }
  return -1;  
}

// 连接到指定外设的函数
bool connectToPeripheral(BLEDevice &peripheral) {
  delay(5000);
  Serial.print("Forming a connection to ");
  
  const int maxAttempts = 20;
  int attempts = 0;
  bool connected = false;

  while (attempts < maxAttempts && !connected) {
    attempts++;
    Serial.print("Attempt ");
    Serial.print(attempts);
    Serial.println(" to connect...");

    if (peripheral.connect()) {
      connected = true;
      Serial.println(" - Connected to peripheral");

      Serial.println("Discovering attributes ...");
      if (peripheral.discoverAttributes()) {
        Serial.println("Attributes discovered");

        // 尝试获取特定特性
        characteristic1 = peripheral.characteristic("19b10003-e8f2-537e-4f6c-d104768a1214");
        characteristic2 = peripheral.characteristic("19b10004-e8f2-537e-4f6c-d104768a1214");
        characteristic3 = peripheral.characteristic("19b10005-e8f2-537e-4f6c-d104768a1214");

        if (!characteristic1 || !characteristic2 || !characteristic3) {
          Serial.println("Failed to find one or more characteristics.");
          peripheral.disconnect();
          return false;
        }

        Serial.println(" - Found all characteristics");
        return true;
      } else {
        Serial.println("Attribute discovery failed!");
        peripheral.disconnect();
        return false;
      }
    } else {
      Serial.print("Failed to connect to the peripheral device, attempt ");
      Serial.print(attempts);
      Serial.println(" of ");
      Serial.println(maxAttempts);
      delay(1000);
    }
  }

  if (!connected) {
    Serial.println("Failed to connect after multiple attempts");
    return false;
  }

  return true;
}

void setup() {
  Serial.begin(9600);

  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  BLE.setAdvertisingInterval(32);
  BLE.setConnectionInterval(6, 12);

  Serial.println("Bluetooth® Low Energy module initialized.");

  BLE.setLocalName("XIAO");
  BLE.setAdvertisedService(ledService);

  ledService.addCharacteristic(labelCharacteristic);
  BLE.addService(ledService);

  labelCharacteristic.writeValue(0);
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");

  BLE.scan();  // 开始扫描其他外设
}

void loop() {
  BLEDevice central = BLE.central();

  // 外设模式：等待中央设备连接并发送数据
  if (central) {
    if (central.connected()) {
      unsigned long currentMillis = millis();
      static unsigned long previousMillis = 0;

      if (currentMillis - previousMillis >= 2500) {
        previousMillis = currentMillis;
        const char* max_label = "hello"; // 示例标签
        int labelId = getLabelId(max_label);
        if (labelId != -1) {
          labelCharacteristic.writeValue(labelId);
          Serial.print("Sent via Bluetooth: ");
          Serial.println(labelId);
        }
      }
    } else {
      Serial.print("Disconnected from central: ");
      Serial.println(central.address());
    }
  }

  // 中央设备模式：扫描并连接到其他外设
  peripheral = BLE.available();

  if (peripheral) {
    Serial.print("Found device with address: ");
    Serial.print(peripheral.address());
    Serial.print(" and name: ");
    Serial.println(peripheral.localName());

    if (peripheral.address() == "c4:de:e2:b9:b6:8e") {  // 替换为你的目标设备地址
      Serial.println("Found the target device. Attempting to connect...");

      BLE.stopScan();

      if (connectToPeripheral(peripheral)) {
        Serial.println("We are now connected to the BLE Server.");

        // 连接后每3秒读取一次特性值
        while (peripheral.connected()) {
          // 读取 characteristic1
          if (characteristic1.canRead()) {
            uint8_t valueBytes[4];
            characteristic1.readValue(valueBytes, 4);

            float value;
            memcpy(&value, valueBytes, sizeof(value));

            Serial.print("Characteristic 1 value: ");
            Serial.println(value, 2);
          } else {
            Serial.println("Characteristic 1 is not readable.");
          }

          // 读取 characteristic2
          if (characteristic2.canRead()) {
            uint8_t valueBytes[4];
            characteristic2.readValue(valueBytes, 4);

            float value;
            memcpy(&value, valueBytes, sizeof(value));

            Serial.print("Characteristic 2 value: ");
            Serial.println(value, 2);
          } else {
            Serial.println("Characteristic 2 is not readable.");
          }

          // 读取 characteristic3
          if (characteristic3.canRead()) {
            uint8_t valueBytes[4];
            characteristic3.readValue(valueBytes, 4);

            float value;
            memcpy(&value, valueBytes, sizeof(value));

            Serial.print("Characteristic 3 value: ");
            Serial.println(value, 2);
          } else {
            Serial.println("Characteristic 3 is not readable.");
          }

          delay(3000);  // 每3秒读取一次所有特性
        }
      } else {
        Serial.println("Connection failed.");
      }

      // 如果连接断开，重新开始扫描
      BLE.scan();
    }
  } else {
    Serial.println("No peripherals found, continuing scan...");
  }

  BLE.poll();
  delay(1000);  // 等待1秒再循环
}

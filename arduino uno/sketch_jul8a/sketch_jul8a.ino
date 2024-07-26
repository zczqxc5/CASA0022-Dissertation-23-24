#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <ESP32Servo.h>  // 添加包含 ESP32Servo 库
#include "DHT.h"

#define DHTPIN 21      // 定义DHT22连接的GPIO引脚
#define DHTTYPE DHT22  // 定义传感器类型DHT22
DHT dht(DHTPIN, DHTTYPE);
// 添加全局变量
Servo myServo;
int servoPosition = 0;  // 0 表示 0°，1 表示 180°
String lastValue = "";  // 上一次读取的特征值
int ledPin = 19;        // 假设LED连接到GPIO 2
bool ledState = false;  // false 表示关，true 表示开


// 定义服务和特征值UUID
static BLEUUID serviceUUID("19b10000-e8f2-537e-4f6c-d104768a1214");
static BLEUUID charUUID("19b10002-e8f2-537e-4f6c-d104768a1214");

// 全局变量
BLEClient* pClient;
BLEScan* pBLEScan;
bool doConnect = false;
bool connected = false;
bool doScan = false;
BLERemoteCharacteristic* pRemoteCharacteristic;
BLEAdvertisedDevice* myDevice;
const char* labels[] = { "bye", "curtain", "display", "hello", "light", "music", "other" };

unsigned long previousMillis = 0;  // 记录上一次读取的时间
const long interval = 5000;        // 读取间隔时间（5秒）

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected");
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remote BLE Server.
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Advertised Device Found: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      Serial.print("Found our device!  address: ");
      Serial.println(advertisedDevice.getAddress().toString().c_str());
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void setup() {
  myServo.attach(18);  // 假设舵机连接到引脚9
  Serial.begin(115200);
  BLEDevice::init("");
  pinMode(ledPin, OUTPUT);  // 设置LED引脚为输出模式
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readData();
  }
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
      connected = true;
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
    doConnect = false;
  }

  if (connected) {
    if (pRemoteCharacteristic->canRead()) {
      String value = pRemoteCharacteristic->readValue();

      Serial.print("Raw value read from characteristic: ");
      for (size_t i = 0; i < value.length(); i++) {
        Serial.print((int)value[i]);
        Serial.print(" ");
      }
      Serial.println();

      if (value.length() >= 1) {
        int intValue = (uint8_t)value[0];
        Serial.print("Parsed int value: ");
        Serial.println(intValue);  // 打印解析后的整数值

        if (intValue >= 0 && intValue < 7) {
          Serial.print("Switch characteristic value: ");
          Serial.println(labels[intValue]);
          handleCharacteristicValue(labels[intValue]);
        } else {
          Serial.println("Failed to read switch characteristic value!");
        }
      } else {
        Serial.println("Unexpected length of value read from characteristic!");
      }

      // 打印读取的特征UUID
      Serial.print("Characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
    }
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // this is just to keep scanning
  }

  delay(1000);
}

void handleCharacteristicValue(const String& value) {
  Serial.print("Handling value: ");
  Serial.println(value);

  // 检查当前值是否不同于上次读取的值
  if (value != lastValue) {
    lastValue = value;  // 更新上次读取的值
    if (value == "curtain") {
      toggleCurtain();
    } else if (value == "light") {
      toggleLed();
    }
  } else {
    Serial.println("Value is the same as the last one, no action taken.");
  }
}

void toggleCurtain() {
  // 每次切换舵机的位置（0° 或 180°）
  if (servoPosition == 0) {
    myServo.write(180);
    servoPosition = 1;
  } else {
    myServo.write(0);
    servoPosition = 0;
  }
  Serial.println("Toggling Curtain...");
}

void toggleLed() {
  // 切换LED的状态（开或关）
  ledState = !ledState;
  digitalWrite(ledPin, ledState ? HIGH : LOW);
  Serial.println("Toggling LED...");
}
void readData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // 检查读取是否成功
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("无法读取传感器数据");
    return;
  }

  // 打印读取的温度和湿度
  Serial.print("温度: ");
  Serial.print(temperature);
  Serial.print(" °C ");
  Serial.print("湿度: ");
  Serial.print(humidity);
  Serial.println(" %");
}
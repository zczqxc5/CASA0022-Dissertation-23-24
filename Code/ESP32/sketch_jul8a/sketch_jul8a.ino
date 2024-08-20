#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include <ESP32Servo.h>
#include "DHT.h"

#define DHTPIN 21
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
Servo myServo;
int servoPosition = 0;
String lastValue = "";
int ledPin = 19;
bool ledState = false;

// 按键和蜂鸣器的引脚定义
const int switchPin = 22;  // 按键开关连接引脚
const int buzzer = 15;     // 蜂鸣器连接引脚
bool isActive = false;     // 增加状态变量，初始为 false

static BLEUUID serviceUUID("19b10000-e8f2-537e-4f6c-d104768a1214");
static BLEUUID charUUID("19b10002-e8f2-537e-4f6c-d104768a1214");
static BLEUUID temperatureCharUUID("19b10003-e8f2-537e-4f6c-d104768a1214");
static BLEUUID humidityCharUUID("19b10004-e8f2-537e-4f6c-d104768a1214");
static BLEUUID buttonCharUUID("19b10005-e8f2-537e-4f6c-d104768a1214");  // 新增按钮特征值UUID
BLECharacteristic* pButtonCharacteristic = NULL;                        // 新增按钮特征值变量

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pTemperatureCharacteristic = NULL;
BLECharacteristic* pHumidityCharacteristic = NULL;

BLEClient* pClient;
BLEScan* pBLEScan;
bool doConnect = false;
bool connected = false;
bool doScan = false;
BLERemoteCharacteristic* pRemoteCharacteristic;
BLEAdvertisedDevice* myDevice;
const char* labels[] = { "bye", "curtain", "display", "hello", "light", "music", "other" };

unsigned long previousMillis = 0;
const long interval = 5000;


unsigned long noteDelay = 0;
int length;



bool isPlaying = false;
unsigned long noteStartTime = 0;
int currentNote = 0;
bool buttonPressed = false;

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {}

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

  pClient->connect(myDevice);
  Serial.println(" - Connected to server");

  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

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

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("Peripheral connected");
  }

  void onDisconnect(BLEServer* pServer) {
    Serial.println("Peripheral disconnected");
  }
};

void setup() {

  myServo.attach(18);
  Serial.begin(115200);
  BLEDevice::init("");
  dht.begin();
  pinMode(ledPin, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP);  // 设置按键引脚为输入模式并启用内部上拉电阻
  pinMode(buzzer, OUTPUT);           // 初始化蜂鸣器引脚为输出模式

  initBLEScan();
  initBLEServer();
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
        Serial.println(intValue);

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

      Serial.print("Characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
    }
  } else if (doScan) {
    BLEDevice::getScan()->start(0);
  }

  checkButton();

  delay(1000);
}

void handleCharacteristicValue(const String& value) {
  Serial.print("Handling value: ");
  Serial.println(value);

  if (value != lastValue) {

    lastValue = value;
    if (value == "hello") {
      isActive = true;  // 识别到 "hello"，激活处理
      Serial.println("System activated by 'hello'.");
    } else if (value == "bye") {
      isActive = false;  // 识别到 "bye"，停止处理
      Serial.println("System deactivated by 'bye'.");
    }
    if (isActive) {

      if (value == "curtain") {
        toggleCurtain();
      } else if (value == "light") {
        toggleLed();
      } else if (value == "music") {
        togmusic();
      } else {
        Serial.println("System is not active. Ignoring commands.");
      }

    } else {
      Serial.println("Value is the same as the last one, no action taken.");
    }
  }
}

void toggleCurtain() {
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
  ledState = !ledState;
  digitalWrite(ledPin, ledState ? HIGH : LOW);
  Serial.println("Toggling LED...");
}
void togmusic() {
  ledState = !ledState;
  buzz();
  buzz();
}


void readData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("无法读取传感器数据");
    return;
  }

  Serial.print("温度: ");
  Serial.print(temperature);
  Serial.print(" °C ");
  Serial.print("湿度: ");
  Serial.print(humidity);
  Serial.println(" %");

  pTemperatureCharacteristic->setValue((uint8_t*)&temperature, sizeof(float));
  pTemperatureCharacteristic->notify();

  pHumidityCharacteristic->setValue((uint8_t*)&humidity, sizeof(float));
  pHumidityCharacteristic->notify();
}

void initBLEScan() {
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void initBLEServer() {
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(serviceUUID);
  pCharacteristic = pService->createCharacteristic(
    charUUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setValue("Hello from ESP32 Peripheral");

  pTemperatureCharacteristic = pService->createCharacteristic(
    temperatureCharUUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  pHumidityCharacteristic = pService->createCharacteristic(
    humidityCharUUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  // 在initBLEServer()函数中添加
  pButtonCharacteristic = pService->createCharacteristic(
    buttonCharUUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  pService->start();
  pServer->getAdvertising()->start();
}

void checkButton() {
  int switchValue = digitalRead(switchPin);

  if (switchValue == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;
      buzz();
      Serial.println("Button pressed!");
      sendButtonState(1);  // 发送按钮按下状态
    }
  } else {
    if (buttonPressed) {
      buttonPressed = false;
      noBuzz();

      sendButtonState(0);  // 发送按钮松开状态
    }
  }
}

void sendButtonState(int state) {
  pButtonCharacteristic->setValue(state);
  pButtonCharacteristic->notify();
}


void buzz() {
  unsigned char i;
  Serial.println("buzz() called");

  for (i = 0; i < 80; i++) {
    digitalWrite(buzzer, HIGH);
    delay(1);
    digitalWrite(buzzer, LOW);
    delay(1);
  }
  for (i = 0; i < 100; i++) {
    digitalWrite(buzzer, HIGH);
    delay(2);
    digitalWrite(buzzer, LOW);
    delay(2);
  }
}

void noBuzz() {
  digitalWrite(buzzer, LOW);  // 关闭蜂鸣器
}

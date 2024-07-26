#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

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
const char* labels[] = {"bye", "curtain", "display", "hello", "light", "music", "other"};

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

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
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
  Serial.begin(115200);
  BLEDevice::init("");

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
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
        Serial.println(intValue); // 打印解析后的整数值

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

  // 处理特征值变化逻辑，例如控制LED或舵机
  if (value == "light") {
    // 处理light逻辑，例如切换LED
    toggleLed();
  } else if (value == "curtain") {
    // 处理curtain逻辑，例如控制舵机
    toggleCurtain();
  }
}

void toggleLed() {
  // 这里添加切换LED的逻辑
  Serial.println("Toggling LED...");
}

void toggleCurtain() {
  // 这里添加控制舵机的逻辑
  Serial.println("Toggling Curtain...");
}

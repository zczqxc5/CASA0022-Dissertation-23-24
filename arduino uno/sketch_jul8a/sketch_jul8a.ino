#include <ArduinoBLE.h>

BLEService sensorService("19B10000-E8F2-537E-4F6C-D104768A1214"); // 服务UUID
BLEByteCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEIntCharacteristic labelCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

const char* labels[] = { "bye", "curtain", "display", "hello", "light", "music", "other" };

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  Serial.println("Bluetooth® Low Energy module initialized.");

  BLE.setLocalName("Receiver");
  BLE.setAdvertisedService(sensorService);

  sensorService.addCharacteristic(switchCharacteristic);
  sensorService.addCharacteristic(labelCharacteristic);

  BLE.addService(sensorService);

  BLE.advertise();
  Serial.println("Bluetooth device active, waiting for connections...");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    while (central.connected()) {
      if (labelCharacteristic.written()) {
        int labelId = labelCharacteristic.value();
        if (labelId >= 0 && labelId < sizeof(labels) / sizeof(labels[0])) {
          Serial.print("Received label: ");
          Serial.println(labels[labelId]);
        } else {
          Serial.println("Received invalid label ID");
        }
      }
    }

    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}

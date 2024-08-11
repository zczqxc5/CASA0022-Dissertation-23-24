#include <ArduinoBLE.h>
#include <st7789v2.h>

BLEDevice peripheral;
st7789v2 Display;
#define MOTOR_PIN 2


// 全局变量，保存特性
BLECharacteristic characteristic1;
BLECharacteristic characteristic2;
BLECharacteristic characteristic3;
float lastValue = 0.0;  // 定义一个变量保存最后一次读取的值
bool firstRead = true;  // 添加一个标志变量来跟踪是否是第一次读取


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
    pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);  
  Serial.begin(9600);

  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1)
      ;
  }

  BLE.setConnectionInterval(6, 12);

  Serial.println("BLE Central - Scanning for peripherals...");
  BLE.scan();
  Display.SetRotate(180);
  Display.Init();
  Display.SetBacklight(100);
  Display.Clear(WHITE);
  display();
}


void loop() {
  BLEDevice peripheral = BLE.available();

  if (peripheral) {
    Serial.print("Found device with address: ");
    Serial.print(peripheral.address());
    Serial.print(" and name: ");
    Serial.println(peripheral.localName());

    if (peripheral.address() == "c4:de:e2:b9:b6:8e") {
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

          // 读取 characteristic3 并检测其值是否为 0100 0000 (64)
          if (characteristic3.canRead()) {
            uint8_t valueBytes[4];
            characteristic3.readValue(valueBytes, 4);

            // 假设我们只关心第一个字节
            uint8_t value = valueBytes[0];

            Serial.print("Characteristic 3 value (byte 0): ");
            Serial.println(value, BIN);

            // 检查是否第7位（第6位索引）为1
            if (value == 1) {  // 0x40 is 0100 0000 in hexadecimal

              if (!firstRead) {  // 如果不是第一次读取
                vibrateMotor(true);
                delay(200);
                vibrateMotor(false);
                delay(200);
                vibrateMotor(true);
                delay(200);
                vibrateMotor(false);
                delay(200);
                Display.DrawString_EN(30, 100, "Knocking", &Font20, WHITE, RED);
                delay(5000);
                updateDisplay("Hi        ");
              } else {
                firstRead = false;  // 标志第一次读取已经完成
              }
            }
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

  delay(1000);
}


void updateDisplay(const char *label) {
  int x = (240 - strlen(label) * 10) / 2;
  int y = (240 - 20) / 2;
  Display.DrawString_EN(30, 100, label, &Font20, WHITE, BLACK);
}
void updatevalue(const char *label) {
  Display.DrawString_EN(120, 100, label, &Font20, WHITE, BLACK);
}
void vibrateMotor(bool on) {
  if (on) {
    Serial.println("Motor ON command sent");
    digitalWrite(MOTOR_PIN, HIGH);
    delay(500);
  } else {
    Serial.println("Motor OFF command sent");
    digitalWrite(MOTOR_PIN, LOW);
    delay(500);
  }
}

void display() {
  updateDisplay("Hi!");
  Display.DrawLine(15, 65, 65, 65, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
  Display.DrawLine(15, 70, 80, 70, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

  Display.DrawRectangle(15, 80, 225, 150, GRAY, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

  Display.DrawCircle(10, 10, 25, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(10, 10, 20, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(10, 10, 15, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(10, 10, 10, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_FULL);

  Display.DrawCircle(230, 10, 25, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(230, 10, 20, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(230, 10, 15, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(230, 10, 10, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_FULL);

  Display.DrawCircle(10, 270, 25, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(10, 270, 20, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(10, 270, 15, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(10, 270, 10, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_FULL);

  Display.DrawCircle(230, 270, 25, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(230, 270, 20, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(230, 270, 15, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(230, 270, 10, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_FULL);

  Display.DrawLine(195, 160, 225, 160, GRAYBLUE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
  Display.DrawLine(175, 165, 225, 165, GRAYBLUE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
  Display.DrawString_EN(30, 125, "Silent Home", &Font20, WHITE, BLACK);
  Display.DrawString_EN(20, 180, "By: Xin Cheng", &Font20, WHITE, BLACK);
}

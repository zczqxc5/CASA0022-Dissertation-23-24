#include <fan_inferencing.h>
#include <Wire.h>
#include <LSM6DS3.h>
#include <st7789v2.h>
#include "SPI.h"
#include "seeed.h"
#include <ArduinoBLE.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 280

#define LABEL_COUNT 7

const char* labels[LABEL_COUNT] = { "bye", "curtain", "display", "hello", "light", "music", "other" };

int getLabelId(const char* label) {
  for (int i = 0; i < LABEL_COUNT; i++) {
    if (strcmp(labels[i], label) == 0) {
      return i;
    }
  }
  return -1; // 未找到标签
}

int getStringWidth(const char *str, const sFONT *font) {
  return strlen(str) * font->Width;
}

// 显示屏初始化
st7789v2 Display;

// 初始化传感器对象
LSM6DS3 myIMU(I2C_MODE, 0x6A);  // 默认I2C地址0x6A

BLEService ledService("19B10000-E8F2-537E-4F6C-D104768A1214");  // 自定义服务UUID
BLEByteCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);  // 自定义特征UUID
BLEIntCharacteristic labelCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite); // 定义labelCharacteristic

enum sensor_status {
  NOT_USED = -1,
  NOT_INIT,
  INIT,
  SAMPLED
};

/** Struct to link sensor axis name to sensor value function */
typedef struct {
  const char *name;
  float *value;
  uint8_t (*poll_sensor)(void);
  bool (*init_sensor)(void);
  sensor_status status;
} eiSensors;

/* Constant defines -------------------------------------------------------- */
#define CONVERT_G_TO_MS2 9.80665f
#define MAX_ACCEPTED_RANGE 2.0f
#define N_SENSORS 6
#define EI_CLASSIFIER_FUSION_AXES_STRING "accX + accY + accZ + gyrX + gyrY + gyrZ"

float data[N_SENSORS];
int8_t fusion_sensors[N_SENSORS];
int fusion_ix = 0;

bool init_IMU(void) {
  static bool init_status = false;
  if (!init_status) {
    init_status = (myIMU.begin() == 0);
    if (init_status) {
      Serial.println("IMU initialized successfully");
    } else {
      Serial.println("IMU initialization failed");
    }
  }
  return init_status;
}

uint8_t poll_acc(void) {
  data[0] = myIMU.readFloatAccelX();
  data[1] = myIMU.readFloatAccelY();
  data[2] = myIMU.readFloatAccelZ();

  for (int i = 0; i < 3; i++) {
    if (fabs(data[i]) > MAX_ACCEPTED_RANGE) {
      data[i] = (data[i] > 0 ? 1 : -1) * MAX_ACCEPTED_RANGE;
    }
  }

  data[0] *= CONVERT_G_TO_MS2;
  data[1] *= CONVERT_G_TO_MS2;
  data[2] *= CONVERT_G_TO_MS2;

  return 0;
}

uint8_t poll_gyr(void) {
  data[3] = myIMU.readFloatGyroX();
  data[4] = myIMU.readFloatGyroY();
  data[5] = myIMU.readFloatGyroZ();

  return 0;
}

/** Used sensors value function connected to label name */
eiSensors sensors[] = {
  { "accX", &data[0], &poll_acc, &init_IMU, NOT_USED },
  { "accY", &data[1], &poll_acc, &init_IMU, NOT_USED },
  { "accZ", &data[2], &poll_acc, &init_IMU, NOT_USED },
  { "gyrX", &data[3], &poll_gyr, &init_IMU, NOT_USED },
  { "gyrY", &data[4], &poll_gyr, &init_IMU, NOT_USED },
  { "gyrZ", &data[5], &poll_gyr, &init_IMU, NOT_USED },
};

static int8_t ei_find_axis(char *axis_name) {
  for (int ix = 0; ix < N_SENSORS; ix++) {
    if (strstr(axis_name, sensors[ix].name)) {
      return ix;
    }
  }
  return -1;
}

static bool ei_connect_fusion_list(const char *input_list) {
  char *buff;
  bool is_fusion = false;

  /* Copy const string in heap mem */
  char *input_string = (char *)ei_malloc(strlen(input_list) + 1);
  if (input_string == NULL) {
    Serial.println("Error allocating memory for input_string");
    return false;
  }
  memset(input_string, 0, strlen(input_list) + 1);
  strncpy(input_string, input_list, strlen(input_list));

  /* Clear fusion sensor list */
  memset(fusion_sensors, 0, N_SENSORS);
  fusion_ix = 0;

  buff = strtok(input_string, "+");
  while (buff != NULL) { /* Run through buffer */
    int8_t found_axis = ei_find_axis(buff);
    Serial.print("Checking axis: ");
    Serial.println(buff);

    if (found_axis >= 0) {
      if (fusion_ix < N_SENSORS) {
        fusion_sensors[fusion_ix++] = found_axis;
        sensors[found_axis].status = NOT_INIT;
      }
      is_fusion = true;
      Serial.print("Found axis: ");
      Serial.println(sensors[found_axis].name);
    } else {
      Serial.print("Axis not found: ");
      Serial.println(buff);
    }

    buff = strtok(NULL, "+ ");
  }

  ei_free(input_string);

  return is_fusion;
}

void setup() {
  Serial.begin(9600);


  // 初始化蓝牙
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  Serial.println("Bluetooth® Low Energy module initialized.");

  BLE.setLocalName("XIAO");
  BLE.setAdvertisedService(ledService);

  // 添加特征到服务
  ledService.addCharacteristic(switchCharacteristic);
  ledService.addCharacteristic(labelCharacteristic); // 添加labelCharacteristic

  // 添加服务
  BLE.addService(ledService);

  // 设置特征的初始值
  switchCharacteristic.writeValue(0);

  // 开始广播
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");

  // 初始化显示屏
  Display.SetRotate(180);
  Display.Init();
  Display.SetBacklight(100);
  Display.Clear(WHITE);

  // 初始化I2C
  Wire.begin();

  Serial.println("Starting initialization...");

  Serial.print("Required sensors: ");
  Serial.println(EI_CLASSIFIER_FUSION_AXES_STRING);

  if (!ei_connect_fusion_list(EI_CLASSIFIER_FUSION_AXES_STRING)) {
    Serial.println("ERR: Errors in sensor list detected");
    return;
  }

  for (int i = 0; i < fusion_ix; i++) {
    if (sensors[fusion_sensors[i]].status == NOT_INIT) {
      sensors[fusion_sensors[i]].status = (sensor_status)sensors[fusion_sensors[i]].init_sensor();
      if (!sensors[fusion_sensors[i]].status) {
        Serial.print(sensors[fusion_sensors[i]].name);
        Serial.println(" axis sensor initialization failed.");
      } else {
        Serial.print(sensors[fusion_sensors[i]].name);
        Serial.println(" axis sensor initialization successful.");
      }
    }
  }
}

void loop() {
  // 监听连接的BLE外围设备
  BLEDevice central = BLE.central();

  // 如果有中央设备连接到外围设备
  if (central) {
    Serial.print("Connected to central: ");
    // 打印中央设备的MAC地址
    Serial.println(central.address());

    // 在显示屏上显示连接信息
    Display.Clear(WHITE);
    Display.DrawString_EN(10, 10, "Connected", &Font20, WHITE, BLACK);

    // 连接后开始推理过程
    Serial.println("\nStarting inferencing in 2 seconds...");
    delay(2000);

    while (central.connected()) {
      // 采集和推理数据
      if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != fusion_ix) {
        Serial.print("ERR: Sensors don't match the sensors required in the model\n");
        Serial.print("Following sensors are required: ");
        Serial.println(EI_CLASSIFIER_FUSION_AXES_STRING);
        return;
      }

      Serial.println("Sampling...");

      float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };

      for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME) {
        int64_t next_tick = (int64_t)micros() + ((int64_t)EI_CLASSIFIER_INTERVAL_MS * 1000);

        for (int i = 0; i < fusion_ix; i++) {
          if (sensors[fusion_sensors[i]].status == INIT) {
            sensors[fusion_sensors[i]].poll_sensor();
            sensors[fusion_sensors[i]].status = SAMPLED;
          }
          if (sensors[fusion_sensors[i]].status == SAMPLED) {
            buffer[ix + i] = *sensors[fusion_sensors[i]].value;
            sensors[fusion_sensors[i]].status = INIT;
          }
        }

        int64_t wait_time = next_tick - (int64_t)micros();
        if (wait_time > 0) {
          delayMicroseconds(wait_time);
        }

        // 定期发送心跳包以保持连接
        switchCharacteristic.writeValue(1);
      }

      signal_t signal;
      int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
      if (err != 0) {
        Serial.print("ERR: ");
        Serial.println(err);
        return;
      }

      ei_impulse_result_t result = { 0 };
      err = run_classifier(&signal, &result, false);
      if (err != EI_IMPULSE_OK) {
        Serial.print("ERR: ");
        Serial.println(err);
        return;
      }

      Serial.print("Predictions (DSP: ");
      Serial.print(result.timing.dsp);
      Serial.print(" ms., Classification: ");
      Serial.print(result.timing.classification);
      Serial.print(" ms., Anomaly: ");
      Serial.print(result.timing.anomaly);
      Serial.println(" ms.):");
      display_max_probability_label(result);

      for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        Serial.print(result.classification[ix].label);
        Serial.print(": ");
        Serial.println(result.classification[ix].value);

        char value_str[10];
        dtostrf(result.classification[ix].value, 6, 2, value_str);
      }

#if EI_CLASSIFIER_HAS_ANOMALY == 1
      Serial.print("    anomaly score: ");
      Serial.println(result.anomaly);

      char anomaly_str[10];
      dtostrf(result.anomaly, 6, 2, anomaly_str);
#endif

      delay(1000);  // 主循环延时

      // 在每次推理完成后检查连接状态
      if (!central.connected()) {
        Serial.println("Central device disconnected during inferencing.");
        Display.Clear(WHITE);
        Display.DrawString_EN(10, 10, "Disconnected", &Font20, WHITE, BLACK);
        return;
      }
    }

    // 当中央设备断开连接时，打印出来
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());

    // 在显示屏上显示断开连接信息
    Display.Clear(WHITE);
    Display.DrawString_EN(10, 10, "Disconnected", &Font20, WHITE, BLACK);
  }
}

void display_max_probability_label(ei_impulse_result_t result) {
  float max_value = 0.0;
  const char *max_label = nullptr;

  // 找出最大可能性的标签
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    if (result.classification[ix].value > max_value) {
      max_value = result.classification[ix].value;
      max_label = result.classification[ix].label;
    }
  }

  // 打印和显示最大可能性的标签
  if (max_label != nullptr) {
    Serial.print("Max label: ");
    Serial.println(max_label);

    Display.Clear(WHITE);  // 清除屏幕，每次显示新结果前清空屏幕
    int text_width = getStringWidth(max_label, &Font20);
    int x_center = (SCREEN_WIDTH - text_width) / 2;                               // 计算居中位置的X坐标
    int y_center = (SCREEN_HEIGHT - Font20.Height) / 2;                           // 计算居中位置的Y坐标
    Display.DrawString_EN(x_center, y_center, max_label, &Font20, WHITE, BLACK);  // 显示最大可能性的标签

    // 将标签转换为数字ID并通过蓝牙发送
    int labelId = getLabelId(max_label);
    if (labelId != -1) {
      labelCharacteristic.writeValue(labelId);
      Serial.print("Sent via Bluetooth: ");
      Serial.println(labelId);
    }
  }
}

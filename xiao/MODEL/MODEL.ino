#include <fonts.h>
#include <st7789v2.h>

#include <new_inferencing.h>
#include <Wire.h>
#include <LSM6DS3.h>

#include "SPI.h"
#include "seeed.h"
#include <ArduinoBLE.h>

#define LABEL_COUNT 7
#define MOTOR_PIN 2
const char *labels[LABEL_COUNT] = { "bye", "cuetain", "display", "hello", "light", "music", "other" };

int getLabelId(const char *label) {
  for (int i = 0; i < LABEL_COUNT; i++) {
    if (strcmp(labels[i], label) == 0) {
      return i;
    }
  }
  return -1;  
}


LSM6DS3 myIMU(I2C_MODE, 0x6A);  

BLEService ledService("19b10000-e8f2-537e-4f6c-d104768a1214");  

BLEIntCharacteristic labelCharacteristic("19b10002-e8f2-537e-4f6c-d104768a1214", BLERead | BLEWrite);  

enum sensor_status {
  NOT_USED = -1,
  NOT_INIT,
  INIT,
  SAMPLED
};

typedef struct {
  const char *name;
  float *value;
  uint8_t (*poll_sensor)(void);
  bool (*init_sensor)(void);
  sensor_status status;
} eiSensors;

#define CONVERT_G_TO_MS2 9.80665f
#define DEG_TO_RAD 0.0174533f
#define N_SENSORS 6
#define EI_CLASSIFIER_FUSION_AXES_STRING "accX + accY + accZ + gyrX + gyrY + gyrZ"

float data[N_SENSORS];
int8_t fusion_sensors[N_SENSORS];
int fusion_ix = 0;

float max_value = 0.0;
const char *max_label = "bye";
const char *previous_label = "";
unsigned long previousMillis = 0;
unsigned long previousSampleMillis = 0;
unsigned long previousDisplayMillis = 0;
const unsigned long sampleInterval = 1000;
const unsigned long displayInterval = 1000;

st7789v2 Display;

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
  data[0] = myIMU.readFloatAccelX() * CONVERT_G_TO_MS2;
  data[1] = myIMU.readFloatAccelY() * CONVERT_G_TO_MS2;
  data[2] = myIMU.readFloatAccelZ() * CONVERT_G_TO_MS2;
  return 0;
}

uint8_t poll_gyr(void) {
  data[3] = myIMU.readFloatGyroX() * DEG_TO_RAD;
  data[4] = myIMU.readFloatGyroY() * DEG_TO_RAD;
  data[5] = myIMU.readFloatGyroZ() * DEG_TO_RAD;
  return 0;
}

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

  char *input_string = (char *)ei_malloc(strlen(input_list) + 1);
  if (input_string == NULL) {
    Serial.println("Error allocating memory for input_string");
    return false;
  }
  memset(input_string, 0, strlen(input_list) + 1);
  strncpy(input_string, input_list, strlen(input_list));

  memset(fusion_sensors, 0, N_SENSORS);
  fusion_ix = 0;

  buff = strtok(input_string, "+");
  while (buff != NULL) {
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
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);  // 确保电机初始关闭

  Serial.begin(9600);

  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
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

  Display.SetRotate(180);
  Display.Init();
  Display.SetBacklight(100);
  Display.Clear(WHITE);
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    if (central.connected()) {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= 2500) {
        previousMillis = currentMillis;
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

  unsigned long currentSampleMillis = millis();
  if (currentSampleMillis - previousSampleMillis >= sampleInterval) {
    previousSampleMillis = currentSampleMillis;

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

      BLE.poll();
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
    update_max_probability_label(result);

    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
      Serial.print(result.classification[ix].label);
      Serial.print(": ");
      Serial.println(result.classification[ix].value);
    }

#if EI_CLASSIFIER_HAS_ANOMALY == 1
    Serial.print("    anomaly score: ");
    Serial.println(result.anomaly);
#endif
  }

  unsigned long currentDisplayMillis = millis();
  if (currentDisplayMillis - previousDisplayMillis >= displayInterval && strcmp(max_label, previous_label) != 0) {
    previousDisplayMillis = currentDisplayMillis;
    previous_label = max_label;
  }
}

void update_max_probability_label(ei_impulse_result_t result) {
  max_value = 0.0;
  max_label = "bye";

  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    if (result.classification[ix].value > max_value) {
      max_value = result.classification[ix].value;
      max_label = result.classification[ix].label;
    }
  }

  if (max_label != nullptr) {
    Serial.print("Max label: ");
    Serial.println(max_label);
    if (previous_label == nullptr || strcmp(previous_label, max_label) != 0) {


      if (strcmp(max_label, "hello") == 0) {
        vibrateMotor(true);
        delay(200);
        vibrateMotor(false);
        delay(200);
      } else if (strcmp(max_label, "bye") == 0) {
        vibrateMotor(true);
        delay(200);
        vibrateMotor(false);
        delay(200);
        vibrateMotor(true);
        delay(200);
        vibrateMotor(false);
        delay(200);
      }

      previous_label = max_label;
    }
  } else {
    previous_label = nullptr;  
}

void updateDisplay(const char *label) {
  Display.Clear(WHITE);

  int x = (240 - strlen(label) * 10) / 2;
  int y = (240 - 20) / 2;

  Display.DrawString_EN(x, y, label, &Font20, WHITE, BLACK);
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

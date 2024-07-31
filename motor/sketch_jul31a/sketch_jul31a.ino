// 定义电机控制引脚
#define MOTOR_PIN 5

void setup() {
  // 初始化电机控制引脚
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW); // 确保电机初始关闭

  // 启用串口调试
  Serial.begin(9600);
}

void loop() {
  // 震动电机
  Serial.println("Starting vibration...");
  vibrateMotor(true);
  delay(200);


  vibrateMotor(false);
  delay(200);

  Serial.println("Starting vibration again...");
  vibrateMotor(true);
  delay(200);
}

void vibrateMotor(bool on) {
  if (on) {
    Serial.println("Motor ON command sent");
    digitalWrite(MOTOR_PIN, HIGH);  // 开启电机
  } else {
    Serial.println("Motor OFF command sent");
    digitalWrite(MOTOR_PIN, LOW);  // 关闭电机
  }
}

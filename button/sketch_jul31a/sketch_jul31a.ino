const int switchPin = 22;  // 按键开关连接引脚9

void setup() {
  pinMode(switchPin, INPUT_PULLUP);  // 设置引脚9为输入模式并启用内部上拉电阻
  Serial.begin(9600); // 设置串口波特率为9600
}

void loop() {
  int switchValue = digitalRead(switchPin);  //读取引脚9的值
  Serial.println(switchValue); //将读取的按键值输出到串口监视器
  delay(500); // 增加延迟，以便更容易读取串口监视器中的输出
}

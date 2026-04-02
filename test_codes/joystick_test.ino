// ============================================================
//  JOYSTICK TEST — ESP32 · Serial Monitor
//  Open Serial Monitor at 115200 baud
//
//  Wiring:
//   VCC → 3.3V
//   GND → GND
//   VRX → GPIO 32
//   VRY → GPIO 34
//   SW  → GPIO 35
// ============================================================

#define PIN_JOY_X  32
#define PIN_JOY_Y  34
#define PIN_JOY_SW 27

// Thresholds (same ones used in the games)
#define JOY_LOW   1000
#define JOY_HIGH  3000

void setup() {
  Serial.begin(115200);
  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  analogReadResolution(12);  // ESP32: 12-bit ADC → 0 to 4095

  Serial.println("==============================");
  Serial.println("   JOYSTICK TEST — ESP32");
  Serial.println("==============================");
  Serial.println("Center should read ~2048");
  Serial.println("Push UP    → Y  < 1000");
  Serial.println("Push DOWN  → Y  > 3000");
  Serial.println("Push LEFT  → X  < 1000");
  Serial.println("Push RIGHT → X  > 3000");
  Serial.println("Click SW   → Button = PRESSED");
  Serial.println("------------------------------");
  delay(1000);
}

void loop() {
  int rawX  = analogRead(PIN_JOY_X);
  int rawY  = analogRead(PIN_JOY_Y);
  bool btn  = digitalRead(PIN_JOY_SW) == LOW;

  // Interpret direction
  String dirX = "CENTER";
  if      (rawX < JOY_LOW)  dirX = "LEFT  <--";
  else if (rawX > JOY_HIGH) dirX = "--> RIGHT";

  String dirY = "CENTER";
  if      (rawY < JOY_LOW)  dirY = "UP  ^^^";
  else if (rawY > JOY_HIGH) dirY = "DOWN vvv";

  String btnStr = btn ? "*** PRESSED ***" : "not pressed";

  // Print everything on one clean line
  Serial.print("X: ");
  Serial.print(rawX);
  Serial.print(" (");
  Serial.print(dirX);
  Serial.print(")  |  Y: ");
  Serial.print(rawY);
  Serial.print(" (");
  Serial.print(dirY);
  Serial.print(")  |  BTN: ");
  Serial.println(btnStr);

  delay(100);  // 10 readings per second — easy to read in monitor
}

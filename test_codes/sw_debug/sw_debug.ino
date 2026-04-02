// ============================================================
//  JOYSTICK SW DEBUG — ESP32 · Serial Monitor
//  Tries multiple GPIO pins and both PULLUP / PULLDOWN modes
//  so you can find which one actually works for your setup.
//
//  Open Serial Monitor at 115200 baud
//  Press the joystick click button and watch which line reacts
// ============================================================

// ── Try changing this pin number first ──────────────────────
//  Common options: 35, 27, 26, 25, 18, 19, 23
#define PIN_SW  34   // <-- change this if nothing works

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("==================================");
  Serial.println("  JOYSTICK SW DEBUG — ESP32");
  Serial.println("==================================");

  // Test 1: INPUT_PULLUP  (most common for joystick SW)
  //   → button connects pin to GND when pressed
  //   → reads LOW when pressed, HIGH when not
  pinMode(PIN_SW, INPUT_PULLUP);
  Serial.print("Mode: INPUT_PULLUP on GPIO ");
  Serial.println(PIN_SW);
  Serial.println("Press the joystick button now...");
  Serial.println("----------------------------------");
}

int  lastRaw      = -1;
bool lastPressed  = false;
int  pressCount   = 0;
unsigned long lastPrint = 0;

void loop() {
  int  raw     = digitalRead(PIN_SW);
  bool pressed = (raw == LOW);   // PULLUP: LOW = pressed

  // Print every 200ms so it's easy to read
  if (millis() - lastPrint >= 200) {
    lastPrint = millis();

    Serial.print("GPIO ");
    Serial.print(PIN_SW);
    Serial.print("  raw=");
    Serial.print(raw);
    Serial.print("  → ");

    if (pressed) {
      Serial.println("*** PRESSED *** <<<");
    } else {
      Serial.println("not pressed");
    }
  }

  // Also print immediately on state change for instant feedback
  if (pressed != lastPressed) {
    lastPressed = pressed;
    if (pressed) {
      pressCount++;
      Serial.print("↓ BUTTON DOWN  (press #");
      Serial.print(pressCount);
      Serial.println(")");
    } else {
      Serial.println("↑ button released");
    }
  }

  delay(10);
}

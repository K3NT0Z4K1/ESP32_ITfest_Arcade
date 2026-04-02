// ============================================================
//  IT FEST DINO GAME — Hardware Test Version
//  LCD : 4×16 I2C
//  MCU : ESP32
//  Input: Joystick (Y-axis only)
//  No WiFi · No Firebase
// ============================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ── I2C pins (ESP32 default) ──────────────────────────────────
#define SDA_PIN  21
#define SCL_PIN  22

// ── LCD: 16 columns, 4 rows, I2C address 0x27 ────────────────
//    If screen stays blank, try 0x3F instead of 0x27
LiquidCrystal_I2C lcd(0x27, 16, 4);

// ── Joystick ──────────────────────────────────────────────────
#define PIN_JOY_Y   34   // Analog Y-axis
#define PIN_JOY_SW  35   // Click button (optional restart)

#define JOY_UP_THRESH    1000   // Y < this  →  jump
#define JOY_DOWN_THRESH  3000   // Y > this  →  duck

// ── Sprite slot numbers ───────────────────────────────────────
#define SPRITE_RUN1              1
#define SPRITE_RUN2              2
#define SPRITE_JUMP              3
#define SPRITE_JUMP_UPPER        '.'
#define SPRITE_JUMP_LOWER        4
#define SPRITE_TERRAIN_EMPTY     ' '
#define SPRITE_TERRAIN_SOLID     5
#define SPRITE_TERRAIN_SOLID_RIGHT 6
#define SPRITE_TERRAIN_SOLID_LEFT  7

// ── Hero position states ──────────────────────────────────────
#define HERO_HORIZONTAL_POSITION 1

#define HERO_POSITION_OFF          0
#define HERO_POSITION_RUN_LOWER_1  1
#define HERO_POSITION_RUN_LOWER_2  2
#define HERO_POSITION_JUMP_1       3
#define HERO_POSITION_JUMP_2       4
#define HERO_POSITION_JUMP_3       5
#define HERO_POSITION_JUMP_4       6
#define HERO_POSITION_JUMP_5       7
#define HERO_POSITION_JUMP_6       8
#define HERO_POSITION_JUMP_7       9
#define HERO_POSITION_JUMP_8       10
#define HERO_POSITION_RUN_UPPER_1  11
#define HERO_POSITION_RUN_UPPER_2  12
#define HERO_POSITION_DUCK_1       13
#define HERO_POSITION_DUCK_2       14

// ── Terrain ───────────────────────────────────────────────────
#define TERRAIN_WIDTH        16
#define TERRAIN_EMPTY         0
#define TERRAIN_LOWER_BLOCK   1
#define TERRAIN_UPPER_BLOCK   2

// ── Terrain buffers ───────────────────────────────────────────
static char terrainUpper[TERRAIN_WIDTH + 1];
static char terrainLower[TERRAIN_WIDTH + 1];

// ── Input state ───────────────────────────────────────────────
static bool jumpPressed = false;
static bool duckPressed = false;
static bool btnPressed  = false;

// ── Score tracking ────────────────────────────────────────────
static unsigned int hiScore = 0;

// ─────────────────────────────────────────────────────────────
//  Custom character bitmaps (7 sprites)
// ─────────────────────────────────────────────────────────────
void initializeGraphics() {
  static byte graphics[] = {
    // 1 – Dino run pose 1
    B11110, B10111, B11110, B11100,
    B11111, B11100, B10010, B11011,
    // 2 – Dino run pose 2
    B11110, B10111, B11110, B11100,
    B11111, B11100, B01010, B10011,
    // 3 – Dino jump (full body)
    B11110, B10111, B11110, B11100,
    B11111, B11110, B10001, B00000,
    // 4 – Jump lower split
    B11111, B11100, B10010, B11011,
    B00000, B00000, B00000, B00000,
    // 5 – Cactus center
    B00100, B11111, B00100, B00101,
    B11111, B00100, B00100, B00100,
    // 6 – Cactus right arm
    B00000, B00011, B00000, B00000,
    B00011, B00000, B00000, B00000,
    // 7 – Cactus left arm
    B00000, B11000, B00000, B01000,
    B11000, B00000, B00000, B00000,
  };

  for (int i = 0; i < 7; ++i)
    lcd.createChar(i + 1, &graphics[i * 8]);

  // Clear terrain buffers
  for (int i = 0; i < TERRAIN_WIDTH; ++i) {
    terrainUpper[i] = SPRITE_TERRAIN_EMPTY;
    terrainLower[i] = SPRITE_TERRAIN_EMPTY;
  }
}

// ─────────────────────────────────────────────────────────────
//  Scroll terrain one step left, feed new tile on the right
// ─────────────────────────────────────────────────────────────
void advanceTerrain(char* terrain, byte newTerrain) {
  for (int i = 0; i < TERRAIN_WIDTH; ++i) {
    char current = terrain[i];
    char next    = (i == TERRAIN_WIDTH - 1) ? newTerrain : terrain[i + 1];
    switch (current) {
      case SPRITE_TERRAIN_EMPTY:
        terrain[i] = (next == SPRITE_TERRAIN_SOLID)
                     ? SPRITE_TERRAIN_SOLID_RIGHT : SPRITE_TERRAIN_EMPTY;
        break;
      case SPRITE_TERRAIN_SOLID:
        terrain[i] = (next == SPRITE_TERRAIN_EMPTY)
                     ? SPRITE_TERRAIN_SOLID_LEFT : SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_RIGHT:
        terrain[i] = SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_LEFT:
        terrain[i] = SPRITE_TERRAIN_EMPTY;
        break;
    }
  }
}

// ─────────────────────────────────────────────────────────────
//  Draw hero on rows 0 & 1, score top-right
//  Returns true if hero collides with terrain
// ─────────────────────────────────────────────────────────────
bool drawHero(byte position, char* tUpper, char* tLower, unsigned int score) {
  bool  collide   = false;
  char  upperSave = tUpper[HERO_HORIZONTAL_POSITION];
  char  lowerSave = tLower[HERO_HORIZONTAL_POSITION];
  byte  upper, lower;

  switch (position) {
    case HERO_POSITION_OFF:
      upper = lower = SPRITE_TERRAIN_EMPTY; break;

    case HERO_POSITION_RUN_LOWER_1:
      upper = SPRITE_TERRAIN_EMPTY; lower = SPRITE_RUN1; break;

    case HERO_POSITION_RUN_LOWER_2:
      upper = SPRITE_TERRAIN_EMPTY; lower = SPRITE_RUN2; break;

    case HERO_POSITION_JUMP_1:
    case HERO_POSITION_JUMP_8:
      upper = SPRITE_TERRAIN_EMPTY; lower = SPRITE_JUMP; break;

    case HERO_POSITION_JUMP_2:
    case HERO_POSITION_JUMP_7:
      upper = SPRITE_JUMP_UPPER; lower = SPRITE_JUMP_LOWER; break;

    case HERO_POSITION_JUMP_3:
    case HERO_POSITION_JUMP_4:
    case HERO_POSITION_JUMP_5:
    case HERO_POSITION_JUMP_6:
      upper = SPRITE_JUMP; lower = SPRITE_TERRAIN_EMPTY; break;

    case HERO_POSITION_RUN_UPPER_1:
      upper = SPRITE_RUN1; lower = SPRITE_TERRAIN_EMPTY; break;

    case HERO_POSITION_RUN_UPPER_2:
      upper = SPRITE_RUN2; lower = SPRITE_TERRAIN_EMPTY; break;

    // Duck: stay on lower row, small hitbox (no upper body)
    case HERO_POSITION_DUCK_1:
    case HERO_POSITION_DUCK_2:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = (position == HERO_POSITION_DUCK_1) ? SPRITE_RUN1 : SPRITE_RUN2;
      break;

    default:
      upper = lower = SPRITE_TERRAIN_EMPTY; break;
  }

  // Place hero sprite into terrain buffers
  if (upper != ' ') {
    tUpper[HERO_HORIZONTAL_POSITION] = upper;
    collide = (upperSave != SPRITE_TERRAIN_EMPTY);
  }
  if (lower != ' ') {
    tLower[HERO_HORIZONTAL_POSITION] = lower;
    collide |= (lowerSave != SPRITE_TERRAIN_EMPTY);
  }

  // Score width
  byte digits = (score > 9999) ? 5
              : (score > 999)  ? 4
              : (score > 99)   ? 3
              : (score > 9)    ? 2 : 1;

  tUpper[TERRAIN_WIDTH] = '\0';
  tLower[TERRAIN_WIDTH] = '\0';

  // Print row 0 (terrain + score)
  char temp = tUpper[16 - digits];
  tUpper[16 - digits] = '\0';
  lcd.setCursor(0, 0);
  lcd.print(tUpper);
  tUpper[16 - digits] = temp;

  // Print row 1 (terrain)
  lcd.setCursor(0, 1);
  lcd.print(tLower);

  // Print score top-right
  lcd.setCursor(16 - digits, 0);
  lcd.print(score);

  // Restore buffers
  tUpper[HERO_HORIZONTAL_POSITION] = upperSave;
  tLower[HERO_HORIZONTAL_POSITION] = lowerSave;

  return collide;
}

// ─────────────────────────────────────────────────────────────
//  Rows 2 & 3 — static info bar
// ─────────────────────────────────────────────────────────────
void drawInfoBar(unsigned int score) {
  lcd.setCursor(0, 2);
  lcd.print("Score:");
  lcd.print(score);
  lcd.print("          ");   // clear trailing chars

  lcd.setCursor(0, 3);
  lcd.print("HI:");
  lcd.print(hiScore);
  lcd.print("          ");
}

// ─────────────────────────────────────────────────────────────
//  Read joystick into global flags
// ─────────────────────────────────────────────────────────────
void readJoystick() {
  int y       = analogRead(PIN_JOY_Y);
  jumpPressed = (y < JOY_UP_THRESH);
  duckPressed = (y > JOY_DOWN_THRESH);
  btnPressed  = (digitalRead(PIN_JOY_SW) == LOW);
}

// ─────────────────────────────────────────────────────────────
//  Splash screen — waits for joystick UP or click
// ─────────────────────────────────────────────────────────────
void waitForStart(unsigned int lastScore) {
  bool blink = false;

  // Show last score if available
  if (lastScore > 0) {
    lcd.clear();
    lcd.setCursor(2, 0); lcd.print("GAME  OVER!");
    lcd.setCursor(0, 1); lcd.print("Score: "); lcd.print(lastScore);
    lcd.setCursor(0, 2); lcd.print("HI:    "); lcd.print(hiScore);
    lcd.setCursor(0, 3); lcd.print("UP=Play again");
  } else {
    lcd.clear();
    lcd.setCursor(1, 0); lcd.print("IT FEST  DINO");
    lcd.setCursor(0, 2); lcd.print("UP  = Jump");
    lcd.setCursor(0, 3); lcd.print("DOWN= Duck");
  }

  // Blink "Press UP" on row 1 until joystick pushed
  while (true) {
    lcd.setCursor(0, 1);
    if (blink) {
      lcd.print("                ");
    } else {
      if (lastScore > 0) {
        // Already shown score on row 1 — just blink a prompt on row 3
        lcd.setCursor(0, 3);
        lcd.print(blink ? "                " : "UP = Play again ");
      } else {
        lcd.setCursor(2, 1);
        lcd.print("Push UP Start");
      }
    }
    blink = !blink;
    delay(300);

    readJoystick();
    if (jumpPressed || btnPressed) {
      delay(200); // debounce
      return;
    }
  }
}

// ─────────────────────────────────────────────────────────────
//  Short countdown before game starts
// ─────────────────────────────────────────────────────────────
void countdown() {
  for (int i = 3; i >= 1; i--) {
    lcd.clear();
    lcd.setCursor(4, 0); lcd.print("Ready?");
    lcd.setCursor(7, 1); lcd.print(i);
    delay(600);
  }
  lcd.clear();
  lcd.setCursor(5, 0); lcd.print("GO!");
  delay(350);
  lcd.clear();
}

// ─────────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Init I2C with explicit pins for ESP32
  Wire.begin(SDA_PIN, SCL_PIN);

  // Init LCD
  lcd.begin(16, 4);
  lcd.backlight();

  // Joystick
  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  analogReadResolution(12); // ESP32 ADC: 0–4095

  // Show splash on first boot
  waitForStart(0);
}

// ─────────────────────────────────────────────────────────────
//  MAIN LOOP — one full game per iteration
// ─────────────────────────────────────────────────────────────
void loop() {
  // ── Reset game state ──────────────────────────────────────
  initializeGraphics();
  countdown();

  byte heroPos             = HERO_POSITION_RUN_LOWER_1;
  byte newTerrainType      = TERRAIN_EMPTY;
  byte newTerrainDuration  = 1;
  unsigned int distance    = 0;
  bool isDucking           = false;
  unsigned long lastInfo   = 0;

  // Clear all 4 rows cleanly
  for (int r = 0; r < 4; r++) {
    lcd.setCursor(0, r);
    lcd.print("                ");
  }

  // ── Game loop ─────────────────────────────────────────────
  while (true) {

    // 1. Read input
    readJoystick();

    // 2. Input → hero action
    if (jumpPressed && !isDucking && heroPos <= HERO_POSITION_RUN_LOWER_2) {
      heroPos   = HERO_POSITION_JUMP_1;
    }

    if (duckPressed && (heroPos == HERO_POSITION_RUN_LOWER_1 ||
                        heroPos == HERO_POSITION_RUN_LOWER_2)) {
      isDucking = true;
      heroPos   = HERO_POSITION_DUCK_1;
    }
    if (!duckPressed && isDucking) {
      isDucking = false;
      heroPos   = HERO_POSITION_RUN_LOWER_1;
    }

    // 3. Scroll terrain
    advanceTerrain(terrainLower,
      newTerrainType == TERRAIN_LOWER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);
    advanceTerrain(terrainUpper,
      newTerrainType == TERRAIN_UPPER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);

    // 4. Generate new terrain tile
    if (--newTerrainDuration == 0) {
      if (newTerrainType == TERRAIN_EMPTY) {
        newTerrainType    = (random(3) == 0) ? TERRAIN_UPPER_BLOCK : TERRAIN_LOWER_BLOCK;
        newTerrainDuration = 2 + random(10);
      } else {
        newTerrainType    = TERRAIN_EMPTY;
        newTerrainDuration = 10 + random(10);
      }
    }

    // 5. Draw rows 0 & 1, check collision
    unsigned int score = distance >> 3;
    if (drawHero(heroPos, terrainUpper, terrainLower, score)) {
      break; // collision → game over
    }

    // 6. Advance hero state machine
    if (isDucking) {
      // Animate duck by alternating poses
      heroPos = (heroPos == HERO_POSITION_DUCK_1)
                ? HERO_POSITION_DUCK_2 : HERO_POSITION_DUCK_1;

    } else if (heroPos == HERO_POSITION_RUN_LOWER_2 ||
               heroPos == HERO_POSITION_JUMP_8) {
      heroPos = HERO_POSITION_RUN_LOWER_1;

    } else if (heroPos >= HERO_POSITION_JUMP_3 &&
               heroPos <= HERO_POSITION_JUMP_5 &&
               terrainLower[HERO_HORIZONTAL_POSITION] != SPRITE_TERRAIN_EMPTY) {
      heroPos = HERO_POSITION_RUN_UPPER_1;

    } else if (heroPos >= HERO_POSITION_RUN_UPPER_1 &&
               terrainLower[HERO_HORIZONTAL_POSITION] == SPRITE_TERRAIN_EMPTY) {
      heroPos = HERO_POSITION_JUMP_5;

    } else if (heroPos == HERO_POSITION_RUN_UPPER_2) {
      heroPos = HERO_POSITION_RUN_UPPER_1;

    } else {
      ++heroPos;
    }

    ++distance;

    // 7. Update info bar (rows 2 & 3) every 400 ms to avoid flicker
    if (millis() - lastInfo > 400) {
      lastInfo = millis();
      drawInfoBar(score);
    }

    delay(85); // ~11 fps
  }

  // ── Game over ─────────────────────────────────────────────
  unsigned int finalScore = distance >> 3;
  if (finalScore > hiScore) hiScore = finalScore;

  // Flash the collision spot
  for (int i = 0; i < 3; i++) {
    lcd.setCursor(HERO_HORIZONTAL_POSITION, 1);
    lcd.print("X");
    delay(180);
    lcd.setCursor(HERO_HORIZONTAL_POSITION, 1);
    lcd.print(" ");
    delay(180);
  }

  // Show game-over screen then wait for restart
  waitForStart(finalScore);
}

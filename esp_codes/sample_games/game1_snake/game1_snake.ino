// ============================================================
//  SNAKE — ESP32 · 4×20 I2C LCD · Joystick
//  Joystick X  = left / right
//  Joystick Y  = up   / down
//  Click SW    = restart after game over
// ============================================================
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SDA_PIN 21
#define SCL_PIN 22
LiquidCrystal_I2C lcd(0x27, 20, 4);

#define PIN_JOY_X  32
#define PIN_JOY_Y  34
#define PIN_JOY_SW 35

#define JOY_LOW   1000
#define JOY_HIGH  3000

// Grid = 20 cols × 4 rows
#define COLS 20
#define ROWS 4
#define MAX_LEN (COLS * ROWS)

// Custom chars
#define C_HEAD  1   // snake head
#define C_BODY  2   // snake body
#define C_FOOD  3   // food

byte HEAD_CHAR[] = {B00000,B01110,B11111,B11011,B11111,B01110,B00000,B00000};
byte BODY_CHAR[] = {B00000,B01110,B11111,B11111,B11111,B01110,B00000,B00000};
byte FOOD_CHAR[] = {B00100,B00100,B01110,B11111,B11111,B01110,B00100,B00000};

struct Point { int8_t x, y; };

Point snake[MAX_LEN];
int   snakeLen;
Point food;
int8_t dirX, dirY;
int8_t nextDirX, nextDirY;
unsigned int score;
unsigned int hiScore = 0;

// ── input ────────────────────────────────────────────────────
void readJoy(int8_t &dx, int8_t &dy, bool &btn) {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);
  btn   = digitalRead(PIN_JOY_SW) == LOW;
  dx = 0; dy = 0;
  if      (x < JOY_LOW)  dx = -1;
  else if (x > JOY_HIGH) dx =  1;
  else if (y < JOY_LOW)  dy = -1;
  else if (y > JOY_HIGH) dy =  1;
}

// ── food placement ───────────────────────────────────────────
void placeFood() {
  bool ok;
  do {
    ok   = true;
    food = {(int8_t)random(COLS), (int8_t)random(ROWS)};
    for (int i = 0; i < snakeLen; i++)
      if (snake[i].x == food.x && snake[i].y == food.y) { ok = false; break; }
  } while (!ok);
}

// ── init game ────────────────────────────────────────────────
void initGame() {
  snakeLen = 3;
  for (int i = 0; i < snakeLen; i++) {
    snake[i] = {(int8_t)(5 - i), 1};
  }
  dirX = 1; dirY = 0;
  nextDirX = 1; nextDirY = 0;
  score = 0;
  placeFood();
}

// ── draw ─────────────────────────────────────────────────────
void drawFrame() {
  // Build grid
  char grid[ROWS][COLS];
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      grid[r][c] = ' ';

  // Food
  grid[food.y][food.x] = C_FOOD;

  // Snake body
  for (int i = 1; i < snakeLen; i++)
    if (snake[i].x >= 0 && snake[i].x < COLS &&
        snake[i].y >= 0 && snake[i].y < ROWS)
      grid[snake[i].y][snake[i].x] = C_BODY;

  // Head
  grid[snake[0].y][snake[0].x] = C_HEAD;

  // Print rows
  for (int r = 0; r < ROWS; r++) {
    lcd.setCursor(0, r);
    for (int c = 0; c < COLS; c++)
      lcd.write(grid[r][c]);
  }

  // Score overlay top-right
  lcd.setCursor(14, 0);
  char buf[7]; sprintf(buf, "S:%4d", score);
  lcd.print(buf);
}

// ── splash ───────────────────────────────────────────────────
void splash(unsigned int last) {
  lcd.clear();
  if (last > 0) {
    lcd.setCursor(3, 0); lcd.print("** GAME OVER **");
    lcd.setCursor(0, 1); lcd.print("Score : "); lcd.print(last);
    lcd.setCursor(0, 2); lcd.print("Best  : "); lcd.print(hiScore);
    lcd.setCursor(0, 3); lcd.print("Click = restart");
  } else {
    lcd.setCursor(4, 0); lcd.print("S N A K E");
    lcd.setCursor(0, 1); lcd.print("Joystick = move");
    lcd.setCursor(0, 2); lcd.print("Eat food to grow");
    lcd.setCursor(0, 3); lcd.print("Click to START");
  }
  bool btn; int8_t dx, dy;
  while (true) {
    readJoy(dx, dy, btn);
    if (btn) { delay(300); return; }
    delay(80);
  }
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.begin(20, 4);
  lcd.backlight();
  lcd.createChar(C_HEAD, HEAD_CHAR);
  lcd.createChar(C_BODY, BODY_CHAR);
  lcd.createChar(C_FOOD, FOOD_CHAR);
  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  analogReadResolution(12);
  randomSeed(analogRead(33));
  splash(0);
}

void loop() {
  initGame();
  lcd.clear();

  unsigned long lastMove = 0;
  unsigned int  speed    = 300;  // ms per step (lower = faster)

  while (true) {
    // Read direction — allow buffering between frames for smoothness
    int8_t dx, dy; bool btn;
    readJoy(dx, dy, btn);
    // Only accept perpendicular turns (no 180° reversal)
    if (dx != 0 && dirX == 0) { nextDirX = dx; nextDirY = 0; }
    if (dy != 0 && dirY == 0) { nextDirX = 0;  nextDirY = dy; }

    if (millis() - lastMove >= speed) {
      lastMove = millis();

      // Commit buffered direction
      dirX = nextDirX; dirY = nextDirY;

      // New head position
      Point newHead = { (int8_t)(snake[0].x + dirX),
                        (int8_t)(snake[0].y + dirY) };

      // Wall collision
      if (newHead.x < 0 || newHead.x >= COLS ||
          newHead.y < 0 || newHead.y >= ROWS) break;

      // Self collision
      for (int i = 0; i < snakeLen; i++)
        if (snake[i].x == newHead.x && snake[i].y == newHead.y) goto dead;

      // Shift snake
      for (int i = snakeLen; i > 0; i--) snake[i] = snake[i - 1];
      snake[0] = newHead;

      // Eat food
      if (newHead.x == food.x && newHead.y == food.y) {
        snakeLen++;
        score++;
        placeFood();
        // Speed up every 5 foods, floor at 120ms
        if (speed > 120) speed = max(120u, speed - 15);
      }

      drawFrame();
    }

    delay(20);  // poll joystick frequently for smoothness
  }

  dead:
  if (score > hiScore) hiScore = score;
  // Flash head
  for (int i = 0; i < 4; i++) {
    lcd.setCursor(snake[0].x, snake[0].y);
    lcd.print(i % 2 == 0 ? "X" : " ");
    delay(200);
  }
  splash(score);
}

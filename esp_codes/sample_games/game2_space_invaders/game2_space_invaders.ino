// ============================================================
//  SPACE INVADERS — ESP32 · 4×20 I2C LCD · Joystick
//  Joystick X  = move ship left / right
//  Click SW    = shoot
//  Enemies march left/right and descend each wave
// ============================================================
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SDA_PIN 21
#define SCL_PIN 22
LiquidCrystal_I2C lcd(0x27, 20, 4);

#define PIN_JOY_X  32
#define PIN_JOY_Y  34
#define PIN_JOY_SW 35
#define JOY_LOW  1000
#define JOY_HIGH 3000

#define COLS 20
#define ROWS 4

// Custom chars
#define C_SHIP    1
#define C_BULLET  2
#define C_ENEMY1  3
#define C_ENEMY2  4
#define C_EXPL    5
#define C_LIFE    6

byte SHIP_CHAR[]   = {B00100,B01110,B11111,B11111,B10101,B00000,B00000,B00000};
byte BULLET_CHAR[] = {B00100,B00100,B00100,B00100,B00100,B00100,B00000,B00000};
byte ENEMY1_CHAR[] = {B01010,B11111,B10101,B11111,B01110,B00000,B01010,B00000};
byte ENEMY2_CHAR[] = {B10001,B01110,B11111,B10101,B11111,B01110,B00000,B00000};
byte EXPL_CHAR[]   = {B10101,B01010,B11111,B01110,B11111,B01010,B10101,B00000};
byte LIFE_CHAR[]   = {B00100,B01110,B11111,B11111,B01110,B00100,B00000,B00000};

// ── Game state ────────────────────────────────────────────────
struct Enemy { int8_t x, y; bool alive; };
#define MAX_ENEMIES 10

Enemy   enemies[MAX_ENEMIES];
int     enemyCount;
int8_t  enemyDirX = 1;
int     enemyMoveTimer = 0;
int     enemyMoveInterval = 8;  // frames between enemy steps

int8_t  shipX = 10;
bool    bulletActive = false;
int8_t  bulletX, bulletY;
int     bulletTimer = 0;

int     lives  = 3;
unsigned int score   = 0;
unsigned int hiScore = 0;
int     wave   = 1;

// ── helpers ───────────────────────────────────────────────────
void readJoy(int8_t &dx, bool &fire) {
  int x = analogRead(PIN_JOY_X);
  bool btn = digitalRead(PIN_JOY_SW) == LOW;
  dx   = (x < JOY_LOW) ? -1 : (x > JOY_HIGH) ? 1 : 0;
  fire = btn;
}

void spawnWave() {
  enemyCount = min(MAX_ENEMIES, 5 + wave);
  for (int i = 0; i < enemyCount; i++) {
    enemies[i] = { (int8_t)(2 + i * 2 % COLS), 0, true };
  }
  enemyDirX        = 1;
  enemyMoveInterval = max(3, 8 - wave);
  bulletActive     = false;
}

void initGame() {
  shipX      = 10;
  lives      = 3;
  score      = 0;
  wave       = 1;
  spawnWave();
}

// ── draw ─────────────────────────────────────────────────────
void drawFrame(int explX, int explY) {
  char grid[ROWS][COLS];
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      grid[r][c] = ' ';

  // Enemies
  for (int i = 0; i < enemyCount; i++) {
    if (!enemies[i].alive) continue;
    int ex = enemies[i].x, ey = enemies[i].y;
    if (ex >= 0 && ex < COLS && ey >= 0 && ey < ROWS)
      grid[ey][ex] = (wave % 2 == 0) ? C_ENEMY2 : C_ENEMY1;
  }

  // Bullet
  if (bulletActive && bulletX >= 0 && bulletX < COLS &&
      bulletY >= 0 && bulletY < ROWS)
    grid[bulletY][bulletX] = C_BULLET;

  // Explosion overlay
  if (explX >= 0 && explY >= 0)
    grid[explY][explX] = C_EXPL;

  // Ship on row 3
  if (shipX >= 0 && shipX < COLS)
    grid[3][shipX] = C_SHIP;

  for (int r = 0; r < ROWS; r++) {
    lcd.setCursor(0, r);
    for (int c = 0; c < COLS; c++) lcd.write(grid[r][c]);
  }

  // HUD on top right
  lcd.setCursor(13, 0);
  char buf[8]; sprintf(buf, "%5d", score);
  lcd.print(buf);

  // Lives
  lcd.setCursor(16, 3);
  for (int i = 0; i < 3; i++)
    lcd.write(i < lives ? (byte)C_LIFE : ' ');
}

// ── splash ────────────────────────────────────────────────────
void splash(bool gameOver) {
  lcd.clear();
  if (gameOver) {
    lcd.setCursor(2, 0); lcd.print("** GAME  OVER **");
    lcd.setCursor(0, 1); lcd.print("Score : "); lcd.print(score);
    lcd.setCursor(0, 2); lcd.print("Best  : "); lcd.print(hiScore);
    lcd.setCursor(0, 3); lcd.print("Click = restart");
  } else {
    lcd.setCursor(3, 0); lcd.print("SPACE INVADERS");
    lcd.setCursor(0, 1); lcd.print("X = move  Btn=fire");
    lcd.setCursor(0, 2); lcd.print("Defend Earth!");
    lcd.setCursor(0, 3); lcd.print("Click to START");
  }
  int8_t dx; bool fire;
  while (true) {
    readJoy(dx, fire);
    if (fire) { delay(300); return; }
    delay(80);
  }
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.begin(20, 4);
  lcd.backlight();
  lcd.createChar(C_SHIP,   SHIP_CHAR);
  lcd.createChar(C_BULLET, BULLET_CHAR);
  lcd.createChar(C_ENEMY1, ENEMY1_CHAR);
  lcd.createChar(C_ENEMY2, ENEMY2_CHAR);
  lcd.createChar(C_EXPL,   EXPL_CHAR);
  lcd.createChar(C_LIFE,   LIFE_CHAR);
  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  analogReadResolution(12);
  randomSeed(analogRead(33));
  splash(false);
}

void loop() {
  initGame();
  lcd.clear();

  unsigned long lastFrame = 0;
  int  frameInterval = 80;  // ms per frame
  int  frameCount    = 0;
  int  explX = -1, explY = -1, explTimer = 0;
  bool fireLast = false;

  while (true) {
    if (millis() - lastFrame < frameInterval) { delay(10); continue; }
    lastFrame = millis();
    frameCount++;

    // ── Input ───────────────────────────────────────────────
    int8_t dx; bool fire;
    readJoy(dx, fire);

    // Ship move — smooth: move every frame if joystick held
    shipX = constrain(shipX + dx, 0, COLS - 1);

    // Fire — only on fresh press
    if (fire && !fireLast && !bulletActive) {
      bulletActive = true;
      bulletX      = shipX;
      bulletY      = 2;
    }
    fireLast = fire;

    // ── Bullet move every 2 frames ──────────────────────────
    if (bulletActive && frameCount % 2 == 0) {
      bulletY--;
      if (bulletY < 0) { bulletActive = false; }
    }

    // ── Bullet-enemy collision ───────────────────────────────
    if (bulletActive) {
      for (int i = 0; i < enemyCount; i++) {
        if (!enemies[i].alive) continue;
        if (enemies[i].x == bulletX && enemies[i].y == bulletY) {
          enemies[i].alive = false;
          bulletActive     = false;
          score           += 10 * wave;
          explX = bulletX; explY = bulletY;
          explTimer = 3;
          break;
        }
      }
    }

    // ── Explosion timer ──────────────────────────────────────
    if (explTimer > 0) { explTimer--; if (explTimer == 0) { explX = explY = -1; } }

    // ── Enemy movement ───────────────────────────────────────
    enemyMoveTimer++;
    if (enemyMoveTimer >= enemyMoveInterval) {
      enemyMoveTimer = 0;
      // Check if any enemy hits a wall
      bool hitWall = false;
      for (int i = 0; i < enemyCount; i++) {
        if (!enemies[i].alive) continue;
        int nx = enemies[i].x + enemyDirX;
        if (nx < 0 || nx >= COLS) { hitWall = true; break; }
      }
      if (hitWall) {
        enemyDirX = -enemyDirX;
        // Drop one row
        for (int i = 0; i < enemyCount; i++) enemies[i].y++;
      } else {
        for (int i = 0; i < enemyCount; i++)
          if (enemies[i].alive) enemies[i].x += enemyDirX;
      }
    }

    // ── Enemy reaches ship row → lose a life ────────────────
    for (int i = 0; i < enemyCount; i++) {
      if (!enemies[i].alive) continue;
      if (enemies[i].y >= 3) {
        lives--;
        enemies[i].alive = false;
        // Flash
        for (int f = 0; f < 3; f++) {
          lcd.setCursor(shipX, 3); lcd.print("*");
          delay(150);
          lcd.setCursor(shipX, 3); lcd.print(" ");
          delay(150);
        }
        if (lives <= 0) goto dead;
      }
    }

    // ── Check wave clear ─────────────────────────────────────
    bool anyAlive = false;
    for (int i = 0; i < enemyCount; i++) if (enemies[i].alive) { anyAlive = true; break; }
    if (!anyAlive) {
      wave++;
      lcd.clear();
      lcd.setCursor(4, 1); lcd.print("WAVE "); lcd.print(wave); lcd.print("!");
      delay(1200);
      lcd.clear();
      spawnWave();
    }

    // ── Draw ─────────────────────────────────────────────────
    drawFrame(explX, explY);
    delay(10);
  }

  dead:
  if (score > hiScore) hiScore = score;
  splash(true);
}

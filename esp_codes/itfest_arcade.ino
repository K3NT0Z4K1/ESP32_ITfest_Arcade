// ============================================================
//  IT FEST ARCADE — ESP32 · 4×20 I2C LCD · Joystick only
//
//  3 games in one sketch — no SW button needed anywhere!
//
//  MENU
//    Joystick UP / DOWN  →  scroll through games
//    Joystick RIGHT held 1 s  →  launch selected game
//
//  IN-GAME (return to menu)
//    Dino  : after game over, hold RIGHT 1 s
//    Snake : after game over, hold RIGHT 1 s
//    Maze  : after game over, hold RIGHT 1 s
//
//  Wiring
//    VRX → GPIO 32   VRY → GPIO 34
//    VCC → 3.3V      GND → GND
//    SDA → GPIO 21   SCL → GPIO 22
// ============================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ── Hardware ─────────────────────────────────────────────────
#define SDA_PIN    21
#define SCL_PIN    22
#define PIN_JOY_X  32
#define PIN_JOY_Y  34
#define JOY_LOW   1000
#define JOY_HIGH  3000
#define COLS       20
#define ROWS        4

LiquidCrystal_I2C lcd(0x27, 20, 4);

// ── Shared joystick read (no SW) ─────────────────────────────
// Returns dx/dy each -1, 0, or 1
void joyRead(int8_t &dx, int8_t &dy) {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);
  dx = (x < JOY_LOW) ? -1 : (x > JOY_HIGH) ?  1 : 0;
  dy = (y < JOY_LOW) ? -1 : (y > JOY_HIGH) ?  1 : 0;
}

// Hold-right to confirm — shows a progress bar, returns true when held
// enough, false if joystick released early
bool holdRight(int holdMs = 900) {
  unsigned long start = 0;
  bool holding = false;

  // Progress bar on row 3
  // 20 chars wide, fills over holdMs
  const int barLen = 18;

  while (true) {
    int8_t dx, dy;
    joyRead(dx, dy);

    if (dx == 1) {
      if (!holding) { holding = true; start = millis(); }
      unsigned long elapsed = millis() - start;
      int filled = map(elapsed, 0, holdMs, 0, barLen);
      filled = constrain(filled, 0, barLen);

      // Draw progress bar
      lcd.setCursor(1, 3);
      lcd.print("[");
      for (int i = 0; i < barLen; i++)
        lcd.print(i < filled ? (char)0xFF : ' ');
      lcd.print("]");

      if (elapsed >= (unsigned long)holdMs) {
        lcd.setCursor(1, 3);
        lcd.print("                  ");
        return true;
      }
    } else {
      if (holding) {
        // Released early — clear bar
        holding = false;
        lcd.setCursor(1, 3);
        lcd.print("                  ");
        return false;
      }
    }
    delay(30);
  }
}

// Wait for any joystick movement (used after game over)
void waitAnyInput() {
  int8_t dx, dy;
  // Drain current position first
  delay(300);
  while (true) {
    joyRead(dx, dy);
    if (dx != 0 || dy != 0) { delay(200); return; }
    delay(40);
  }
}

// ── Utility: clear LCD ───────────────────────────────────────
void lcdClear() {
  for (int r = 0; r < ROWS; r++) {
    lcd.setCursor(0, r);
    lcd.print("                    ");
  }
}

// ── Shared hi-scores ─────────────────────────────────────────
unsigned int hiDino  = 0;
unsigned int hiSnake = 0;
unsigned int hiMaze  = 0;

// ============================================================
//  CUSTOM CHARACTERS
//  LCD only has 8 slots (1-7 + slot 0).
//  We reload chars when switching games.
// ============================================================

// ── Dino sprites ─────────────────────────────────────────────
byte D_RUN1[]  = {B01110,B01110,B11110,B01111,B11110,B11100,B01010,B01010};
byte D_RUN2[]  = {B01110,B01110,B11110,B01111,B11110,B11100,B10100,B01100};
byte D_JUMP[]  = {B01110,B01110,B11110,B01111,B11111,B11110,B01110,B00110};
byte D_DUCK[]  = {B00000,B00000,B01110,B11111,B11101,B11100,B01010,B01010};
byte D_CACTC[] = {B00100,B11111,B00100,B00101,B11111,B00100,B00100,B00100};
byte D_CACTR[] = {B00000,B00011,B00000,B00000,B00011,B00000,B00000,B00000};
byte D_CACTL[] = {B00000,B11000,B00000,B01000,B11000,B00000,B00000,B00000};

void loadDinoChars() {
  lcd.createChar(1, D_RUN1);
  lcd.createChar(2, D_RUN2);
  lcd.createChar(3, D_JUMP);
  lcd.createChar(4, D_DUCK);
  lcd.createChar(5, D_CACTC);
  lcd.createChar(6, D_CACTR);
  lcd.createChar(7, D_CACTL);
}

// ── Snake sprites ─────────────────────────────────────────────
byte S_HEAD[] = {B00000,B01110,B11111,B11011,B11111,B01110,B00000,B00000};
byte S_BODY[] = {B00000,B01110,B11111,B11111,B11111,B01110,B00000,B00000};
byte S_FOOD[] = {B00100,B00100,B01110,B11111,B11111,B01110,B00100,B00000};

void loadSnakeChars() {
  lcd.createChar(1, S_HEAD);
  lcd.createChar(2, S_BODY);
  lcd.createChar(3, S_FOOD);
}

// ── Maze sprites ──────────────────────────────────────────────
byte M_WALL[]   = {B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111};
byte M_PLAYER[] = {B00100,B01110,B00100,B01110,B10101,B00100,B01010,B10001};
byte M_EXIT[]   = {B11111,B10001,B10101,B10101,B10101,B10001,B11111,B00000};
byte M_COIN[]   = {B01110,B11011,B10101,B10101,B11011,B01110,B00000,B00000};
byte M_LIFE[]   = {B01010,B11111,B11111,B01110,B00100,B00000,B00000,B00000};

void loadMazeChars() {
  lcd.createChar(1, M_WALL);
  lcd.createChar(2, M_PLAYER);
  lcd.createChar(3, M_EXIT);
  lcd.createChar(4, M_COIN);
  lcd.createChar(5, M_LIFE);
}

// ============================================================
//  MAIN MENU
// ============================================================
#define GAME_DINO  0
#define GAME_SNAKE 1
#define GAME_MAZE  2
#define NUM_GAMES  3

const char* GAME_NAMES[NUM_GAMES] = {
  "  Dino Run  ",
  "   Snake    ",
  " Maze Runner"
};

const char* GAME_HINTS[NUM_GAMES] = {
  "UP=jump DN=duck ",
  "Joystick=move   ",
  "Navigate to EXIT"
};

const char* GAME_ICONS[NUM_GAMES] = { "Dino", "Snke", "Maze" };

int menuSelect = 0;

// Draw the menu — 3 items visible, selected highlighted with arrows
void drawMenu() {
  lcdClear();
  lcd.setCursor(3, 0);
  lcd.print("IT FEST ARCADE");

  // Show 3 game entries on rows 1-3
  // We always show all 3 since NUM_GAMES == 3
  for (int i = 0; i < NUM_GAMES; i++) {
    lcd.setCursor(0, i + 1);
    if (i == menuSelect) {
      lcd.print(">");
    } else {
      lcd.print(" ");
    }
    lcd.print(GAME_NAMES[i]);
    if (i == menuSelect) {
      lcd.print("<");
    }
  }
}

// Draw game-over screen and wait for player to return to menu
// Returns when player holds RIGHT
void gameOverScreen(const char* gameName, unsigned int score, unsigned int hi) {
  lcdClear();
  lcd.setCursor(2, 0); lcd.print("** GAME  OVER **");
  lcd.setCursor(0, 1); lcd.print("Score : "); lcd.print(score);
  lcd.setCursor(0, 2); lcd.print("Best  : "); lcd.print(hi);
  lcd.setCursor(0, 3); lcd.print("Hold>> for menu ");

  while (true) {
    if (holdRight(900)) return;
  }
}

// Returns the selected game index
int runMenu() {
  drawMenu();

  int8_t lastDY = 0;
  unsigned long lastScroll = 0;

  // Show hint at bottom of selected row
  while (true) {
    int8_t dx, dy;
    joyRead(dx, dy);

    // Scroll menu UP / DOWN
    if (dy != 0 && dy != lastDY) {
      menuSelect = (menuSelect + dy + NUM_GAMES) % NUM_GAMES;
      drawMenu();
      lastScroll = millis();
    }
    lastDY = dy;

    // Show hint line while idle
    lcd.setCursor(2, menuSelect + 1);
    lcd.print(GAME_NAMES[menuSelect]);

    // Confirm with hold RIGHT — show progress on row below selected
    if (dx == 1) {
      // Temporarily use row 3 area for bar regardless of selection
      lcd.setCursor(0, 3);
      lcd.print("Hold >> to play ");
      if (holdRight(900)) return menuSelect;
      // If released early, redraw menu
      drawMenu();
    }

    delay(80);
  }
}

// ============================================================
//  GAME 1 — DINO RUN
// ============================================================
#define DINO_HERO_COL    2
#define DINO_JUMP_PHASES 9
const int8_t DINO_JUMP_ROW[DINO_JUMP_PHASES] = {2,2,1,1,0,0,1,1,2};

#define DINO_TERRAIN_W 20
static char dinoTerrainUpper[DINO_TERRAIN_W + 1];
static char dinoTerrainLower[DINO_TERRAIN_W + 1];

struct DinoObs { int col; bool active; };
static DinoObs dinoObs;

void dinoClearTerrain() {
  for (int c = 0; c < DINO_TERRAIN_W; c++) {
    dinoTerrainUpper[c] = ' ';
    dinoTerrainLower[c] = ' ';
  }
  dinoTerrainUpper[DINO_TERRAIN_W] = '\0';
  dinoTerrainLower[DINO_TERRAIN_W] = '\0';
}

void dinoScrollTerrain() {
  for (int c = 0; c < DINO_TERRAIN_W - 1; c++) {
    dinoTerrainUpper[c] = dinoTerrainUpper[c + 1];
    dinoTerrainLower[c] = dinoTerrainLower[c + 1];
  }
  dinoTerrainUpper[DINO_TERRAIN_W - 1] = ' ';
  dinoTerrainLower[DINO_TERRAIN_W - 1] = ' ';
}

bool dinoCollide(int heroRow) {
  // Check lower terrain rows 2 & 3 (LCD rows 2/3 map to lower terrain)
  // We store cactus on dinoTerrainLower (row 2)
  if (heroRow == 2) {
    char cell = dinoTerrainLower[DINO_HERO_COL];
    return (cell != ' ');
  }
  return false;
}

void dinoRender(int heroRow, byte heroSprite, unsigned int score) {
  // Build display buffers
  char upper[DINO_TERRAIN_W + 1];
  char lower[DINO_TERRAIN_W + 1];
  memcpy(upper, dinoTerrainUpper, DINO_TERRAIN_W + 1);
  memcpy(lower, dinoTerrainLower, DINO_TERRAIN_W + 1);

  // Stamp hero
  if (heroRow <= 1) upper[DINO_HERO_COL] = heroSprite;
  else              lower[DINO_HERO_COL] = heroSprite;

  // Score
  char scoreBuf[7]; sprintf(scoreBuf, "%5d", score);
  // Overlay score top-right (cols 15-19)
  for (int i = 0; i < 5; i++) upper[15 + i] = scoreBuf[i];

  // Print rows 0-3
  // Row 0: sky (empty for jump arc apex)
  lcd.setCursor(0, 0);
  for (int c = 0; c < DINO_TERRAIN_W; c++) {
    if (heroRow == 0 && c == DINO_HERO_COL) lcd.write(heroSprite);
    else lcd.write(upper[c] == ' ' ? ' ' : upper[c]);
  }

  // Row 1: mid-air
  lcd.setCursor(0, 1);
  for (int c = 0; c < DINO_TERRAIN_W; c++) {
    if (heroRow == 1 && c == DINO_HERO_COL) lcd.write(heroSprite);
    else lcd.write(' ');
  }

  // Row 2: ground (terrain + cactus)
  lcd.setCursor(0, 2);
  for (int c = 0; c < DINO_TERRAIN_W; c++) {
    if (heroRow == 2 && c == DINO_HERO_COL) lcd.write(heroSprite);
    else lcd.write(lower[c]);
  }

  // Row 3: duck lane
  lcd.setCursor(0, 3);
  for (int c = 0; c < DINO_TERRAIN_W; c++) {
    if (heroRow == 3 && c == DINO_HERO_COL) lcd.write(heroSprite);
    else lcd.write(' ');
  }

  // Score on row 0 right
  lcd.setCursor(15, 0);
  lcd.print(scoreBuf);
}

void runDino() {
  loadDinoChars();
  lcdClear();

  // Splash
  lcdClear();
  lcd.setCursor(3, 0); lcd.print("** DINO RUN **");
  lcd.setCursor(0, 1); lcd.print("UP  = jump");
  lcd.setCursor(0, 2); lcd.print("DOWN= duck");
  lcd.setCursor(0, 3); lcd.print("Hold>> to start ");
  while (true) { if (holdRight(900)) break; }
  lcdClear();

  dinoClearTerrain();
  dinoObs.active = false;

  int8_t  jumpPhase       = -1;
  int     heroRow         = 2;
  bool    isDucking       = false;
  byte    heroSprite      = 1;
  bool    runFrame        = false;
  unsigned int distance   = 0;
  unsigned int nextObsDist = 18 + random(18);

  unsigned long lastFrame = 0;
  int8_t lastDY = 0;

  while (true) {
    if (millis() - lastFrame < 90) { delay(10); continue; }
    lastFrame = millis();

    int8_t dx, dy;
    joyRead(dx, dy);

    // Jump
    if (dy == -1 && lastDY != -1 && jumpPhase < 0 && !isDucking && heroRow == 2)
      jumpPhase = 0;

    // Duck
    if (dy == 1 && jumpPhase < 0) {
      isDucking = true; heroRow = 3;
    } else if (dy != 1 && jumpPhase < 0) {
      isDucking = false; heroRow = 2;
    }

    // Advance jump arc
    if (jumpPhase >= 0) {
      heroRow   = DINO_JUMP_ROW[jumpPhase];
      isDucking = false;
      jumpPhase++;
      if (jumpPhase >= DINO_JUMP_PHASES) jumpPhase = -1;
    }

    lastDY = dy;

    // Sprite
    runFrame = !runFrame;
    if      (jumpPhase >= 0) heroSprite = 3;
    else if (isDucking)      heroSprite = 4;
    else                     heroSprite = runFrame ? 1 : 2;

    // Scroll terrain
    dinoScrollTerrain();

    // Obstacle
    if (!dinoObs.active) {
      if (distance >= nextObsDist) {
        dinoObs.col    = DINO_TERRAIN_W - 1;
        dinoObs.active = true;
        nextObsDist    = distance + 16 + random(20);
      }
    } else {
      dinoObs.col--;
      if (dinoObs.col < 0) {
        dinoObs.active = false;
      } else {
        // Place 3-char cactus on lower terrain
        if (dinoObs.col - 1 >= 0 && dinoObs.col - 1 < DINO_TERRAIN_W)
          dinoTerrainLower[dinoObs.col - 1] = 7; // left arm
        if (dinoObs.col     >= 0 && dinoObs.col     < DINO_TERRAIN_W)
          dinoTerrainLower[dinoObs.col]     = 5; // centre
        if (dinoObs.col + 1 >= 0 && dinoObs.col + 1 < DINO_TERRAIN_W)
          dinoTerrainLower[dinoObs.col + 1] = 6; // right arm
      }
    }

    // Collision
    if (dinoCollide(heroRow)) {
      for (int f = 0; f < 4; f++) {
        lcd.setCursor(DINO_HERO_COL, heroRow);
        lcd.print(f % 2 == 0 ? "X" : " ");
        delay(160);
      }
      break;
    }

    unsigned int score = distance >> 3;
    dinoRender(heroRow, heroSprite, score);
    distance++;
  }

  unsigned int finalScore = distance >> 3;
  if (finalScore > hiDino) hiDino = finalScore;
  gameOverScreen("Dino", finalScore, hiDino);
}

// ============================================================
//  GAME 2 — SNAKE
// ============================================================
#define SNAKE_COLS 20
#define SNAKE_ROWS  4
#define SNAKE_MAX  (SNAKE_COLS * SNAKE_ROWS)

struct Pt { int8_t x, y; };
Pt   snk[SNAKE_MAX];
int  snkLen;
Pt   snkFood;
int8_t snkDX, snkDY, snkNDX, snkNDY;
unsigned int snkScore;

void snkPlaceFood() {
  bool ok;
  do {
    ok = true;
    snkFood = {(int8_t)random(SNAKE_COLS), (int8_t)random(SNAKE_ROWS)};
    for (int i = 0; i < snkLen; i++)
      if (snk[i].x == snkFood.x && snk[i].y == snkFood.y) { ok = false; break; }
  } while (!ok);
}

void snkInit() {
  snkLen = 3;
  for (int i = 0; i < snkLen; i++) snk[i] = {(int8_t)(6 - i), 1};
  snkDX = 1; snkDY = 0; snkNDX = 1; snkNDY = 0;
  snkScore = 0;
  snkPlaceFood();
}

void snkDraw() {
  char grid[SNAKE_ROWS][SNAKE_COLS];
  for (int r = 0; r < SNAKE_ROWS; r++)
    for (int c = 0; c < SNAKE_COLS; c++)
      grid[r][c] = ' ';

  grid[snkFood.y][snkFood.x] = 3;  // food

  for (int i = 1; i < snkLen; i++)
    if (snk[i].x >= 0 && snk[i].x < SNAKE_COLS &&
        snk[i].y >= 0 && snk[i].y < SNAKE_ROWS)
      grid[snk[i].y][snk[i].x] = 2; // body

  grid[snk[0].y][snk[0].x] = 1; // head

  for (int r = 0; r < SNAKE_ROWS; r++) {
    lcd.setCursor(0, r);
    for (int c = 0; c < SNAKE_COLS; c++) lcd.write(grid[r][c]);
  }

  // Score top-right
  lcd.setCursor(13, 0);
  char buf[8]; sprintf(buf, "S:%4d", snkScore);
  lcd.print(buf);
}

void runSnake() {
  loadSnakeChars();
  lcdClear();

  // Splash
  lcd.setCursor(4, 0); lcd.print("** SNAKE **");
  lcd.setCursor(0, 1); lcd.print("Joystick = steer");
  lcd.setCursor(0, 2); lcd.print("Eat   to  grow!");
  lcd.setCursor(0, 3); lcd.print("Hold>> to start ");
  while (true) { if (holdRight(900)) break; }

  snkInit();
  lcdClear();

  unsigned long lastMove = 0;
  unsigned int  speed   = 280;

  while (true) {
    int8_t dx, dy;
    joyRead(dx, dy);

    // Buffer turn — no 180 reversal
    if (dx != 0 && snkDX == 0) { snkNDX = dx; snkNDY = 0; }
    if (dy != 0 && snkDY == 0) { snkNDX = 0;  snkNDY = dy; }

    if (millis() - lastMove >= speed) {
      lastMove = millis();
      snkDX = snkNDX; snkDY = snkNDY;

      Pt newHead = {(int8_t)(snk[0].x + snkDX), (int8_t)(snk[0].y + snkDY)};

      // Wall collision
      if (newHead.x < 0 || newHead.x >= SNAKE_COLS ||
          newHead.y < 0 || newHead.y >= SNAKE_ROWS) goto snkDead;

      // Self collision
      for (int i = 0; i < snkLen; i++)
        if (snk[i].x == newHead.x && snk[i].y == newHead.y) goto snkDead;

      // Shift
      for (int i = snkLen; i > 0; i--) snk[i] = snk[i - 1];
      snk[0] = newHead;

      // Eat
      if (newHead.x == snkFood.x && newHead.y == snkFood.y) {
        snkLen++; snkScore++;
        snkPlaceFood();
        if (speed > 120) speed = max(120u, speed - 12);
      }

      snkDraw();
    }
    delay(20);
  }

  snkDead:
  if (snkScore > hiSnake) hiSnake = snkScore;
  for (int f = 0; f < 4; f++) {
    lcd.setCursor(snk[0].x, snk[0].y);
    lcd.print(f % 2 == 0 ? "X" : " ");
    delay(180);
  }
  gameOverScreen("Snake", snkScore, hiSnake);
}

// ============================================================
//  GAME 3 — MAZE RUNNER
// ============================================================
#define MAZE_W 40
#define MAZE_H  4

uint8_t mz[MAZE_H][MAZE_W];
int8_t  mzPX, mzPY;
int     mzCamX;
int     mzLives, mzLevel;
unsigned int mzScore;
int     mzCoins;

void mzGenerate() {
  memset(mz, 0, sizeof(mz));
  for (int c = 0; c < MAZE_W; c++) { mz[0][c] = 1; mz[3][c] = 1; }

  for (int c = 3; c < MAZE_W - 2; c++) {
    if (random(3) == 0) {
      // Wall pillar — only block one of the two open rows
      mz[1 + random(2)][c] = 1;
      // Guarantee the other row is clear (no two consecutive pillars)
      if (c > 3 && mz[1][c-1] == 1) mz[1][c] = 0;
      if (c > 3 && mz[2][c-1] == 1) mz[2][c] = 0;
    }
    if (mz[1][c] == 0 && random(6) == 0) mz[1][c] = 2; // coin
    if (mz[2][c] == 0 && random(6) == 0) mz[2][c] = 2;
  }

  // Clear start
  for (int r = 1; r <= 2; r++) for (int c = 0; c < 3; c++) mz[r][c] = 0;

  // Exit
  mz[1][MAZE_W - 1] = 3;
  mz[2][MAZE_W - 1] = 3;

  mzPX = 1; mzPY = 1;
  mzCamX = 0;
  mzCoins = 0;
}

void mzDraw() {
  for (int r = 0; r < MAZE_H; r++) {
    lcd.setCursor(0, r);
    for (int c = 0; c < COLS; c++) {
      int mc = mzCamX + c;
      if (mc < 0 || mc >= MAZE_W) { lcd.write((byte)1); continue; }
      if (mc == mzPX && r == mzPY) { lcd.write((byte)2); continue; }
      switch (mz[r][mc]) {
        case 1: lcd.write((byte)1); break;
        case 2: lcd.write((byte)4); break;
        case 3: lcd.write((byte)3); break;
        default: lcd.write(' ');    break;
      }
    }
  }
  // HUD
  lcd.setCursor(13, 0);
  char buf[8]; sprintf(buf, "%5d", mzScore);
  lcd.print(buf);
  lcd.setCursor(17, 3);
  for (int i = 0; i < 3; i++) lcd.write(i < mzLives ? (byte)5 : ' ');
}

void runMaze() {
  loadMazeChars();
  lcdClear();

  // Splash
  lcd.setCursor(3, 0); lcd.print("* MAZE RUNNER *");
  lcd.setCursor(0, 1); lcd.print("Navigate to EXIT");
  lcd.setCursor(0, 2); lcd.print("Collect coins!");
  lcd.setCursor(0, 3); lcd.print("Hold>> to start ");
  while (true) { if (holdRight(900)) break; }

  mzLives = 3; mzScore = 0; mzLevel = 1;
  mzGenerate();
  lcdClear();

  int8_t lastDX = 0, lastDY = 0;
  unsigned long lastMove = 0;
  int moveDelay = 175;

  while (true) {
    int8_t dx, dy;
    joyRead(dx, dy);

    bool canMove = false;
    if (dx != 0 || dy != 0) {
      if (dx != lastDX || dy != lastDY ||
          millis() - lastMove >= (unsigned long)moveDelay) {
        canMove = true; lastDX = dx; lastDY = dy; lastMove = millis();
      }
    } else { lastDX = 0; lastDY = 0; }

    if (canMove) {
      int8_t nx = constrain(mzPX + dx, 0, MAZE_W - 1);
      int8_t ny = constrain(mzPY + dy, 0, MAZE_H - 1);
      uint8_t cell = mz[ny][nx];

      if (cell == 1) {
        mzLives--;
        for (int f = 0; f < 3; f++) {
          lcd.setCursor(mzPX - mzCamX, mzPY);
          lcd.print(f % 2 == 0 ? "!" : " ");
          delay(120);
        }
        if (mzLives <= 0) {
          if (mzScore > hiMaze) hiMaze = mzScore;
          gameOverScreen("Maze", mzScore, hiMaze);
          return;
        }
      } else if (cell == 2) {
        mz[ny][nx] = 0; mzScore += 5 * mzLevel; mzCoins++;
        mzPX = nx; mzPY = ny;
      } else if (cell == 3) {
        mzScore += 50 * mzLevel;
        if (mzScore > hiMaze) hiMaze = mzScore;
        lcdClear();
        lcd.setCursor(3, 1); lcd.print("LEVEL CLEAR!");
        lcd.setCursor(3, 2); lcd.print("Level "); lcd.print(mzLevel + 1);
        delay(1800);
        mzLevel++;
        moveDelay = max(80, moveDelay - 10);
        mzGenerate();
        lcdClear();
      } else {
        mzPX = nx; mzPY = ny;
      }

      mzCamX = constrain(mzPX - 3, 0, MAZE_W - COLS);
    }

    mzDraw();
    delay(30);
  }
}

// ============================================================
//  SETUP & LOOP
// ============================================================
void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.begin(20, 4);
  lcd.backlight();
  analogReadResolution(12);
  randomSeed(analogRead(33));

  // Boot screen
  lcdClear();
  lcd.setCursor(2, 1); lcd.print("IT FEST ARCADE");
  lcd.setCursor(4, 2); lcd.print("Loading...");
  delay(1200);
}

void loop() {
  int choice = runMenu();

  switch (choice) {
    case GAME_DINO:  runDino();  break;
    case GAME_SNAKE: runSnake(); break;
    case GAME_MAZE:  runMaze();  break;
  }

  // Brief pause then back to menu
  delay(400);
}

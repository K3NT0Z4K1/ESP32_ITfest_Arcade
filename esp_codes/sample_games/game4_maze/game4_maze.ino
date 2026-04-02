// ============================================================
//  MAZE RUNNER — ESP32 · 4×20 I2C LCD · Joystick
//
//  Navigate through a scrolling maze. The maze is wider than
//  the screen — it scrolls horizontally as you move right.
//  Reach the exit (column 39) to win and advance a level.
//
//  Joystick X  = move left / right
//  Joystick Y  = move up   / down
//  Click SW    = restart after death
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

#define COLS 20
#define ROWS  4

// Maze dimensions (wider than screen so it scrolls)
#define MAZE_W 40
#define MAZE_H  4

// Custom chars
#define C_WALL   1
#define C_PLAYER 2
#define C_EXIT   3
#define C_COIN   4
#define C_LIFE   5

byte WALL_CHAR[]   = {B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111};
byte PLAYER_CHAR[] = {B00100,B01110,B00100,B01110,B10101,B00100,B01010,B10001};
byte EXIT_CHAR[]   = {B11111,B10001,B10101,B10101,B10101,B10001,B11111,B00000};
byte COIN_CHAR[]   = {B01110,B11011,B10101,B10101,B11011,B01110,B00000,B00000};
byte LIFE_CHAR[]   = {B01010,B11111,B11111,B01110,B00100,B00000,B00000,B00000};

// ── Maze storage (0=open 1=wall 2=coin 3=exit) ───────────────
uint8_t maze[MAZE_H][MAZE_W];

int8_t playerX, playerY;  // position in maze coords
int    camX;              // leftmost visible column
int    lives  = 3;
int    coins  = 0;
unsigned int score   = 0;
unsigned int hiScore = 0;
int    level  = 1;

// ── Generate maze ─────────────────────────────────────────────
// Simple hand-crafted-style random maze:
// Top and bottom rows are walls. Middle rows have random wall
// columns with guaranteed open path from left to right.
void generateMaze() {
  memset(maze, 0, sizeof(maze));

  // Border walls
  for (int c = 0; c < MAZE_W; c++) {
    maze[0][c] = 1;
    maze[3][c] = 1;
  }

  // Internal walls with gaps — ensure at least one open path
  // We guarantee rows 1 and 2 are always passable somewhere
  for (int c = 2; c < MAZE_W - 2; c++) {
    if (random(3) == 0) {
      // Place a wall pillar, but leave at least one of row 1/2 open
      int wallRow = (random(2) == 0) ? 1 : 2;
      maze[wallRow][c] = 1;
    }
    // Coins in open cells
    if (maze[1][c] == 0 && maze[2][c] == 0 && random(5) == 0)
      maze[1 + random(2)][c] = 2;
  }

  // Clear start area
  for (int r = 1; r <= 2; r++)
    for (int c = 0; c < 3; c++)
      maze[r][c] = 0;

  // Place exit
  maze[1][MAZE_W - 1] = 3;
  maze[2][MAZE_W - 1] = 3;

  // Player start
  playerX = 1; playerY = 1;
  camX = 0;
  coins = 0;
}

void initGame() {
  lives  = 3;
  score  = 0;
  level  = 1;
  generateMaze();
}

// ── input ────────────────────────────────────────────────────
void readJoy(int8_t &dx, int8_t &dy, bool &btn) {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);
  btn   = digitalRead(PIN_JOY_SW) == LOW;
  dx    = (x < JOY_LOW) ? -1 : (x > JOY_HIGH) ? 1 : 0;
  dy    = (y < JOY_LOW) ? -1 : (y > JOY_HIGH) ? 1 : 0;
}

// ── draw ─────────────────────────────────────────────────────
void drawFrame() {
  for (int r = 0; r < ROWS; r++) {
    lcd.setCursor(0, r);
    for (int c = 0; c < COLS; c++) {
      int mc = camX + c;
      if (mc < 0 || mc >= MAZE_W) { lcd.write((byte)C_WALL); continue; }

      // Player
      if (mc == playerX && r == playerY) { lcd.write((byte)C_PLAYER); continue; }

      uint8_t cell = maze[r][mc];
      switch (cell) {
        case 1: lcd.write((byte)C_WALL);   break;
        case 2: lcd.write((byte)C_COIN);   break;
        case 3: lcd.write((byte)C_EXIT);   break;
        default: lcd.write(' ');           break;
      }
    }
  }

  // HUD: score + lives on row 0 right side (overwrites edge)
  lcd.setCursor(13, 0);
  char buf[8]; sprintf(buf, "%4d", score);
  lcd.print(buf);

  // Lives icons
  lcd.setCursor(17, 3);
  for (int i = 0; i < 3; i++)
    lcd.write(i < lives ? (byte)C_LIFE : ' ');
}

// ── splash ────────────────────────────────────────────────────
void splash(bool over) {
  lcd.clear();
  if (over) {
    lcd.setCursor(2, 0); lcd.print("** GAME  OVER **");
    lcd.setCursor(0, 1); lcd.print("Score : "); lcd.print(score);
    lcd.setCursor(0, 2); lcd.print("Best  : "); lcd.print(hiScore);
    lcd.setCursor(0, 3); lcd.print("Click = restart");
  } else {
    lcd.setCursor(4, 0); lcd.print("MAZE RUNNER");
    lcd.setCursor(0, 1); lcd.print("Joystick = move");
    lcd.setCursor(0, 2); lcd.print("Reach the EXIT!");
    lcd.setCursor(0, 3); lcd.print("Click to START");
  }
  int8_t dx, dy; bool btn;
  while (true) {
    readJoy(dx, dy, btn);
    if (btn) { delay(300); return; }
    delay(80);
  }
}

// ── level clear screen ────────────────────────────────────────
void showLevelClear() {
  lcd.clear();
  lcd.setCursor(3, 0); lcd.print("LEVEL CLEAR!");
  lcd.setCursor(0, 1); lcd.print("Level  "); lcd.print(level);
  lcd.setCursor(0, 2); lcd.print("Score: "); lcd.print(score);
  lcd.setCursor(0, 3); lcd.print("Get ready...");
  delay(2000);
  lcd.clear();
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.begin(20, 4);
  lcd.backlight();
  lcd.createChar(C_WALL,   WALL_CHAR);
  lcd.createChar(C_PLAYER, PLAYER_CHAR);
  lcd.createChar(C_EXIT,   EXIT_CHAR);
  lcd.createChar(C_COIN,   COIN_CHAR);
  lcd.createChar(C_LIFE,   LIFE_CHAR);
  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  analogReadResolution(12);
  randomSeed(analogRead(33));
  splash(false);
}

void loop() {
  initGame();
  lcd.clear();

  int8_t lastDX = 0, lastDY = 0;
  unsigned long lastMove = 0;
  int moveDelay = 180;  // ms between moves — lower = faster response

  while (true) {
    int8_t dx, dy; bool btn;
    readJoy(dx, dy, btn);

    // Smooth movement: allow move on fresh input OR held after delay
    bool canMove = false;
    if ((dx != 0 || dy != 0)) {
      if ((dx != lastDX || dy != lastDY) ||
          (millis() - lastMove >= (unsigned long)moveDelay)) {
        canMove  = true;
        lastDX   = dx; lastDY = dy;
        lastMove = millis();
      }
    } else {
      lastDX = 0; lastDY = 0;
    }

    if (canMove && (dx != 0 || dy != 0)) {
      int8_t nx = playerX + dx;
      int8_t ny = playerY + dy;

      // Bounds
      nx = constrain(nx, 0, MAZE_W - 1);
      ny = constrain(ny, 0, MAZE_H - 1);

      uint8_t cell = maze[ny][nx];

      if (cell == 1) {
        // Wall hit — lose a life
        lives--;
        // Flash player position
        for (int f = 0; f < 3; f++) {
          lcd.setCursor(playerX - camX, playerY);
          lcd.print(f % 2 == 0 ? "!" : " ");
          delay(120);
        }
        if (lives <= 0) {
          if (score > hiScore) hiScore = score;
          splash(true);
          initGame();
          lcd.clear();
          continue;
        }
      } else if (cell == 2) {
        // Coin!
        maze[ny][nx] = 0;
        coins++;
        score += 5 * level;
        playerX = nx; playerY = ny;
      } else if (cell == 3) {
        // Exit reached!
        score += 50 * level;
        if (score > hiScore) hiScore = score;
        showLevelClear();
        level++;
        moveDelay = max(80, moveDelay - 10);
        generateMaze();
        lcd.clear();
        continue;
      } else {
        playerX = nx; playerY = ny;
      }

      // Scroll camera to keep player visible (centered in 3rd column from left)
      camX = constrain(playerX - 3, 0, MAZE_W - COLS);
    }

    drawFrame();
    delay(30);
  }
}

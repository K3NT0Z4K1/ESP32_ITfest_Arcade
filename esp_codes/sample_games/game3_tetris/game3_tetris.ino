// ============================================================
//  TETRIS (simplified) — ESP32 · 4×20 I2C LCD · Joystick
//
//  Board   : 10 cols × 4 rows (right half of screen)
//  Left 10 : score / level / next piece preview
//
//  Joystick X → move piece left / right
//  Joystick Y → rotate (up) / soft-drop (down)
//  Click SW   → hard drop
//
//  Pieces: I  O  T  S  Z  (5 types, 1 cell tall for 4-row LCD)
//  Each piece is a 1-row strip so they fit a 4-row display.
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

// Board occupies columns 10-19, rows 0-3
#define B_COLS  10
#define B_ROWS   4
#define B_OFFX  10   // board starts at LCD column 10

// Custom chars
#define C_BLOCK  1   // filled cell
#define C_PIECE  2   // active piece cell
#define C_EMPTY  ' '

byte BLOCK_CHAR[] = {B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111};
byte PIECE_CHAR[] = {B11111,B10001,B10001,B10001,B10001,B10001,B10001,B11111};

// Board: 0=empty 1=locked block
uint8_t board[B_ROWS][B_COLS];

// ── Pieces: each defined as column offsets from anchor (row fixed per piece) ──
// Piece format: {width, col0, col1, col2, col3}
// All pieces are 1 row tall — fits perfectly in 4 rows
struct PieceDef { uint8_t w; int8_t cols[4]; };
const PieceDef PIECES[] = {
  {4, {0,1,2,3}},   // I — 4 wide
  {2, {0,1,0,1}},   // O — 2×2 (we only use top row)
  {3, {0,1,2,1}},   // T — 3 wide, T-shape flattened
  {3, {1,0,1,2}},   // S
  {3, {0,1,2,0}},   // Z
};
#define NUM_PIECES 5

// Active piece
int8_t  pieceType;
int8_t  pieceX;    // leftmost col on board
int8_t  pieceRow;  // which row the piece is on
uint8_t pieceW;    // width of current piece
int8_t  pieceCols[4]; // relative col offsets

int8_t  nextType;
unsigned int score   = 0;
unsigned int hiScore = 0;
int     level  = 1;
int     linesCleared = 0;

unsigned long lastDrop;
int  dropInterval = 700;  // ms between auto-drops

// ── helpers ───────────────────────────────────────────────────
void readJoy(int8_t &dx, int8_t &dy, bool &btn) {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);
  btn   = digitalRead(PIN_JOY_SW) == LOW;
  dx    = (x < JOY_LOW) ? -1 : (x > JOY_HIGH) ? 1 : 0;
  dy    = (y < JOY_LOW) ? -1 : (y > JOY_HIGH) ? 1 : 0;
}

void spawnPiece(int8_t type) {
  pieceType = type;
  pieceW    = PIECES[type].w;
  memcpy(pieceCols, PIECES[type].cols, 4);
  pieceX   = (B_COLS / 2) - (pieceW / 2);
  pieceRow = 0;
}

void newPiece() {
  spawnPiece(nextType);
  nextType = random(NUM_PIECES);
}

// Check if piece at (px, pr) is valid
bool pieceValid(int8_t px, int8_t pr) {
  for (int i = 0; i < (int)pieceW; i++) {
    int c = px + pieceCols[i];
    if (c < 0 || c >= B_COLS) return false;
    if (pr < 0 || pr >= B_ROWS) return false;
    if (board[pr][c]) return false;
  }
  return true;
}

// Lock piece into board
void lockPiece() {
  for (int i = 0; i < (int)pieceW; i++)
    board[pieceRow][pieceX + pieceCols[i]] = 1;
}

// Check and clear full rows — on 4-row LCD, a full row = all 10 cols filled
int clearRows() {
  int cleared = 0;
  for (int r = B_ROWS - 1; r >= 0; r--) {
    bool full = true;
    for (int c = 0; c < B_COLS; c++) if (!board[r][c]) { full = false; break; }
    if (full) {
      // Shift everything above down
      for (int rr = r; rr > 0; rr--)
        memcpy(board[rr], board[rr - 1], B_COLS);
      memset(board[0], 0, B_COLS);
      cleared++;
      r++;  // recheck same row
    }
  }
  return cleared;
}

void initGame() {
  memset(board, 0, sizeof(board));
  score  = 0; level = 1; linesCleared = 0;
  dropInterval = 700;
  nextType = random(NUM_PIECES);
  newPiece();
}

// ── draw ─────────────────────────────────────────────────────
void drawFrame() {
  // ── Right side: board (cols 10-19) ──────────────────────
  for (int r = 0; r < B_ROWS; r++) {
    lcd.setCursor(B_OFFX, r);
    for (int c = 0; c < B_COLS; c++) {
      bool isActive = false;
      for (int i = 0; i < (int)pieceW; i++)
        if (r == pieceRow && c == pieceX + pieceCols[i]) { isActive = true; break; }
      if      (isActive)  lcd.write((byte)C_PIECE);
      else if (board[r][c]) lcd.write((byte)C_BLOCK);
      else                lcd.write(' ');
    }
  }

  // ── Left side: HUD (cols 0-9) ───────────────────────────
  // Row 0: score
  lcd.setCursor(0, 0); lcd.print("SC:");
  char buf[7]; sprintf(buf, "%5d", score);
  lcd.print(buf);

  // Row 1: level
  lcd.setCursor(0, 1); lcd.print("LV:"); lcd.print(level);
  lcd.print("        ");

  // Row 2: hi-score
  lcd.setCursor(0, 2); lcd.print("HI:");
  sprintf(buf, "%5d", hiScore);
  lcd.print(buf);

  // Row 3: next piece preview
  lcd.setCursor(0, 3); lcd.print("NX:");
  uint8_t nw = PIECES[nextType].w;
  for (int i = 0; i < 4; i++) {
    if (i < (int)nw) lcd.write((byte)C_PIECE);
    else             lcd.write(' ');
  }
  lcd.print("  ");
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
    lcd.setCursor(5, 0); lcd.print("TETRIS");
    lcd.setCursor(0, 1); lcd.print("X=move Y=rotate");
    lcd.setCursor(0, 2); lcd.print("Btn = hard drop");
    lcd.setCursor(0, 3); lcd.print("Click to START");
  }
  int8_t dx, dy; bool btn;
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
  lcd.createChar(C_BLOCK, BLOCK_CHAR);
  lcd.createChar(C_PIECE, PIECE_CHAR);
  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  analogReadResolution(12);
  randomSeed(analogRead(33));
  splash(false);
}

void loop() {
  initGame();
  lcd.clear();
  lastDrop = millis();

  int8_t  lastDX = 0, lastDY = 0;
  bool    lastBtn = false;
  unsigned long lastInput = 0;
  int     inputRepeat = 150;  // ms between held-input repeats

  while (true) {
    int8_t dx, dy; bool btn;
    readJoy(dx, dy, btn);

    bool inputReady = (millis() - lastInput >= inputRepeat);

    if (inputReady) {
      bool acted = false;

      // Move left / right
      if (dx == -1 && pieceValid(pieceX - 1, pieceRow)) { pieceX--; acted = true; }
      if (dx ==  1 && pieceValid(pieceX + 1, pieceRow)) { pieceX++; acted = true; }

      // Rotate (joystick UP) — cycle through piece variants by shifting cols
      if (dy == -1) {
        // Simple rotate: reverse col order
        for (int i = 0; i < (int)pieceW / 2; i++) {
          int8_t tmp = pieceCols[i];
          pieceCols[i] = pieceCols[pieceW - 1 - i];
          pieceCols[pieceW - 1 - i] = tmp;
        }
        if (!pieceValid(pieceX, pieceRow)) {
          // Undo rotate
          for (int i = 0; i < (int)pieceW / 2; i++) {
            int8_t tmp = pieceCols[i];
            pieceCols[i] = pieceCols[pieceW - 1 - i];
            pieceCols[pieceW - 1 - i] = tmp;
          }
        }
        acted = true;
      }

      // Soft drop (joystick DOWN)
      if (dy == 1) {
        if (pieceValid(pieceX, pieceRow + 1)) pieceRow++;
        acted = true;
      }

      if (acted) lastInput = millis();
    }

    // Hard drop (button)
    if (btn && !lastBtn) {
      while (pieceValid(pieceX, pieceRow + 1)) pieceRow++;
      lastDrop = 0;  // force lock immediately
    }
    lastBtn = btn;

    // Auto-drop
    if (millis() - lastDrop >= (unsigned long)dropInterval) {
      lastDrop = millis();
      if (pieceValid(pieceX, pieceRow + 1)) {
        pieceRow++;
      } else {
        lockPiece();
        int cleared = clearRows();
        if (cleared > 0) {
          linesCleared += cleared;
          score += cleared * 100 * level;
          if (linesCleared >= level * 3) {
            level++;
            dropInterval = max(150, dropInterval - 80);
          }
          if (score > hiScore) hiScore = score;
        }
        newPiece();
        // Spawn collision = game over
        if (!pieceValid(pieceX, pieceRow)) break;
      }
    }

    drawFrame();
    delay(20);
  }

  if (score > hiScore) hiScore = score;
  splash(true);
}

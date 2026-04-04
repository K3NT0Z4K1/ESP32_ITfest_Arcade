// ╔══════════════════════════════════════════════════════════════╗
// ║       IT FEST ARCADE  —  3 GAMES + FIREBASE                 ║
// ║  ESP32 · 4×20 I2C LCD · Joystick (no button needed)         ║
// ╠══════════════════════════════════════════════════════════════╣
// ║  GAMES                                                       ║
// ║   1. Dino Run     UP = jump  avoid obstacles                 ║
// ║   2. Snake        Joystick = steer                           ║
// ║   3. Maze Runner  Joystick = navigate  (60s timer!)          ║
// ╠══════════════════════════════════════════════════════════════╣
// ║  MENU   UP/DN = scroll   Hold RIGHT 1s = select              ║
// ╠══════════════════════════════════════════════════════════════╣
// ║  FIREBASE FLOW                                               ║
// ║   Dashboard writes /currentPlayer/name                       ║
// ║   ESP32 reads name → shows on LCD before game                ║
// ║   On game over → writes to /scores/<game>/<name>/score       ║
// ╠══════════════════════════════════════════════════════════════╣
// ║  WIRING                                                      ║
// ║   SDA→26  SCL→27  VRX→32  VRY→34  VCC→3.3V  GND→GND        ║
// ╚══════════════════════════════════════════════════════════════╝

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <FirebaseESP32.h>

// ── WiFi ──────────────────────────────────────────────────────
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASS     "YOUR_WIFI_PASSWORD"

// ── Firebase ──────────────────────────────────────────────────
#define FB_HOST  "dino-game-59371-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FB_AUTH  "IxtHeeV7AjKhWnQGZOebmwxu7I2Uf6ztakDbNtv7"

// ── Hardware ──────────────────────────────────────────────────
#define SDA_PIN   26
#define SCL_PIN   27
#define PIN_JOY_X 32
#define PIN_JOY_Y 34
#define JOY_LOW  1000
#define JOY_HIGH 3000
#define LCD_COLS  20
#define LCD_ROWS   4

LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);

FirebaseData   fbdo;
FirebaseAuth   fbAuth;
FirebaseConfig fbConfig;
bool fbReady = false;

String playerName = "Player";

// ── Hi-scores (RAM, reset on power off) ───────────────────────
unsigned int hiDino  = 0;
unsigned int hiSnake = 0;
unsigned int hiMaze  = 0;

// ╔══════════════════════════════════════════════════════════════╗
// ║  FIREBASE HELPERS                                            ║
// ╚══════════════════════════════════════════════════════════════╝

void connectWiFi() {
  lcdFill();
  lcd.setCursor(1, 1); lcd.print("Connecting WiFi..");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 24) {
    delay(500);
    lcd.setCursor(tries % LCD_COLS, 2); lcd.print(".");
    tries++;
  }
  lcdFill();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(3, 1); lcd.print("WiFi  Connected!");
    lcd.setCursor(1, 2); lcd.print(WiFi.localIP().toString());
  } else {
    lcd.setCursor(2, 1); lcd.print("WiFi Failed");
    lcd.setCursor(1, 2); lcd.print("Offline Mode");
  }
  delay(1600);
}

void initFirebase() {
  fbConfig.host                       = FB_HOST;
  fbConfig.signer.tokens.legacy_token = FB_AUTH;
  Firebase.begin(&fbConfig, &fbAuth);
  Firebase.reconnectWiFi(true);
  fbReady = true;
}

// Read /currentPlayer/name — returns "Player" on failure
String fbGetPlayerName() {
  if (!fbReady) return "Player";
  if (Firebase.getString(fbdo, "/currentPlayer/name")) {
    String n = fbdo.stringData();
    n.trim();
    return (n.length() >= 2) ? n : "Player";
  }
  return "Player";
}

// Post score — only overwrites if new score is higher
void fbPostScore(const char* game, String name, unsigned int score) {
  if (!fbReady) return;
  String path = "/scores/";
  path += game; path += "/"; path += name;
  unsigned int stored = 0;
  if (Firebase.getInt(fbdo, path + "/score"))
    stored = (unsigned int)fbdo.intData();
  if (score >= stored) {
    Firebase.setInt(fbdo,    path + "/score", (int)score);
    Firebase.setString(fbdo, path + "/name",  name);
    Firebase.setString(fbdo, path + "/game",  game);
  }
}

// Mark player as no longer active
void fbClearPlayer() {
  if (!fbReady) return;
  Firebase.setString(fbdo, "/currentPlayer/name", "");
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  SHARED UTILITIES                                            ║
// ╚══════════════════════════════════════════════════════════════╝

void joyRead(int8_t &dx, int8_t &dy) {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);
  dx = (x < JOY_LOW) ? -1 : (x > JOY_HIGH) ?  1 : 0;
  dy = (y < JOY_LOW) ? -1 : (y > JOY_HIGH) ?  1 : 0;
}

void lcdFill(char c = ' ') {
  for (int r = 0; r < LCD_ROWS; r++) {
    lcd.setCursor(0, r);
    for (int col = 0; col < LCD_COLS; col++) lcd.write(c);
  }
}

// Hold joystick RIGHT for holdMs → shows progress bar row 3
bool holdRight(int holdMs = 900) {
  const int barLen = 16;
  unsigned long start = 0;
  bool holding = false;
  while (true) {
    int8_t dx, dy; joyRead(dx, dy);
    if (dx == 1) {
      if (!holding) { holding = true; start = millis(); }
      int filled = constrain((int)map(millis()-start, 0, holdMs, 0, barLen), 0, barLen);
      lcd.setCursor(2, 3); lcd.print("[");
      for (int i = 0; i < barLen; i++) lcd.write(i < filled ? (byte)0xFF : ' ');
      lcd.print("]");
      if (millis()-start >= (unsigned long)holdMs) {
        lcd.setCursor(2, 3); lcd.print("                  ");
        return true;
      }
    } else {
      if (holding) { holding = false; lcd.setCursor(2, 3); lcd.print("                  "); }
    }
    delay(25);
  }
}

// Game over screen — posts score to Firebase then waits for player
void gameOverScreen(const char* game, unsigned int score, unsigned int &hi) {
  if (score > hi) hi = score;
  fbPostScore(game, playerName, score);
  fbClearPlayer();

  lcdFill();
  lcd.setCursor(2, 0); lcd.print("  ** GAME OVER **");
  lcd.setCursor(0, 1); lcd.print("  Score  : "); lcd.print(score);
  lcd.setCursor(0, 2); lcd.print("  Best   : "); lcd.print(hi);
  lcd.setCursor(0, 3); lcd.print("  Hold>>");
  while (!holdRight(900)) {}
}

// Welcome screen shown when player name arrives from dashboard
void showWelcome() {
  lcdFill();
  lcd.setCursor(2, 0); lcd.print("* IT FEST ARCADE *");
  lcd.setCursor(0, 1); lcd.print("Hello, ");
  lcd.print(playerName.substring(0, 13));
  lcd.setCursor(0, 2); lcd.print("Get ready!");
  lcd.setCursor(0, 3); lcd.print("Hold>> to start ");
  while (!holdRight(900)) {}
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  MAIN MENU                                                   ║
// ╚══════════════════════════════════════════════════════════════╝
#define NUM_GAMES 3
const char* GNAMES[NUM_GAMES] = {
  " 1. Dino  Run  ",
  " 2. Snake      ",
  " 3. Maze Runner"
};
int menuSel = 0;

void drawMenu() {
  lcdFill();
  lcd.setCursor(1, 0); lcd.print("* IT FEST ARCADE *");
  for (int s = 0; s < 3; s++) {
    int idx = (menuSel - 1 + s + NUM_GAMES) % NUM_GAMES;
    lcd.setCursor(0, s + 1);
    lcd.print(s == 1 ? ">" : " ");
    lcd.print(GNAMES[idx]);
    if (s == 1) lcd.print("<");
  }
}

// Polls Firebase every 2s while in menu waiting for a player name
int runMenu() {
  drawMenu();
  int8_t lastDY = 0;
  unsigned long lastScroll = 0;
  unsigned long lastFbPoll = 0;

  while (true) {
    int8_t dx, dy; joyRead(dx, dy);

    // Scroll
    if (dy != 0 && dy != lastDY && millis()-lastScroll > 250) {
      menuSel = (menuSel + dy + NUM_GAMES) % NUM_GAMES;
      drawMenu(); lastScroll = millis();
    }
    lastDY = dy;

    // Select with hold right
    if (dx == 1) {
      lcd.setCursor(0, 3); lcd.print(" Hold>> to launch");
      if (holdRight(900)) return menuSel;
      drawMenu();
    }

    // Poll Firebase for new player name every 2 s
    if (millis() - lastFbPoll > 2000) {
      lastFbPoll = millis();
      String n = fbGetPlayerName();
      if (n != "Player" && n != playerName) {
        playerName = n;
        // Flash player name on row 3 of menu
        lcd.setCursor(0, 3);
        lcd.print("Player: ");
        lcd.print(playerName.substring(0, 12));
      }
    }

    delay(60);
  }
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  GAME 1 — DINO RUN                                          ║
// ╚══════════════════════════════════════════════════════════════╝
#define DN_W      20
#define DN_COL     2
#define DN_EMPTY  ' '
#define DN_SOLID   5
#define DN_SLDR    6
#define DN_SLDL    7
#define DN_RUN1    1
#define DN_RUN2    2
#define DN_JUMP    3
#define DN_JUPUP  '.'
#define DN_JUPLO   4
#define HP_RUN_LO1  1
#define HP_RUN_LO2  2
#define HP_JUMP_1   3
#define HP_JUMP_2   4
#define HP_JUMP_3   5
#define HP_JUMP_4   6
#define HP_JUMP_5   7
#define HP_JUMP_6   8
#define HP_JUMP_7   9
#define HP_JUMP_8  10
#define HP_RUN_UP1 11
#define HP_RUN_UP2 12

static char dnTerU[DN_W+1], dnTerL[DN_W+1];

void loadDinoChars() {
  byte c1[]={B11110,B10111,B11110,B11100,B11111,B11100,B10010,B11011};
  byte c2[]={B11110,B10111,B11110,B11100,B11111,B11100,B01010,B10011};
  byte c3[]={B11110,B10111,B11110,B11100,B11111,B11110,B10001,B00000};
  byte c4[]={B11111,B11100,B10010,B11011,B00000,B00000,B00000,B00000};
  byte c5[]={B00100,B11111,B00100,B00101,B11111,B00100,B00100,B00100};
  byte c6[]={B00000,B00011,B00000,B00000,B00011,B00000,B00000,B00000};
  byte c7[]={B00000,B11000,B00000,B01000,B11000,B00000,B00000,B00000};
  lcd.createChar(1,c1); lcd.createChar(2,c2); lcd.createChar(3,c3);
  lcd.createChar(4,c4); lcd.createChar(5,c5); lcd.createChar(6,c6);
  lcd.createChar(7,c7);
}

void dnAdvance(char* ter, char newCell) {
  for (int i = 0; i < DN_W; i++) {
    char cur = ter[i];
    char nxt = (i == DN_W-1) ? newCell : ter[i+1];
    switch (cur) {
      case DN_EMPTY: ter[i]=(nxt==DN_SOLID)?DN_SLDR:DN_EMPTY; break;
      case DN_SOLID: ter[i]=(nxt==DN_EMPTY)?DN_SLDL:DN_SOLID; break;
      case DN_SLDR:  ter[i]=DN_SOLID; break;
      case DN_SLDL:  ter[i]=DN_EMPTY; break;
    }
  }
}

bool drawDinoHero(byte pos, unsigned int score) {
  char uSave=dnTerU[DN_COL], lSave=dnTerL[DN_COL];
  char upper=DN_EMPTY, lower=DN_EMPTY;
  switch (pos) {
    case HP_RUN_LO1: lower=DN_RUN1; break;
    case HP_RUN_LO2: lower=DN_RUN2; break;
    case HP_JUMP_1: case HP_JUMP_8: lower=DN_JUMP; break;
    case HP_JUMP_2: case HP_JUMP_7: upper=DN_JUPUP; lower=DN_JUPLO; break;
    case HP_JUMP_3: case HP_JUMP_4:
    case HP_JUMP_5: case HP_JUMP_6: upper=DN_JUMP; break;
    case HP_RUN_UP1: upper=DN_RUN1; break;
    case HP_RUN_UP2: upper=DN_RUN2; break;
    default: break;
  }
  bool collide=false;
  if (upper!=DN_EMPTY) { dnTerU[DN_COL]=upper; collide=(uSave!=DN_EMPTY); }
  if (lower!=DN_EMPTY) { dnTerL[DN_COL]=lower; collide|=(lSave!=DN_EMPTY); }
  lcd.setCursor(0,1); for(int c=0;c<DN_W;c++) lcd.write((byte)dnTerU[c]);
  lcd.setCursor(0,2); for(int c=0;c<DN_W;c++) lcd.write((byte)dnTerL[c]);
  char sb[21]; snprintf(sb,21,"DINO RUN     %6u",score);
  lcd.setCursor(0,0); lcd.print(sb);
  dnTerU[DN_COL]=uSave; dnTerL[DN_COL]=lSave;
  return collide;
}

void runDino() {
  loadDinoChars(); lcdFill();
  lcd.setCursor(3,0); lcd.print("** DINO RUN **");
  lcd.setCursor(0,1); lcd.print("  UP = jump");
  lcd.setCursor(0,2); lcd.print("  Avoid obstacles!");
  lcd.setCursor(0,3); lcd.print("  Hold>>");
  while (!holdRight(900)) {}

  for (int i=0;i<DN_W;i++) { dnTerU[i]=DN_EMPTY; dnTerL[i]=DN_EMPTY; }
  lcdFill();
  lcd.setCursor(0,3); for(int i=0;i<LCD_COLS;i++) lcd.write((byte)0xFF);

  byte heroPos=HP_RUN_LO1, dnType=0, dnDur=1;
  unsigned int dist=0;
  int gameDelay=110;
  int8_t lastDY=0;

  while (true) {
    char newLo=(dnType==1)?(char)DN_SOLID:(char)DN_EMPTY;
    char newUp=(dnType==2)?(char)DN_SOLID:(char)DN_EMPTY;
    dnAdvance(dnTerL,newLo); dnAdvance(dnTerU,newUp);
    if (--dnDur==0) {
      if (dnType==0) { dnType=(random(3)==0)?2:1; dnDur=2+random(10); }
      else           { dnType=0; dnDur=10+random(10); }
    }
    int8_t dx,dy; joyRead(dx,dy);
    if (dy==-1&&lastDY!=-1&&heroPos<=HP_RUN_LO2) heroPos=HP_JUMP_1;
    lastDY=dy;
    if (drawDinoHero(heroPos,dist>>3)) {
      int cr=(heroPos>=HP_RUN_UP1)?1:2;
      for(int f=0;f<6;f++){lcd.setCursor(DN_COL,cr);lcd.print(f%2==0?"X":" ");delay(130);}
      break;
    }
    if      (heroPos==HP_RUN_LO2||heroPos==HP_JUMP_8)                                   heroPos=HP_RUN_LO1;
    else if (heroPos>=HP_JUMP_3&&heroPos<=HP_JUMP_5&&dnTerL[DN_COL]!=DN_EMPTY)          heroPos=HP_RUN_UP1;
    else if (heroPos>=HP_RUN_UP1&&dnTerL[DN_COL]==DN_EMPTY)                             heroPos=HP_JUMP_5;
    else if (heroPos==HP_RUN_UP2)                                                        heroPos=HP_RUN_UP1;
    else                                                                                 heroPos++;
    dist++;
    if (dist%50==0&&gameDelay>55) gameDelay-=3;
    delay(gameDelay);
  }
  unsigned int fs=dist>>3;
  gameOverScreen("dino", fs, hiDino);
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  GAME 2 — SNAKE                                             ║
// ╚══════════════════════════════════════════════════════════════╝
#define SNK_COLS 20
#define SNK_ROWS  4
#define SNK_MAX  (SNK_COLS*SNK_ROWS)

struct Pt { int8_t x,y; };
Pt  snk[SNK_MAX]; int snkLen;
Pt  snkF; int8_t snkDX,snkDY,snkNDX,snkNDY;
unsigned int snkSc;

void loadSnakeChars() {
  byte sh[]={B00000,B01110,B11111,B11011,B11111,B01110,B00000,B00000};
  byte sb[]={B00000,B01110,B11111,B11111,B11111,B01110,B00000,B00000};
  byte sf[]={B00100,B01110,B11111,B11111,B01110,B00100,B00000,B00000};
  lcd.createChar(1,sh); lcd.createChar(2,sb); lcd.createChar(3,sf);
}

void snkPlace() {
  bool ok;
  do {
    ok=true; snkF={(int8_t)random(SNK_COLS),(int8_t)random(SNK_ROWS)};
    for(int i=0;i<snkLen;i++) if(snk[i].x==snkF.x&&snk[i].y==snkF.y){ok=false;break;}
  } while(!ok);
}

void snkInit() {
  snkLen=3; for(int i=0;i<snkLen;i++) snk[i]={(int8_t)(6-i),1};
  snkDX=1;snkDY=0;snkNDX=1;snkNDY=0;snkSc=0;snkPlace();
}

void snkDraw() {
  char g[SNK_ROWS][SNK_COLS];
  for(int r=0;r<SNK_ROWS;r++) for(int c=0;c<SNK_COLS;c++) g[r][c]=' ';
  g[snkF.y][snkF.x]=3;
  for(int i=1;i<snkLen;i++) if(snk[i].x>=0&&snk[i].x<SNK_COLS&&snk[i].y>=0&&snk[i].y<SNK_ROWS) g[snk[i].y][snk[i].x]=2;
  g[snk[0].y][snk[0].x]=1;
  for(int r=0;r<SNK_ROWS;r++){lcd.setCursor(0,r);for(int c=0;c<SNK_COLS;c++)lcd.write(g[r][c]);}
  lcd.setCursor(13,0); char b[8]; sprintf(b,"S:%4d",snkSc); lcd.print(b);
}

void runSnake() {
  loadSnakeChars(); lcdFill();
  lcd.setCursor(4,0); lcd.print("** SNAKE **");
  lcd.setCursor(0,1); lcd.print("Joystick = steer");
  lcd.setCursor(0,2); lcd.print("Eat food to grow");
  lcd.setCursor(0,3); lcd.print("Hold>>");
  while (!holdRight(900)) {}
  snkInit(); lcdFill();
  unsigned long lm=0; unsigned int sp=280;
  while(true){
    int8_t dx,dy; joyRead(dx,dy);
    if(dx!=0&&snkDX==0){snkNDX=dx;snkNDY=0;}
    if(dy!=0&&snkDY==0){snkNDX=0;snkNDY=dy;}
    if(millis()-lm>=sp){
      lm=millis();snkDX=snkNDX;snkDY=snkNDY;
      Pt nh={(int8_t)(snk[0].x+snkDX),(int8_t)(snk[0].y+snkDY)};
      if(nh.x<0||nh.x>=SNK_COLS||nh.y<0||nh.y>=SNK_ROWS) goto snkDead;
      for(int i=0;i<snkLen;i++) if(snk[i].x==nh.x&&snk[i].y==nh.y) goto snkDead;
      for(int i=snkLen;i>0;i--) snk[i]=snk[i-1]; snk[0]=nh;
      if(nh.x==snkF.x&&nh.y==snkF.y){snkLen++;snkSc++;snkPlace();if(sp>100)sp=max(100u,sp-15);}
      snkDraw();
    }
    delay(20);
  }
  snkDead:
  for(int f=0;f<4;f++){lcd.setCursor(snk[0].x,snk[0].y);lcd.print(f%2==0?"X":" ");delay(180);}
  gameOverScreen("snake", snkSc, hiSnake);
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  GAME 3 — MAZE RUNNER  (with 60-second countdown timer)     ║
// ╚══════════════════════════════════════════════════════════════╝
#define MZ_W 40
#define MZ_H  4
#define MZ_TIME_LIMIT 60  // seconds per maze level

uint8_t mz[MZ_H][MZ_W];
int8_t  mzPX,mzPY;
int     mzCam,mzLv,mzLives;
unsigned int mzSc;

void loadMazeChars() {
  byte mw[]={B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111};
  byte mp[]={B00100,B01110,B00100,B01110,B10101,B00100,B01010,B10001};
  byte me[]={B11111,B10001,B10101,B10101,B10101,B10001,B11111,B00000};
  byte mc[]={B01110,B11011,B10101,B10101,B11011,B01110,B00000,B00000};
  byte ml[]={B01010,B11111,B11111,B01110,B00100,B00000,B00000,B00000};
  lcd.createChar(1,mw);lcd.createChar(2,mp);lcd.createChar(3,me);
  lcd.createChar(4,mc);lcd.createChar(5,ml);
}

void mzGen() {
  memset(mz,0,sizeof(mz));
  for(int c=0;c<MZ_W;c++){mz[0][c]=1;mz[3][c]=1;}
  for(int c=3;c<MZ_W-2;c++){
    if(random(3)==0){
      int wr=1+random(2);mz[wr][c]=1;
      if(c>3&&mz[1][c-1]==1)mz[1][c]=0;
      if(c>3&&mz[2][c-1]==1)mz[2][c]=0;
    }
    if(mz[1][c]==0&&random(6)==0)mz[1][c]=2;
    if(mz[2][c]==0&&random(6)==0)mz[2][c]=2;
  }
  for(int r=1;r<=2;r++) for(int c=0;c<3;c++) mz[r][c]=0;
  mz[1][MZ_W-1]=3;mz[2][MZ_W-1]=3;
  mzPX=1;mzPY=1;mzCam=0;
}

// Draw maze + HUD with timer on row 3 right side
void mzDraw(int timeLeft) {
  for(int r=0;r<MZ_H;r++){
    lcd.setCursor(0,r);
    for(int c=0;c<LCD_COLS;c++){
      int mc=mzCam+c;
      if(mc<0||mc>=MZ_W){lcd.write((byte)1);continue;}
      if(mc==mzPX&&r==mzPY){lcd.write((byte)2);continue;}
      switch(mz[r][mc]){
        case 1:lcd.write((byte)1);break;
        case 2:lcd.write((byte)4);break;
        case 3:lcd.write((byte)3);break;
        default:lcd.write(' ');
      }
    }
  }
  // Score top-right row 0
  lcd.setCursor(12,0); char sb[9]; sprintf(sb,"%6d",mzSc); lcd.print(sb);

  // Lives row 3 left
  lcd.setCursor(17,3); for(int i=0;i<3;i++) lcd.write(i<mzLives?(byte)5:' ');

  // Timer row 3 — shows remaining seconds, pulses when < 10
  lcd.setCursor(0,3);
  char tb[8]; sprintf(tb,"T:%2d  ",timeLeft);
  lcd.print(tb);
}

void runMaze() {
  loadMazeChars(); lcdFill();
  lcd.setCursor(3,0); lcd.print("* MAZE RUNNER *");
  lcd.setCursor(0,1); lcd.print("Navigate to EXIT");
  lcd.setCursor(0,2); lcd.print("60s per level!  ");
  lcd.setCursor(0,3); lcd.print("Hold>>");
  while (!holdRight(900)) {}
  mzLives=3;mzSc=0;mzLv=1;mzGen();lcdFill();

  int8_t ldx=0,ldy=0;
  unsigned long lm=0;
  int md=175;

  // Timer per level
  unsigned long levelStart=millis();

  while(true){
    // ── Timer check ───────────────────────────────────────
    unsigned long elapsed=(millis()-levelStart)/1000;
    int timeLeft=(int)MZ_TIME_LIMIT-(int)elapsed;
    if(timeLeft<=0){
      // Time up — lose a life
      mzLives--;
      lcdFill();
      lcd.setCursor(4,1); lcd.print("TIME'S UP!");
      lcd.setCursor(1,2); lcd.print("Lives: ");lcd.print(mzLives);
      delay(1400);
      if(mzLives<=0){
        if(mzSc>hiMaze)hiMaze=mzSc;
        gameOverScreen("maze",mzSc,hiMaze);
        return;
      }
      // Restart same level from beginning
      mzGen();lcdFill();
      levelStart=millis();
      continue;
    }

    int8_t dx,dy;joyRead(dx,dy);
    bool can=false;
    if(dx!=0||dy!=0){
      if(dx!=ldx||dy!=ldy||millis()-lm>=(unsigned long)md)
        {can=true;ldx=dx;ldy=dy;lm=millis();}
    } else {ldx=0;ldy=0;}

    if(can){
      int8_t nx=constrain(mzPX+dx,0,MZ_W-1);
      int8_t ny=constrain(mzPY+dy,0,MZ_H-1);
      uint8_t cell=mz[ny][nx];
      if(cell==1){
        mzLives--;
        for(int f=0;f<3;f++){lcd.setCursor(mzPX-mzCam,mzPY);lcd.print(f%2==0?"!":" ");delay(110);}
        if(mzLives<=0){if(mzSc>hiMaze)hiMaze=mzSc;gameOverScreen("maze",mzSc,hiMaze);return;}
      } else if(cell==2){
        mz[ny][nx]=0;mzSc+=5*mzLv;mzPX=nx;mzPY=ny;
      } else if(cell==3){
        mzSc+=50*mzLv;if(mzSc>hiMaze)hiMaze=mzSc;
        lcdFill();
        lcd.setCursor(3,1);lcd.print("LEVEL CLEAR!");
        lcd.setCursor(2,2);lcd.print("Score: ");lcd.print(mzSc);
        lcd.setCursor(2,3);lcd.print("Time left: ");lcd.print(timeLeft);lcd.print("s");
        delay(1800);
        mzLv++;md=max(80,md-10);
        // Bonus score for time remaining
        mzSc+=timeLeft*mzLv;
        mzGen();lcdFill();
        levelStart=millis();  // reset timer for new level
      } else {
        mzPX=nx;mzPY=ny;
      }
      mzCam=constrain(mzPX-3,0,MZ_W-LCD_COLS);
    }
    mzDraw(timeLeft);
    delay(30);
  }
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  SETUP & LOOP                                               ║
// ╚══════════════════════════════════════════════════════════════╝
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN,SCL_PIN);
  lcd.begin(LCD_COLS,LCD_ROWS);
  lcd.backlight();
  analogReadResolution(12);
  randomSeed(analogRead(33));

  // Boot splash
  lcdFill();
  lcd.setCursor(1,1); lcd.print("  IT FEST ARCADE");
  lcd.setCursor(1,2); lcd.print("  3 Games Inside");
  delay(1200);

  connectWiFi();
  if (WiFi.status()==WL_CONNECTED) initFirebase();
}

void loop() {
  // Always try to fetch latest player name before showing menu
  String n = fbGetPlayerName();
  if (n != "Player" && n.length() >= 2) {
    playerName = n;
    showWelcome();
  }

  int choice = runMenu();

  switch (choice) {
    case 0: runDino();  break;
    case 1: runSnake(); break;
    case 2: runMaze();  break;
  }
  delay(300);
}

// ╔══════════════════════════════════════════════════════════════╗
// ║          IT FEST ARCADE  —  6 GAMES IN ONE                  ║
// ║  ESP32 · 4×20 I2C LCD · Joystick (no SW button needed)      ║
// ╠══════════════════════════════════════════════════════════════╣
// ║  GAMES                                                       ║
// ║   1. Dino Run     UP=jump  DOWN=duck                         ║
// ║   2. Snake        Joystick = steer                           ║
// ║   3. Maze Runner  Joystick = navigate                        ║
// ║   4. Flappy Bird  UP = flap  (gravity pulls down)            ║
// ║   5. Pac-Man      Joystick = move  eat all dots              ║
// ║   6. Tetris       X=move  UP=rotate  DOWN=soft drop          ║
// ╠══════════════════════════════════════════════════════════════╣
// ║  MENU CONTROLS  (no button needed anywhere)                  ║
// ║   UP / DOWN      scroll game list                            ║
// ║   Hold RIGHT 1s  select / confirm / restart                  ║
// ╠══════════════════════════════════════════════════════════════╣
// ║  WIRING                                                      ║
// ║   SDA → GPIO 26    SCL → GPIO 27                             ║
// ║   VRX → GPIO 32    VRY → GPIO 34                             ║
// ║   VCC → 3.3V       GND → GND                                 ║
// ╚══════════════════════════════════════════════════════════════╝

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ── Pins ─────────────────────────────────────────────────────
#define SDA_PIN   26
#define SCL_PIN   27
#define PIN_JOY_X 32
#define PIN_JOY_Y 34
#define JOY_LOW  1000
#define JOY_HIGH 3000
#define LCD_COLS  20
#define LCD_ROWS   4

LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);

// ── Shared hi-scores ─────────────────────────────────────────
unsigned int hiDino   = 0;
unsigned int hiSnake  = 0;
unsigned int hiMaze   = 0;
unsigned int hiFlappy = 0;
unsigned int hiPac    = 0;
unsigned int hiTetris = 0;

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

// Hold joystick RIGHT for holdMs — shows progress bar on row 3
// Returns true when fully held, false if released early
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
      if (millis() - start >= (unsigned long)holdMs) {
        lcd.setCursor(2, 3); lcd.print("                  ");
        return true;
      }
    } else {
      if (holding) {
        holding = false;
        lcd.setCursor(2, 3); lcd.print("                  ");
      }
    }
    delay(25);
  }
}

void gameOverScreen(unsigned int score, unsigned int hi) {
  lcdFill();
  lcd.setCursor(2, 0); lcd.print("  ** GAME OVER **");
  lcd.setCursor(0, 1); lcd.print("  Score  : "); lcd.print(score);
  lcd.setCursor(0, 2); lcd.print("  Best   : "); lcd.print(hi);
  lcd.setCursor(0, 3); lcd.print("  Hold>>");
  while (!holdRight(900)) {}
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  CUSTOM CHARACTER BANKS  (7 slots used, slot 0 = 0xFF full) ║
// ╚══════════════════════════════════════════════════════════════╝

// ── Dino ─────────────────────────────────────────────────────
byte D_RUN1[]  = {B01110,B01110,B11110,B01111,B11110,B11100,B01010,B01010};
byte D_RUN2[]  = {B01110,B01110,B11110,B01111,B11110,B11100,B10100,B01100};
byte D_JUMP[]  = {B01110,B01110,B11110,B01111,B11111,B11110,B01110,B00110};
byte D_DUCK[]  = {B00000,B00000,B01110,B11111,B11101,B11100,B01010,B01010};
byte D_CACTC[] = {B00100,B11111,B00100,B00101,B11111,B00100,B00100,B00100};
byte D_CACTR[] = {B00000,B00011,B00000,B00000,B00011,B00000,B00000,B00000};
byte D_CACTL[] = {B00000,B11000,B00000,B01000,B11000,B00000,B00000,B00000};
void loadDinoChars()  { lcd.createChar(1,D_RUN1); lcd.createChar(2,D_RUN2); lcd.createChar(3,D_JUMP); lcd.createChar(4,D_DUCK); lcd.createChar(5,D_CACTC); lcd.createChar(6,D_CACTR); lcd.createChar(7,D_CACTL); }

// ── Snake ────────────────────────────────────────────────────
byte S_HEAD[] = {B00000,B01110,B11111,B11011,B11111,B01110,B00000,B00000};
byte S_BODY[] = {B00000,B01110,B11111,B11111,B11111,B01110,B00000,B00000};
byte S_FOOD[] = {B00100,B01110,B11111,B11111,B01110,B00100,B00000,B00000};
void loadSnakeChars() { lcd.createChar(1,S_HEAD); lcd.createChar(2,S_BODY); lcd.createChar(3,S_FOOD); }

// ── Maze ─────────────────────────────────────────────────────
byte M_WALL[]   = {B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111};
byte M_PLAYER[] = {B00100,B01110,B00100,B01110,B10101,B00100,B01010,B10001};
byte M_EXIT[]   = {B11111,B10001,B10101,B10101,B10101,B10001,B11111,B00000};
byte M_COIN[]   = {B01110,B11011,B10101,B10101,B11011,B01110,B00000,B00000};
byte M_LIFE[]   = {B01010,B11111,B11111,B01110,B00100,B00000,B00000,B00000};
void loadMazeChars()  { lcd.createChar(1,M_WALL); lcd.createChar(2,M_PLAYER); lcd.createChar(3,M_EXIT); lcd.createChar(4,M_COIN); lcd.createChar(5,M_LIFE); }

// ── Flappy ───────────────────────────────────────────────────
byte F_BIRD1[] = {B00110,B01111,B11110,B11100,B11111,B01110,B00100,B00000};
byte F_BIRD2[] = {B00110,B01111,B11110,B11100,B11111,B01100,B00110,B00000};
byte F_PIPE[]  = {B11111,B11111,B10001,B10001,B10001,B10001,B11111,B11111};
byte F_PIPET[] = {B11111,B11111,B11111,B10001,B10001,B10001,B10001,B10001};
void loadFlappyChars(){ lcd.createChar(1,F_BIRD1); lcd.createChar(2,F_BIRD2); lcd.createChar(3,F_PIPE); lcd.createChar(4,F_PIPET); }

// ── Pac-Man ──────────────────────────────────────────────────
byte P_PAC1[]  = {B01110,B11111,B11110,B11100,B11100,B11110,B11111,B01110};
byte P_PAC2[]  = {B01110,B11111,B11111,B11111,B11111,B11111,B11111,B01110};
byte P_GHOST[] = {B01110,B11111,B10101,B11111,B11111,B11111,B10101,B00000};
byte P_DOT[]   = {B00000,B00000,B00000,B01100,B01100,B00000,B00000,B00000};
byte P_WALL[]  = {B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111};
void loadPacChars()   { lcd.createChar(1,P_PAC1); lcd.createChar(2,P_PAC2); lcd.createChar(3,P_GHOST); lcd.createChar(4,P_DOT); lcd.createChar(5,P_WALL); }

// ── Tetris ───────────────────────────────────────────────────
byte T_BLOCK[] = {B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111};
byte T_PIECE[] = {B11111,B10001,B10001,B10001,B10001,B10001,B10001,B11111};
void loadTetrisChars(){ lcd.createChar(1,T_BLOCK); lcd.createChar(2,T_PIECE); }

// ╔══════════════════════════════════════════════════════════════╗
// ║  MAIN MENU                                                   ║
// ╚══════════════════════════════════════════════════════════════╝
#define NUM_GAMES 6
const char* GNAMES[NUM_GAMES] = {
  " 1. Dino  Run  ",
  " 2. Snake      ",
  " 3. Maze Runner",
  " 4. Flappy Bird",
  " 5. Pac - Man  ",
  " 6. Tetris     "
};
const char* GHINTS[NUM_GAMES] = {
  "UP=jump DN=duck ",
  "Stick = steer   ",
  "Reach the exit! ",
  "UP=flap gravity ",
  "Eat all the dots",
  "X=mv UP=rot DN=dn"
};

int menuSel = 0;

void drawMenu() {
  lcdFill();
  lcd.setCursor(2, 0); lcd.print("* IT FEST ARCADE *");
  // Show 3 items: sel-1, sel, sel+1 on rows 1-3
  for (int slot = 0; slot < 3; slot++) {
    int idx = (menuSel - 1 + slot + NUM_GAMES) % NUM_GAMES;
    lcd.setCursor(0, slot + 1);
    lcd.print(slot == 1 ? ">" : " ");
    lcd.print(GNAMES[idx]);
    if (slot == 1) lcd.print("<");
  }
}

int runMenu() {
  drawMenu();
  int8_t lastDY = 0;
  unsigned long lastScroll = 0;
  while (true) {
    int8_t dx, dy; joyRead(dx, dy);
    // Scroll
    if (dy != 0 && dy != lastDY && millis()-lastScroll > 250) {
      menuSel = (menuSel + dy + NUM_GAMES) % NUM_GAMES;
      drawMenu();
      lastScroll = millis();
    }
    lastDY = dy;
    // Select with hold right
    if (dx == 1) {
      lcd.setCursor(0, 3); lcd.print(" Hold>> to launch");
      if (holdRight(900)) return menuSel;
      drawMenu();
    }
    delay(60);
  }
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  GAME 1 — DINO RUN                                          ║
// ╚══════════════════════════════════════════════════════════════╝
#define DINO_COL 2
#define DINO_JUMP_PHASES 10
const int8_t DINO_JROW[DINO_JUMP_PHASES] = {2,2,1,1,0,0,0,1,1,2};

static char dTerU[LCD_COLS+1], dTerL[LCD_COLS+1];
struct DinoObs { int col; bool active; };
static DinoObs dObs;

void dScroll() {
  for (int c=0;c<LCD_COLS-1;c++){dTerU[c]=dTerU[c+1]; dTerL[c]=dTerL[c+1];}
  dTerU[LCD_COLS-1]=' '; dTerL[LCD_COLS-1]=' ';
  dTerU[LCD_COLS]='\0';  dTerL[LCD_COLS]='\0';
}

void dRender(int hrow, byte spr, unsigned int sc) {
  char u[LCD_COLS+1], l[LCD_COLS+1];
  memcpy(u,dTerU,LCD_COLS+1); memcpy(l,dTerL,LCD_COLS+1);

  // Score top-right
  char sb[7]; sprintf(sb,"%5d",sc);
  for(int i=0;i<5;i++) u[15+i]=sb[i];

  for(int r=0;r<LCD_ROWS;r++){
    lcd.setCursor(0,r);
    for(int c=0;c<LCD_COLS;c++){
      if(r==hrow && c==DINO_COL) { lcd.write(spr); continue; }
      if(r==0) lcd.write(u[c]);
      else if(r==2) lcd.write(l[c]);
      else lcd.write(' ');
    }
  }
  lcd.setCursor(15,0); lcd.print(sb);
}

bool dCollide(int hrow){
  if(hrow==2){ char cell=dTerL[DINO_COL]; return(cell!=' '); }
  return false;
}

void runDino() {
  loadDinoChars(); lcdFill();
  lcd.setCursor(3,0); lcd.print("** DINO RUN **");
  lcd.setCursor(0,1); lcd.print("UP  = jump");
  lcd.setCursor(0,2); lcd.print("DOWN= duck");
  lcd.setCursor(0,3); lcd.print("Hold>>");
  while(!holdRight(900)){}
  lcdFill();

  for(int c=0;c<LCD_COLS;c++){dTerU[c]=' ';dTerL[c]=' ';}
  dTerU[LCD_COLS]=dTerL[LCD_COLS]='\0';
  dObs.active=false;

  int8_t jp=-1, hrow=2; bool duck=false;
  byte spr=1; bool rf=false;
  unsigned int dist=0;
  unsigned int nextObs=18+random(18);
  unsigned long lastF=0;
  int8_t lastDY=0;

  while(true){
    if(millis()-lastF<85){delay(10);continue;}
    lastF=millis();
    int8_t dx,dy; joyRead(dx,dy);

    if(dy==-1 && lastDY!=-1 && jp<0 && !duck && hrow==2) jp=0;
    if(dy==1 && jp<0){duck=true;hrow=3;}
    else if(dy!=1 && jp<0){duck=false;hrow=2;}
    if(jp>=0){hrow=DINO_JROW[jp];duck=false;jp++;if(jp>=DINO_JUMP_PHASES)jp=-1;}
    lastDY=dy;

    rf=!rf;
    if(jp>=0) spr=3;
    else if(duck) spr=4;
    else spr=rf?1:2;

    dScroll();
    if(!dObs.active){
      if(dist>=nextObs){dObs.col=LCD_COLS-1;dObs.active=true;nextObs=dist+16+random(20);}
    } else {
      dObs.col--;
      if(dObs.col<0){dObs.active=false;}
      else{
        if(dObs.col-1>=0) dTerL[dObs.col-1]=7;
        if(dObs.col>=0&&dObs.col<LCD_COLS) dTerL[dObs.col]=5;
        if(dObs.col+1<LCD_COLS) dTerL[dObs.col+1]=6;
      }
    }
    if(dCollide(hrow)){
      for(int f=0;f<4;f++){lcd.setCursor(DINO_COL,hrow);lcd.print(f%2==0?"X":" ");delay(150);}
      break;
    }
    unsigned int sc=dist>>3;
    dRender(hrow,spr,sc); dist++;
  }
  unsigned int fs=dist>>3;
  if(fs>hiDino) hiDino=fs;
  gameOverScreen(fs,hiDino);
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  GAME 2 — SNAKE                                             ║
// ╚══════════════════════════════════════════════════════════════╝
#define SNK_COLS 20
#define SNK_ROWS  4
#define SNK_MAX  (SNK_COLS*SNK_ROWS)
struct Pt{int8_t x,y;};
Pt snk[SNK_MAX]; int snkLen;
Pt snkF; int8_t snkDX,snkDY,snkNDX,snkNDY;
unsigned int snkSc;

void snkPlace(){bool ok;do{ok=true;snkF={(int8_t)random(SNK_COLS),(int8_t)random(SNK_ROWS)};for(int i=0;i<snkLen;i++)if(snk[i].x==snkF.x&&snk[i].y==snkF.y){ok=false;break;}}while(!ok);}
void snkInit(){snkLen=3;for(int i=0;i<snkLen;i++)snk[i]={(int8_t)(6-i),1};snkDX=1;snkDY=0;snkNDX=1;snkNDY=0;snkSc=0;snkPlace();}
void snkDraw(){
  char g[SNK_ROWS][SNK_COLS];
  for(int r=0;r<SNK_ROWS;r++) for(int c=0;c<SNK_COLS;c++) g[r][c]=' ';
  g[snkF.y][snkF.x]=3;
  for(int i=1;i<snkLen;i++) if(snk[i].x>=0&&snk[i].x<SNK_COLS&&snk[i].y>=0&&snk[i].y<SNK_ROWS) g[snk[i].y][snk[i].x]=2;
  g[snk[0].y][snk[0].x]=1;
  for(int r=0;r<SNK_ROWS;r++){lcd.setCursor(0,r);for(int c=0;c<SNK_COLS;c++)lcd.write(g[r][c]);}
  lcd.setCursor(13,0); char b[8]; sprintf(b,"S:%4d",snkSc); lcd.print(b);
}

void runSnake(){
  loadSnakeChars(); lcdFill();
  lcd.setCursor(4,0); lcd.print("** SNAKE **");
  lcd.setCursor(0,1); lcd.print("Joystick = steer");
  lcd.setCursor(0,2); lcd.print("Eat food to grow");
  lcd.setCursor(0,3); lcd.print("Hold>>");
  while(!holdRight(900)){}
  snkInit(); lcdFill();
  unsigned long lm=0; unsigned int sp=280;
  while(true){
    int8_t dx,dy; joyRead(dx,dy);
    if(dx!=0&&snkDX==0){snkNDX=dx;snkNDY=0;}
    if(dy!=0&&snkDY==0){snkNDX=0;snkNDY=dy;}
    if(millis()-lm>=sp){
      lm=millis(); snkDX=snkNDX; snkDY=snkNDY;
      Pt nh={(int8_t)(snk[0].x+snkDX),(int8_t)(snk[0].y+snkDY)};
      if(nh.x<0||nh.x>=SNK_COLS||nh.y<0||nh.y>=SNK_ROWS) goto snkDead;
      for(int i=0;i<snkLen;i++) if(snk[i].x==nh.x&&snk[i].y==nh.y) goto snkDead;
      for(int i=snkLen;i>0;i--) snk[i]=snk[i-1]; snk[0]=nh;
      if(nh.x==snkF.x&&nh.y==snkF.y){snkLen++;snkSc++;snkPlace();if(sp>120)sp=max(120u,sp-12);}
      snkDraw();
    }
    delay(20);
  }
  snkDead:
  if(snkSc>hiSnake) hiSnake=snkSc;
  for(int f=0;f<4;f++){lcd.setCursor(snk[0].x,snk[0].y);lcd.print(f%2==0?"X":" ");delay(180);}
  gameOverScreen(snkSc,hiSnake);
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  GAME 3 — MAZE RUNNER                                       ║
// ╚══════════════════════════════════════════════════════════════╝
#define MZ_W 40
#define MZ_H  4
uint8_t mz[MZ_H][MZ_W];
int8_t mzPX,mzPY; int mzCam,mzLv,mzLives; unsigned int mzSc;

void mzGen(){
  memset(mz,0,sizeof(mz));
  for(int c=0;c<MZ_W;c++){mz[0][c]=1;mz[3][c]=1;}
  for(int c=3;c<MZ_W-2;c++){
    if(random(3)==0){
      int wr=1+random(2); mz[wr][c]=1;
      if(c>3&&mz[1][c-1]==1) mz[1][c]=0;
      if(c>3&&mz[2][c-1]==1) mz[2][c]=0;
    }
    if(mz[1][c]==0&&random(6)==0) mz[1][c]=2;
    if(mz[2][c]==0&&random(6)==0) mz[2][c]=2;
  }
  for(int r=1;r<=2;r++) for(int c=0;c<3;c++) mz[r][c]=0;
  mz[1][MZ_W-1]=3; mz[2][MZ_W-1]=3;
  mzPX=1;mzPY=1;mzCam=0;
}
void mzDraw(){
  for(int r=0;r<MZ_H;r++){
    lcd.setCursor(0,r);
    for(int c=0;c<LCD_COLS;c++){
      int mc=mzCam+c;
      if(mc<0||mc>=MZ_W){lcd.write((byte)1);continue;}
      if(mc==mzPX&&r==mzPY){lcd.write((byte)2);continue;}
      switch(mz[r][mc]){case 1:lcd.write((byte)1);break;case 2:lcd.write((byte)4);break;case 3:lcd.write((byte)3);break;default:lcd.write(' ');}
    }
  }
  lcd.setCursor(13,0); char b[8]; sprintf(b,"%5d",mzSc); lcd.print(b);
  lcd.setCursor(17,3); for(int i=0;i<3;i++) lcd.write(i<mzLives?(byte)5:' ');
}

void runMaze(){
  loadMazeChars(); lcdFill();
  lcd.setCursor(3,0); lcd.print("* MAZE RUNNER *");
  lcd.setCursor(0,1); lcd.print("Navigate to EXIT");
  lcd.setCursor(0,2); lcd.print("Collect coins!  ");
  lcd.setCursor(0,3); lcd.print("Hold>>");
  while(!holdRight(900)){}
  mzLives=3;mzSc=0;mzLv=1; mzGen(); lcdFill();
  int8_t ldx=0,ldy=0; unsigned long lm=0; int md=175;
  while(true){
    int8_t dx,dy; joyRead(dx,dy);
    bool can=false;
    if(dx!=0||dy!=0){if(dx!=ldx||dy!=ldy||millis()-lm>=(unsigned long)md){can=true;ldx=dx;ldy=dy;lm=millis();}}
    else{ldx=0;ldy=0;}
    if(can){
      int8_t nx=constrain(mzPX+dx,0,MZ_W-1),ny=constrain(mzPY+dy,0,MZ_H-1);
      uint8_t cell=mz[ny][nx];
      if(cell==1){
        mzLives--;
        for(int f=0;f<3;f++){lcd.setCursor(mzPX-mzCam,mzPY);lcd.print(f%2==0?"!":" ");delay(110);}
        if(mzLives<=0){if(mzSc>hiMaze)hiMaze=mzSc;gameOverScreen(mzSc,hiMaze);return;}
      } else if(cell==2){mz[ny][nx]=0;mzSc+=5*mzLv;mzPX=nx;mzPY=ny;}
      else if(cell==3){
        mzSc+=50*mzLv;if(mzSc>hiMaze)hiMaze=mzSc;
        lcdFill();lcd.setCursor(3,1);lcd.print("LEVEL CLEAR!");lcd.setCursor(3,2);lcd.print("Level ");lcd.print(mzLv+1);delay(1800);
        mzLv++;md=max(80,md-10);mzGen();lcdFill();
      } else {mzPX=nx;mzPY=ny;}
      mzCam=constrain(mzPX-3,0,MZ_W-LCD_COLS);
    }
    mzDraw(); delay(30);
  }
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  GAME 4 — FLAPPY BIRD                                       ║
// ╚══════════════════════════════════════════════════════════════╝
// Bird has a floating-point Y position for smooth gravity.
// Pipes scroll in from the right with a random gap row.
// Gap is 2 rows tall so the player can pass through.
#define FLAP_BIRD_COL 3
#define FLAP_PIPE_GAP 2       // gap height in rows
#define FLAP_PIPE_SPACING 10  // cols between pipes

struct FlPipe {
  int col;
  int gapTop; // top row of the gap (0-2, gap occupies gapTop & gapTop+1)
  bool scored;
};

#define MAX_PIPES 3
FlPipe flPipes[MAX_PIPES];
float  flBirdY;    // 0.0 – 3.0 (row as float)
float  flVel;      // rows per frame (positive = down)
bool   flAlive;
unsigned int flSc;

void flInitPipes(){
  for(int i=0;i<MAX_PIPES;i++){
    flPipes[i].col    = LCD_COLS + i * FLAP_PIPE_SPACING;
    flPipes[i].gapTop = 1;   // start gap in middle
    flPipes[i].scored = false;
  }
}

void flRender(){
  char g[LCD_ROWS][LCD_COLS];
  for(int r=0;r<LCD_ROWS;r++) for(int c=0;c<LCD_COLS;c++) g[r][c]=' ';

  // Pipes
  for(int p=0;p<MAX_PIPES;p++){
    int pc=flPipes[p].col;
    if(pc<0||pc>=LCD_COLS) continue;
    int gt=flPipes[p].gapTop;
    for(int r=0;r<LCD_ROWS;r++){
      if(r==gt||r==gt+1) continue; // gap rows are open
      g[r][pc]= (r==gt-1||r==0) ? 4 : 3; // top cap or body
    }
  }

  // Bird — integer row
  int br=constrain((int)flBirdY,0,LCD_ROWS-1);
  static bool bframe=false; bframe=!bframe;
  g[br][FLAP_BIRD_COL]= bframe ? 1 : 2;

  for(int r=0;r<LCD_ROWS;r++){lcd.setCursor(0,r);for(int c=0;c<LCD_COLS;c++)lcd.write(g[r][c]);}

  // Score
  lcd.setCursor(14,0); char b[7]; sprintf(b,"F:%3d",flSc); lcd.print(b);
}

bool flCheckCollision(){
  int br=constrain((int)flBirdY,0,LCD_ROWS-1);
  for(int p=0;p<MAX_PIPES;p++){
    if(flPipes[p].col!=FLAP_BIRD_COL) continue;
    int gt=flPipes[p].gapTop;
    if(br!=gt && br!=gt+1) return true; // not in gap = hit
  }
  // Out of bounds
  if(flBirdY<0||flBirdY>=LCD_ROWS) return true;
  return false;
}

void runFlappy(){
  loadFlappyChars(); lcdFill();
  lcd.setCursor(2,0); lcd.print("** FLAPPY BIRD **");
  lcd.setCursor(0,1); lcd.print("UP = flap");
  lcd.setCursor(0,2); lcd.print("Avoid the pipes!");
  lcd.setCursor(0,3); lcd.print("Hold>>");
  while(!holdRight(900)){}
  lcdFill();

  flBirdY=1.5f; flVel=0.0f; flSc=0;
  flInitPipes();

  const float GRAVITY   = 0.32f;
  const float FLAP_STR  = -0.70f;
  const float MAX_VEL   =  0.90f;
  unsigned long lastF=0;
  int8_t lastDY=0;

  while(true){
    if(millis()-lastF<110){delay(10);continue;}
    lastF=millis();

    int8_t dx,dy; joyRead(dx,dy);

    // Flap on fresh UP
    if(dy==-1&&lastDY!=-1) flVel=FLAP_STR;
    lastDY=dy;

    // Gravity
    flVel+=GRAVITY;
    if(flVel>MAX_VEL) flVel=MAX_VEL;
    flBirdY+=flVel;

    // Scroll pipes
    for(int p=0;p<MAX_PIPES;p++){
      flPipes[p].col--;
      if(flPipes[p].col<-1){
        // Recycle pipe at right side
        // Find rightmost pipe
        int rightmost=0;
        for(int q=0;q<MAX_PIPES;q++) if(flPipes[q].col>rightmost) rightmost=flPipes[q].col;
        flPipes[p].col    = rightmost + FLAP_PIPE_SPACING;
        flPipes[p].gapTop = 1+random(2); // gap at row 1 or 2
        flPipes[p].scored = false;
      }
      // Score when bird passes pipe
      if(!flPipes[p].scored && flPipes[p].col < FLAP_BIRD_COL){
        flPipes[p].scored=true; flSc++;
      }
    }

    if(flCheckCollision()){
      // Flash bird
      for(int f=0;f<4;f++){
        int br=constrain((int)flBirdY,0,LCD_ROWS-1);
        lcd.setCursor(FLAP_BIRD_COL,br); lcd.print(f%2==0?"*":" "); delay(150);
      }
      break;
    }
    flRender();
  }
  if(flSc>hiFlappy) hiFlappy=flSc;
  gameOverScreen(flSc,hiFlappy);
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  GAME 5 — PAC-MAN (simplified)                              ║
// ╚══════════════════════════════════════════════════════════════╝
// Map: 20 cols × 4 rows. '#'=wall ' '=open '.'=dot 'o'=power
// Ghost chases Pac with simple pathfinding.
// Eating all dots = level clear.

#define PM_COLS 20
#define PM_ROWS  4

// Map template (20 chars per row × 4 rows)
// '#' wall  '.' dot  ' ' open  'o' power pellet
const char PM_MAP_TMPL[PM_ROWS][PM_COLS+1] = {
  "####################",
  "#.o..#....#....o..##",  // note: trailing # fills to 20
  "#...##....#.....#..#",
  "####################",
};

char pmMap[PM_ROWS][PM_COLS];
int8_t pmPX,pmPY,pmPDX,pmPDY,pmNDX,pmNDY;
int8_t pmGX,pmGY,pmGDX,pmGDY;
unsigned int pmSc, pmDots;
bool pmFright; unsigned long pmFrightEnd;
int pmLv;

void pmLoad(){
  pmDots=0;
  for(int r=0;r<PM_ROWS;r++){
    for(int c=0;c<PM_COLS;c++){
      pmMap[r][c]=PM_MAP_TMPL[r][c];
      if(pmMap[r][c]=='.') pmDots++;
    }
  }
  // Pac start
  pmPX=1;pmPY=1;pmPDX=1;pmPDY=0;pmNDX=1;pmNDY=0;
  // Ghost start
  pmGX=17;pmGY=2;pmGDX=-1;pmGDY=0;
  pmFright=false;
}

void pmDraw(){
  for(int r=0;r<PM_ROWS;r++){
    lcd.setCursor(0,r);
    for(int c=0;c<PM_COLS;c++){
      if(r==pmPY&&c==pmPX){ static bool pf=false;pf=!pf; lcd.write(pf?(byte)1:(byte)2); continue;}
      if(r==pmGY&&c==pmGX){ lcd.write(pmFright?'?':(byte)3); continue;}
      switch(pmMap[r][c]){
        case '#': lcd.write((byte)5); break;
        case '.': lcd.write((byte)4); break;
        case 'o': lcd.write('*');     break;
        default:  lcd.write(' ');     break;
      }
    }
  }
  lcd.setCursor(13,0); char b[8]; sprintf(b,"P:%4d",pmSc); lcd.print(b);
}

// Simple ghost AI: try to move toward Pac each step
void pmMoveGhost(){
  int8_t bestDX=pmGDX,bestDY=pmGDY;
  int bestDist=9999;
  int8_t dirs[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
  for(int d=0;d<4;d++){
    int8_t ndx=dirs[d][0],ndy=dirs[d][1];
    // No 180 reversal unless stuck
    if(ndx==-pmGDX&&ndy==-pmGDY) continue;
    int8_t nx=pmGX+ndx,ny=pmGY+ndy;
    if(nx<0||nx>=PM_COLS||ny<0||ny>=PM_ROWS) continue;
    if(pmMap[ny][nx]=='#') continue;
    int dist;
    if(pmFright){
      // Run away from Pac
      dist=-(abs(nx-pmPX)+abs(ny-pmPY));
    } else {
      dist=abs(nx-pmPX)+abs(ny-pmPY);
    }
    if(dist<bestDist){bestDist=dist;bestDX=ndx;bestDY=ndy;}
  }
  int8_t nx=pmGX+bestDX,ny=pmGY+bestDY;
  if(nx>=0&&nx<PM_COLS&&ny>=0&&ny<PM_ROWS&&pmMap[ny][nx]!='#'){
    pmGX=nx;pmGY=ny;pmGDX=bestDX;pmGDY=bestDY;
  }
}

void runPac(){
  loadPacChars(); lcdFill();
  lcd.setCursor(3,0); lcd.print("** PAC - MAN **");
  lcd.setCursor(0,1); lcd.print("Eat all the dots");
  lcd.setCursor(0,2); lcd.print("*=power pellet! ");
  lcd.setCursor(0,3); lcd.print("Hold>>");
  while(!holdRight(900)){}
  pmSc=0;pmLv=1;pmLoad();lcdFill();

  unsigned long lastPac=0,lastGhost=0;
  int pacSpd=200, ghostSpd=400;

  while(true){
    int8_t dx,dy; joyRead(dx,dy);
    // Buffer turn
    if(dx!=0) {pmNDX=dx;pmNDY=0;}
    if(dy!=0) {pmNDX=0;pmNDY=dy;}

    // Move Pac
    if(millis()-lastPac>=( unsigned long)pacSpd){
      lastPac=millis();
      // Try buffered dir first, else keep current
      int8_t tx=pmPX+pmNDX,ty=pmPY+pmNDY;
      if(tx>=0&&tx<PM_COLS&&ty>=0&&ty<PM_ROWS&&pmMap[ty][tx]!='#'){
        pmPDX=pmNDX;pmPDY=pmNDY;
      }
      int8_t nx=pmPX+pmPDX,ny=pmPY+pmPDY;
      if(nx>=0&&nx<PM_COLS&&ny>=0&&ny<PM_ROWS&&pmMap[ny][nx]!='#'){
        pmPX=nx;pmPY=ny;
        if(pmMap[ny][nx]=='.'){pmMap[ny][nx]=' ';pmSc+=10;pmDots--;}
        else if(pmMap[ny][nx]=='o'){pmMap[ny][nx]=' ';pmSc+=50;pmFright=true;pmFrightEnd=millis()+5000;}
      }
    }

    // Fright timeout
    if(pmFright&&millis()>pmFrightEnd) pmFright=false;

    // Move ghost
    if(millis()-lastGhost>=(unsigned long)ghostSpd){
      lastGhost=millis();
      pmMoveGhost();
    }

    // Ghost catch Pac
    if(pmGX==pmPX&&pmGY==pmPY){
      if(pmFright){
        // Eat ghost!
        pmSc+=200;pmGX=17;pmGY=2;pmGDX=-1;pmGDY=0;pmFright=false;
      } else {
        // Pac dies
        for(int f=0;f<4;f++){lcd.setCursor(pmPX,pmPY);lcd.print(f%2==0?"X":" ");delay(180);}
        break;
      }
    }

    // All dots eaten
    if(pmDots==0){
      pmSc+=100*pmLv;
      lcdFill();lcd.setCursor(3,1);lcd.print("LEVEL CLEAR!");
      lcd.setCursor(2,2);lcd.print("Score: ");lcd.print(pmSc);
      delay(1800);pmLv++;
      ghostSpd=max(180,(int)(ghostSpd-30));
      pmLoad();lcdFill();
    }

    pmDraw(); delay(25);
  }
  if(pmSc>hiPac) hiPac=pmSc;
  gameOverScreen(pmSc,hiPac);
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  GAME 6 — TETRIS                                            ║
// ╚══════════════════════════════════════════════════════════════╝
// Board: right 10 cols (10-19), HUD on left 10 cols (0-9)
// Pieces are 1 row tall — they fall row by row across 4 rows.
#define TB_COLS 10
#define TB_ROWS  4
#define TB_OFX  10

uint8_t tb[TB_ROWS][TB_COLS];

struct TPDef{uint8_t w;int8_t cols[4];};
const TPDef TPIECES[]={
  {4,{0,1,2,3}}, // I
  {3,{0,1,2,0}}, // L
  {3,{0,1,2,2}}, // J
  {3,{0,1,1,2}}, // S
  {3,{1,0,1,2}}, // Z
  {3,{0,1,2,1}}, // T
  {2,{0,1,0,1}}, // O
};
#define TP_NUM 7

int8_t tpType,tpX,tpRow,tpNext; uint8_t tpW; int8_t tpCols[4];
unsigned int tSc,tHi=0; int tLv,tLines; unsigned long tLastDrop; int tDropMs;

bool tpValid(int8_t px,int8_t pr){
  for(int i=0;i<(int)tpW;i++){int c=px+tpCols[i];if(c<0||c>=TB_COLS||pr<0||pr>=TB_ROWS||tb[pr][c])return false;}
  return true;
}
void tpLock(){for(int i=0;i<(int)tpW;i++)tb[tpRow][tpX+tpCols[i]]=1;}
int tClear(){
  int cl=0;
  for(int r=TB_ROWS-1;r>=0;r--){
    bool full=true;for(int c=0;c<TB_COLS;c++)if(!tb[r][c]){full=false;break;}
    if(full){for(int rr=r;rr>0;rr--)memcpy(tb[rr],tb[rr-1],TB_COLS);memset(tb[0],0,TB_COLS);cl++;r++;}
  }
  return cl;
}
void tSpawn(int8_t type){
  tpType=type;tpW=TPIECES[type].w;memcpy(tpCols,TPIECES[type].cols,4);
  tpX=(TB_COLS/2)-(tpW/2);tpRow=0;
}
void tNew(){tSpawn(tpNext);tpNext=random(TP_NUM);}

void tDraw(){
  for(int r=0;r<TB_ROWS;r++){
    lcd.setCursor(TB_OFX,r);
    for(int c=0;c<TB_COLS;c++){
      bool act=false;
      for(int i=0;i<(int)tpW;i++) if(r==tpRow&&c==tpX+tpCols[i]){act=true;break;}
      if(act) lcd.write((byte)2);
      else if(tb[r][c]) lcd.write((byte)1);
      else lcd.write(' ');
    }
  }
  // HUD
  lcd.setCursor(0,0);char b[8];sprintf(b,"SC:%5d",tSc);lcd.print(b);
  lcd.setCursor(0,1);lcd.print("LV:");lcd.print(tLv);lcd.print("       ");
  lcd.setCursor(0,2);sprintf(b,"HI:%5d",tHi);lcd.print(b);
  lcd.setCursor(0,3);lcd.print("NX:");
  uint8_t nw=TPIECES[tpNext].w;
  for(int i=0;i<4;i++) lcd.write(i<(int)nw?(byte)2:' ');
  lcd.print("   ");
}

void runTetris(){
  loadTetrisChars(); lcdFill();
  lcd.setCursor(4,0); lcd.print("** TETRIS **");
  lcd.setCursor(0,1); lcd.print("X=move  UP=rotate");
  lcd.setCursor(0,2); lcd.print("DOWN = soft drop");
  lcd.setCursor(0,3); lcd.print("Hold>>");
  while(!holdRight(900)){}

  memset(tb,0,sizeof(tb));
  tSc=0;tLv=1;tLines=0;tDropMs=700;
  tpNext=random(TP_NUM);tNew();lcdFill();
  tLastDrop=millis();

  unsigned long lastIn=0; int inRpt=160;
  int8_t lastDX=0,lastDY=0;

  while(true){
    int8_t dx,dy; joyRead(dx,dy);
    bool inRdy=(millis()-lastIn>=(unsigned long)inRpt);
    if(inRdy){
      bool act=false;
      if(dx==-1&&tpValid(tpX-1,tpRow)){tpX--;act=true;}
      if(dx== 1&&tpValid(tpX+1,tpRow)){tpX++;act=true;}
      if(dy==-1&&dy!=lastDY){
        // Rotate: reverse col order
        for(int i=0;i<(int)tpW/2;i++){int8_t t=tpCols[i];tpCols[i]=tpCols[tpW-1-i];tpCols[tpW-1-i]=t;}
        if(!tpValid(tpX,tpRow)){for(int i=0;i<(int)tpW/2;i++){int8_t t=tpCols[i];tpCols[i]=tpCols[tpW-1-i];tpCols[tpW-1-i]=t;}}
        act=true;
      }
      if(dy==1&&tpValid(tpX,tpRow+1)){tpRow++;act=true;}
      if(act) lastIn=millis();
    }
    lastDX=dx;lastDY=dy;

    if(millis()-tLastDrop>=(unsigned long)tDropMs){
      tLastDrop=millis();
      if(tpValid(tpX,tpRow+1)){tpRow++;}
      else{
        tpLock();
        int cl=tClear();
        if(cl>0){tLines+=cl;tSc+=cl*100*tLv;if(tLines>=tLv*4){tLv++;tDropMs=max(150,tDropMs-70);}if(tSc>tHi)tHi=tSc;}
        tNew();
        if(!tpValid(tpX,tpRow)) break;
      }
    }
    tDraw(); delay(20);
  }
  if(tSc>hiTetris) hiTetris=tSc;
  if(tSc>tHi) tHi=tSc;
  gameOverScreen(tSc,hiTetris);
}

// ╔══════════════════════════════════════════════════════════════╗
// ║  SETUP & LOOP                                               ║
// ╚══════════════════════════════════════════════════════════════╝
void setup(){
  Wire.begin(SDA_PIN,SCL_PIN);
  lcd.begin(LCD_COLS,LCD_ROWS);
  lcd.backlight();
  analogReadResolution(12);
  randomSeed(analogRead(33));

  // Boot splash
  lcdFill();
  lcd.setCursor(1,1); lcd.print("  IT FEST ARCADE");
  lcd.setCursor(1,2); lcd.print("  6 Games Inside");
  delay(1800);
}

void loop(){
  int choice=runMenu();
  switch(choice){
    case 0: runDino();   break;
    case 1: runSnake();  break;
    case 2: runMaze();   break;
    case 3: runFlappy(); break;
    case 4: runPac();    break;
    case 5: runTetris(); break;
  }
  delay(300);
}

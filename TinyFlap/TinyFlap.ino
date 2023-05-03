#include <TFT_eSPI.h>
#include <SPI.h>
#include <Ticker.h>
#include <Preferences.h>

#define LEFT 0
#define RIGHT 47

#define HIGH_SCORES "high_scores"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite game_over_spr = TFT_eSprite(&tft);

int refreshRate = 20;

bool leftHold = false;

float GRAVITY = 0.3; // normal 0.3 // easy 0.2 // hard 0.3
float JUMP_STRENGTH = 2.2;  // normal 2.2 // easy 2 // hard 3.8

int bird_w = 26;
int bird_h = 20;

float bird_y = 64;
float bird_speed = 0;
int bird_angle = 0;

int pipe_x = 128;
int pipe_gap_y = 60;
int pipe_gap_size = 40; // normal 40 / easy 50
int pipe_speed = 2; // Even numbers only

int groundPos = 0;
int GROUND_SPEED = pipe_speed / 2;
int GROUND_STRIPE_WIDTH = 8;

int cloud_x1 = 20;
int cloud_x2 = 80;

int landscape_x = 0;
int landscape_speed = 1;

const float flapInterval = 0.15;
const int flapStates[] = {1, 2, 3, 2};
const int numFlapStates = sizeof(flapStates) / sizeof(flapStates[0]);
int currentFlapStateIndex = 0;
int flapState = flapStates[currentFlapStateIndex];
Ticker flapTicker;

const int gameModes[] = {1, 2, 3};
const int numGameModes = sizeof(gameModes) / sizeof(gameModes[0]);
int currentGameModeIndex = 1;
int gameMode = gameModes[currentGameModeIndex];

unsigned long lastUpdate = 0;

int score = 0;
Preferences highScores;
int bestScore = 0;
bool newBestScore = false;

bool ready = false;
bool isCollision = false;

void setup() {
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);

  highScores.begin(HIGH_SCORES, false);

  flapTicker.attach(flapInterval, flapTick);

  tft.init();
  tft.setRotation(2);
  tft.setSwapBytes(true);

  sprite.createSprite(128,128);
  //sprite.setSwapBytes(true);

  // sprite.setTextColor(TFT_DARKGREY, TFT_SKYBLUE);
  // sprite.setTextSize(2);
  // sprite.setTextDatum(TR_DATUM);

}

void setGameMode(int mode) {
  switch(mode) {
    case 1: // normal progressive
      GRAVITY = 0.3;
      JUMP_STRENGTH = 2.2;
      pipe_gap_size = 160;
      break;
    case 2: // normal
      GRAVITY = 0.3;
      JUMP_STRENGTH = 2.2;
      pipe_gap_size = 40;
      break;
    case 3: // hard
      GRAVITY = 0.4;
      JUMP_STRENGTH = 4;
      pipe_gap_size = 40;
      break;
    default:
      ; // Nothing here ..
  }
}

void toggleGameMode() {
  currentGameModeIndex = (currentGameModeIndex + 1) % numGameModes;
  gameMode = gameModes[currentGameModeIndex];

  setGameMode(gameMode);
}

void flapTick() {
  currentFlapStateIndex = (currentFlapStateIndex + 1) % numFlapStates;
  flapState = flapStates[currentFlapStateIndex];
}

void drawBird2(float y) { // For reference

  uint16_t color;

  if (isCollision) {
    color = TFT_RED;
  } else {
    color = TFT_YELLOW;
  }

  // Body and Head
  sprite.fillSmoothCircle(8, y, 7, TFT_BLACK); // Body outline
  sprite.fillSmoothCircle(13, y, 6, TFT_BLACK); // Head outline
  sprite.fillSmoothCircle(8, y, 6, color);
  sprite.fillSmoothCircle(13, y, 5, color);

  // Beak
  sprite.fillSmoothRoundRect(14, y, 8, 6, 2, TFT_BLACK); // Beak outline
  sprite.fillSmoothRoundRect(14, y, 7, 5, 2, TFT_ORANGE);

  // Eye
  sprite.fillSmoothCircle(15, y - 3, 3, TFT_BLACK); // Eye outline
  sprite.fillSmoothCircle(15, y - 3, 2, TFT_WHITE);
  sprite.fillSmoothCircle(16, y - 3, 1, TFT_BLACK); // 1

  // Wing
  sprite.drawFastHLine(5, y + 4, 7, TFT_DARKGREY);
  sprite.drawFastHLine(5, y + 5, 6, TFT_DARKGREY);
  sprite.drawFastHLine(4, y + 3, 9, TFT_BLACK); // Wing outline
}

TFT_eSprite drawBird3() {

  uint16_t color;

  if (isCollision) {
    color = TFT_LIGHTGREY; 
    //color = TFT_RED;
  } else {
    switch(gameMode) {
      case 1: // progressive
        color = TFT_CYAN;
        break;
      case 2: // normal
        color = TFT_GOLD;
        break;
      case 3: // hard
        //color = TFT_BLUE;
        //color = TFT_MAGENTA;
        color = TFT_OLIVE;
        break;
      default:
        ; // Nothing here ..
    }
  }

  float x = (bird_w / 2) - 4;
  float y = bird_h / 2;

  TFT_eSprite back_spr = TFT_eSprite(&tft);
  back_spr.createSprite(bird_w + 2, bird_h + 2);
  back_spr.fillSprite(TFT_SKYBLUE);

  TFT_eSprite bird_spr = TFT_eSprite(&tft);

  bird_spr.createSprite(bird_w, bird_h);
  //bird_spr.setSwapBytes(true);

  bird_spr.fillSprite(TFT_SKYBLUE);

  // Body and Head
  bird_spr.fillSmoothCircle(x, y, 7, TFT_BLACK); // Body outline
  bird_spr.fillSmoothCircle(x + 5, y, 6, TFT_BLACK); // Head outline
  bird_spr.fillSmoothCircle(x, y, 6, color);
  bird_spr.fillSmoothCircle(x + 5, y, 5, color);

  // Beak
  bird_spr.fillSmoothRoundRect(x + 5, y - 1, 9, 7, 2, TFT_BLACK);
  bird_spr.fillSmoothRoundRect(x + 6, y, 7, 5, 2, TFT_ORANGE);
  int mouth_x = (isCollision) ? x + 7 : x + 8;
  int mouth_y = (isCollision) ? y + 2 : y + 3;
  int mouth_w = (isCollision) ? 6 : 5;
  bird_spr.drawFastHLine(mouth_x, mouth_y, mouth_w, TFT_BLACK); // Funny: y + 2

  // Eye
  bird_spr.fillSmoothCircle(x + 7, y - 3, 3, TFT_BLACK);
  bird_spr.fillSmoothCircle(x + 7, y - 3, 2, TFT_WHITE);
  int eyeBall_x = (isCollision) ? x + 7 : x + 8;
  bird_spr.fillSmoothCircle(eyeBall_x, y - 3, 1, TFT_BLACK); // Funny: x + 7

  // Flappy wing
  if (isCollision) {
    flapState = 2;
  }

  switch(flapState) {
    case 1: // down
      bird_spr.fillSmoothRoundRect(x - 8, y - 1, 10, 7, 2, TFT_BLACK);
      bird_spr.fillSmoothRoundRect(x - 7, y, 8, 5, 2, color);
      break;
    case 2: // neutral
      bird_spr.fillSmoothRoundRect(x - 8, y - 2, 10, 4, 2, TFT_BLACK);
      bird_spr.fillSmoothRoundRect(x - 7, y - 1, 8, 2, 2, color);
      break;
    case 3: // up
      bird_spr.fillSmoothRoundRect(x - 7, y - 4, 10, 6, 2, TFT_BLACK);
      bird_spr.fillSmoothRoundRect(x - 6, y - 3, 8, 4, 2, TFT_PINK);
      break;
    // default:
    //   // Nothing here ..
  }

  bird_spr.pushRotated(&back_spr, bird_angle, TFT_SKYBLUE);

  return back_spr; //bird_spr;

}

void drawCloud(int x, int y) {
  uint16_t cloudColor = TFT_WHITE;

  // Draw cloud circles
  sprite.fillCircle(x, y, 4, cloudColor);
  sprite.fillCircle(x + 5, y + 2, 5, cloudColor);
  sprite.fillCircle(x + 12, y, 4, cloudColor);
  sprite.fillCircle(x + 9, y - 2, 3, cloudColor);
}

void drawClouds() {
  // Update cloud positions
  cloud_x1 -= 1;
  cloud_x2 -= 1;

  if (cloud_x1 < -16) {
    cloud_x1 = 128;
  }

  if (cloud_x2 < -16) {
    cloud_x2 = 128;
  }

  drawCloud(cloud_x1, 20);
  drawCloud(cloud_x2, 40);
}

void drawLandscape() {
  // Update landscape position
  landscape_x += landscape_speed;
}

void drawPipe2(int x, int gapY) {
  // Upper pipe
  sprite.fillRect(x-1, 0-1, 6+2, (gapY - pipe_gap_size / 2) + 2, TFT_BLACK);
  sprite.fillRectHGradient(x, 0, 3, gapY - pipe_gap_size / 2, TFT_GREEN, TFT_DARKGREEN);
  sprite.fillRectHGradient(x+3, 0, 3, gapY - pipe_gap_size / 2, TFT_DARKGREEN, TFT_GREEN);

  sprite.fillRect((x - 2) - 1, (gapY - pipe_gap_size / 2 - 4) - 1, 10 + 2, 4 + 2, TFT_BLACK);
  sprite.fillRect(x - 2, gapY - pipe_gap_size / 2 - 4, 10, 4, TFT_GREEN);
  
  // Lower pipe
  sprite.fillRect(x - 1, (gapY + pipe_gap_size / 2) - 1, 6 + 2, (128 - (gapY + pipe_gap_size / 2)) + 2, TFT_BLACK);
  sprite.fillRectHGradient(x, gapY + pipe_gap_size / 2, 3, 128 - (gapY + pipe_gap_size / 2), TFT_GREEN, TFT_DARKGREEN);
  sprite.fillRectHGradient(x+3, gapY + pipe_gap_size / 2, 3, 128 - (gapY + pipe_gap_size / 2), TFT_DARKGREEN, TFT_GREEN);

  sprite.fillRect((x - 2) - 1, (gapY + pipe_gap_size / 2) - 1, 10 + 2, 4 + 2, TFT_BLACK);
  sprite.fillRect(x - 2, gapY + pipe_gap_size / 2, 10, 4, TFT_GREEN);
}

void drawPipes() {
  // Update pipe position
  pipe_x -= pipe_speed;

  if (pipe_x < -6) {
    pipe_x = 128;
    pipe_gap_y = random(24, 90);
    score += 1;

    if (gameMode == 1 && pipe_gap_size > 40) {
      pipe_gap_size -= 1;
    }
  }

  drawPipe2(pipe_x, pipe_gap_y);
}

void drawGround() {
  int screenOffsetX = -groundPos % GROUND_STRIPE_WIDTH * 2;
  int stripeCount = (128 /*screen width*/ + GROUND_STRIPE_WIDTH) / GROUND_STRIPE_WIDTH + 1;

  sprite.drawFastHLine(0, 128 - 7, 128, TFT_BLACK);
  sprite.drawFastHLine(0, 128 - 6, 128, TFT_BROWN);
  
  // Draw the ground stripes at the current position
  for (int i = 0; i < stripeCount; i++) {
    uint16_t stripeColor = (i % 2 == 0) ? TFT_GREEN : TFT_DARKGREEN;
    int x = i * GROUND_STRIPE_WIDTH + screenOffsetX;
    int y = 128 /*screen height*/ - 5 /*GROUND_HEIGHT*/;
    int w = GROUND_STRIPE_WIDTH;
    int h = 5 /*GROUND_STRIPE_HEIGHT*/;
    if (x + w > 0 && x < 128/*tft.width()*/) {
      sprite.fillRect(max(x, 0), y, min(w, 128 /*tft.width()*/ - x), h, stripeColor);
    }
  }

  groundPos += GROUND_SPEED;

  // If the ground has moved past the width of the screen, reset its position
  if (groundPos >= GROUND_STRIPE_WIDTH) {
    groundPos = 0;
  }
}

void drawScore() {
  sprite.setTextColor(TFT_DARKGREY, TFT_SKYBLUE);
  sprite.setTextSize(2);
  sprite.setTextDatum(TR_DATUM);
  sprite.drawString(String(score), 126, 2);
}

void drawGetReady() {

  sprite.setTextColor(TFT_DARKGREY, TFT_SKYBLUE);
  sprite.setTextSize(1);

  sprite.setTextDatum(MC_DATUM);
  sprite.drawString("GET READY!", 64, 64);

  String modeText;
  switch(gameMode) {
    case 1: // normal progressive
      modeText = "- progressive -";
      break;
    case 2: // normal
      modeText = "- normal -";
      break;
    case 3: // hard
      modeText = "- hard -";
      break;
    default:
      ; // Nothing here ..
  }
  sprite.drawString(modeText, 64, 90); // Mode display

  sprite.setTextDatum(BR_DATUM);
  sprite.drawString("START", 124, 117); // Start

  sprite.setTextDatum(BL_DATUM);
  sprite.drawString("MODE", 4, 117); // Mode select
}

void gameOver() {
  game_over_spr.createSprite(96, 105);

  game_over_spr.fillSprite(TFT_SKYBLUE);
  game_over_spr.fillSmoothRoundRect(0, 0, 80, 80, 5, TFT_BLACK); //, TFT_SKYBLUE
  game_over_spr.fillSmoothRoundRect(1, 1, 78, 78, 4, TFT_OLIVE);

  game_over_spr.setTextColor(TFT_WHITE, TFT_OLIVE);
  game_over_spr.setTextSize(1);
  game_over_spr.setTextDatum(TC_DATUM);

  game_over_spr.drawString("SCORE", 40, 10);
  String bestScoreText;
  if (newBestScore) {
    bestScoreText = "NEW BEST";
  } else {
    bestScoreText = "BEST";
  }
  game_over_spr.drawString(bestScoreText, 40, 45);

  game_over_spr.setTextSize(2);
  game_over_spr.drawString(String(score), 40, 22);
  game_over_spr.drawString(String(bestScore), 40, 57);

  game_over_spr.setTextColor(TFT_DARKGREY, TFT_SKYBLUE);
  game_over_spr.setTextSize(1);
  game_over_spr.setTextDatum(BR_DATUM);
  game_over_spr.drawString("CONTINUE", 92, 101);

  game_over_spr.pushSprite(32, 16);
}

void drawFrame() {
  sprite.fillSprite(TFT_SKYBLUE);

  drawClouds();

  //drawLandscape();

  if (ready) {
    drawPipes();
  }
  
  drawGround();

  if (ready && !isCollision) {
    drawScore();
  }
  
  //drawBird2(bird_y);
  drawBird3().pushToSprite(&sprite, -1 /* Sprite offset */, bird_y - (bird_h / 2), TFT_SKYBLUE);

  if (!ready) {
    drawGetReady();
  }

  sprite.pushSprite(0, 0);
}

int getHighScore() {
  int prefHighScore;
  switch(gameMode) {
    case 1: // normal progressive
      prefHighScore = highScores.getInt("game_mode_1", 0);
      break;
    case 2: // normal
      prefHighScore = highScores.getInt("game_mode_2", 0);
      break;
    case 3: // hard
      prefHighScore = highScores.getInt("game_mode_3", 0);
      break;
    default:
      ; // Nothing here ..
  }
  return prefHighScore;
}

void saveHighScore() {
  switch(gameMode) {
    case 1: // normal progressive
      highScores.putInt("game_mode_1", bestScore);
      break;
    case 2: // normal
      highScores.putInt("game_mode_2", bestScore);
      break;
    case 3: // hard
      highScores.putInt("game_mode_3", bestScore);
      break;
    default:
      ; // Nothing here ..
  }
}

void clearHighScore() {
  switch(gameMode) {
    case 1: // normal progressive
      highScores.putInt("game_mode_1", 0);
      break;
    case 2: // normal
      highScores.putInt("game_mode_2", 0);
      break;
    case 3: // hard
      highScores.putInt("game_mode_3", 0);
      break;
    default:
      ; // Nothing here ..
  }
  bestScore = 0;
}

void resetGame() {
  bird_y = 64;
  bird_speed = 0;
  bird_angle = 0;
  pipe_x = 128;
  score = 0;
  bestScore = getHighScore();
  newBestScore = false;
  setGameMode(gameMode); // Calling this to reset values in "progressive mode"
  isCollision = false;
  ready = false;
}

void loop() {

  while (!ready) {
    drawFrame();
    delay(refreshRate);

    if (digitalRead(RIGHT) == LOW) {
      resetGame();
      ready = true;
      break;
    }

    if (digitalRead(LEFT) == LOW) {
      toggleGameMode();
      while (digitalRead(LEFT) == LOW) {
        delay(refreshRate);
      }
    }
  }

  while (isCollision) {

    // Update best score
    if (score > bestScore) {
      newBestScore = true;
      bestScore = score;
      saveHighScore();
    }

    delay(600);
    gameOver();

    while (leftHold) { // Safty to avoid unintended game reset
      if (digitalRead(RIGHT) == HIGH) {
        leftHold = false;
      }
    }

    bool halt = true;
    while (halt) {
      while (digitalRead(RIGHT) == LOW) {
        resetGame();
        halt = false;
      }

      // Clear high score
      if (digitalRead(LEFT) == LOW) {
        int resetStart = millis();
        while (digitalRead(LEFT) == LOW) {
          if (millis() - resetStart > 8000) {
            clearHighScore();
            gameOver(); // Drawing
            break;
          }
        }
      }

    }
  }

  unsigned long currentMillis = millis();
  
  if (currentMillis - lastUpdate >= refreshRate) {
    lastUpdate = currentMillis;

    // Update bird position and angle
    bird_speed += GRAVITY;
    bird_y += bird_speed;
    tft.setPivot(12, bird_y);
    bird_angle = std::max(-45, std::min(static_cast<int>(bird_speed * 7), 60));

    // CLOUD WAS HERE

    // PIPE WAS HERE

    // TODO could be an issue with possitions updated inside the drawFrame, happening after the collision check.

    // Check for collisions
    if (bird_y < -128 /* Sky is not the limit */ 
      || bird_y > 128 - 10 /*Bird*/ - 3 /* Ground */ 
      || (pipe_x < 18 && pipe_x > 0 && (bird_y < pipe_gap_y - pipe_gap_size / 2 
        || bird_y > pipe_gap_y + pipe_gap_size / 2))) 
    {
      isCollision = true;
    }

    drawFrame();

    // Check for input
    if (digitalRead(RIGHT) == LOW) {
      bird_speed = -JUMP_STRENGTH;
      bird_angle = -30;
      leftHold = true;
    } else {
      leftHold = false;
    }

    if (digitalRead(LEFT) == LOW) {
      pipe_speed = 4;
    } else {
      pipe_speed = 2;
    }
    GROUND_SPEED = pipe_speed / 2;

  }
}
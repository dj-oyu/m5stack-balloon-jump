#include "game_config.h"
#include <M5Unified.h>
#include "balloon_game.h"

struct Obstacle {
  int x;
  int y;
  int width;
  int height;
  bool active;
  int vy;
};

GameState gameState;
Obstacle obstacles[5];

void initGame() {
  gameState.balloonY = SCREEN_HEIGHT / 2;
  gameState.balloonVelocity = 0;
  gameState.score = 0;
  gameState.height = 0;
  gameState.isBlowing = false;
  gameState.isBurst = false;
  gameState.blowStartTime = 0;
  gameState.lastObstacleTime = 0;
  gameState.burstTime = 0;
  gameState.prevBalloonY = gameState.balloonY;
  gameState.prevUIY = 0;
  gameState.level = 1;
  gameState.nextLevelHeight = LEVEL_HEIGHT_STEP;
  gameState.showLevelUp = false;
  gameState.levelUpTime = 0;
  
  memset(gameState.micData, 0, sizeof(gameState.micData));
  
  for (int i = 0; i < 5; i++) {
    obstacles[i].active = false;
    obstacles[i].vy = 0;
    gameState.prevObstaclesX[i] = 0;
    gameState.prevObstaclesY[i] = 0;
    gameState.prevObstaclesActive[i] = false;
  }

  memset(gameState.prevWaveY, 0, sizeof(gameState.prevWaveY));
  memset(gameState.prevWaveH, 0, sizeof(gameState.prevWaveH));
  
  M5.Display.setRotation(0);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextColor(TEXT_COLOR);
  M5.Display.startWrite();
  
  M5.Speaker.end();
  M5.Mic.begin();
  
  // ゲーム開始時に画面をクリア
  M5.Display.fillScreen(BACKGROUND_COLOR);
  
  M5.Display.display();
}

bool detectBlow() {
  if (gameState.isBurst) return false;
  
  if (M5.Mic.record(gameState.micData, RECORD_LENGTH, RECORD_SAMPLERATE)) {
    int16_t maxAmplitude = 0;
    for (int i = 0; i < RECORD_LENGTH; i++) {
      if (abs(gameState.micData[i]) > maxAmplitude) {
        maxAmplitude = abs(gameState.micData[i]);
      }
    }
    
    if (maxAmplitude > MIC_THRESHOLD) {
      if (!gameState.isBlowing) {
        gameState.isBlowing = true;
        gameState.blowStartTime = millis();
      }
      return true;
    } else {
      if (gameState.isBlowing) {
        gameState.isBlowing = false;
      }
      return false;
    }
  }
  return false;
}

void updateBalloon() {
  if (gameState.isBurst) return;
  
  if (gameState.isBlowing) {
    gameState.balloonVelocity -= 2;
  } else {
    gameState.balloonVelocity += 1;
  }
  
  if (gameState.balloonVelocity > 5) gameState.balloonVelocity = 5;
  if (gameState.balloonVelocity < -8) gameState.balloonVelocity = -8;
  
  gameState.balloonY += gameState.balloonVelocity;
  
  if (gameState.balloonY < BALLOON_RADIUS) {
    gameState.balloonY = BALLOON_RADIUS;
    gameState.balloonVelocity = 0;
  }
  
  if (gameState.balloonY > SCREEN_HEIGHT - GROUND_HEIGHT - BALLOON_RADIUS) {
    gameState.balloonY = SCREEN_HEIGHT - GROUND_HEIGHT - BALLOON_RADIUS;
    gameState.isBurst = true;
    gameState.burstTime = millis();
  }
  
  if (gameState.balloonVelocity < 0) {
    gameState.height -= gameState.balloonVelocity;
  }

  if (gameState.height >= gameState.nextLevelHeight &&
      gameState.level < MAX_LEVEL) {
    gameState.level++;
    gameState.nextLevelHeight += LEVEL_HEIGHT_STEP;
    gameState.showLevelUp = true;
    gameState.levelUpTime = millis();
  }
}

void updateObstacles() {
  if (gameState.isBurst) return;
  
  unsigned long currentTime = millis();
  unsigned long interval = BASE_OBSTACLE_INTERVAL - (gameState.level - 1) * 150;
  if (interval < MIN_OBSTACLE_INTERVAL) interval = MIN_OBSTACLE_INTERVAL;

  if (currentTime - gameState.lastObstacleTime > interval) {
    for (int i = 0; i < 5; i++) {
      if (!obstacles[i].active) {
        obstacles[i].active = true;
        obstacles[i].x = SCREEN_WIDTH;
        obstacles[i].y = random(GROUND_HEIGHT + 30, SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT - 30);
        obstacles[i].width = OBSTACLE_WIDTH;
        obstacles[i].height = OBSTACLE_HEIGHT;
        if (gameState.level >= 2) {
          obstacles[i].vy = random(-gameState.level, gameState.level + 1);
          if (obstacles[i].vy == 0) obstacles[i].vy = 1;
        } else {
          obstacles[i].vy = 0;
        }
        gameState.lastObstacleTime = currentTime;
        break;
      }
    }
  }
  
  for (int i = 0; i < 5; i++) {
    if (obstacles[i].active) {
      obstacles[i].x -= GAME_SPEED + (gameState.level - 1) * 2;
      obstacles[i].y += obstacles[i].vy;

      if (obstacles[i].y < WAVE_POS_Y + 10 ||
          obstacles[i].y > SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT - 10) {
        obstacles[i].vy = -obstacles[i].vy;
      }

      if (obstacles[i].x < -OBSTACLE_WIDTH) {
        obstacles[i].active = false;
        obstacles[i].vy = 0;
        gameState.score += 10;
      }
    }
  }
}

bool checkCollision() {
  if (gameState.isBurst) return false;
  
  int balloonX = SCREEN_WIDTH / 2;
  
  for (int i = 0; i < 5; i++) {
    if (obstacles[i].active) {
      if (balloonX + BALLOON_RADIUS > obstacles[i].x &&
          balloonX - BALLOON_RADIUS < obstacles[i].x + obstacles[i].width &&
          gameState.balloonY + BALLOON_RADIUS > obstacles[i].y &&
          gameState.balloonY - BALLOON_RADIUS < obstacles[i].y + obstacles[i].height) {
        gameState.isBurst = true;
        gameState.burstTime = millis();
        return true;
      }
    }
  }
  return false;
}

void drawBalloon() {
  int balloonX = SCREEN_WIDTH / 2;
  
  M5.Display.fillCircle(balloonX, gameState.prevBalloonY, BALLOON_RADIUS + 5, BACKGROUND_COLOR);
  
  M5.Display.drawLine(balloonX, gameState.prevBalloonY + BALLOON_RADIUS, balloonX, gameState.prevBalloonY + BALLOON_RADIUS + 25, BACKGROUND_COLOR);
  
  if (gameState.isBurst) {
    unsigned long elapsed = millis() - gameState.burstTime;
    int burstRadius = BALLOON_RADIUS + (elapsed / 30);
    
    if (elapsed < 150) {
      M5.Display.fillCircle(balloonX, gameState.balloonY, burstRadius, BURST_COLOR);
    } else {
      M5.Display.fillCircle(balloonX, gameState.balloonY, BALLOON_RADIUS + 5, BACKGROUND_COLOR);
    }
  } else {
    M5.Display.fillCircle(balloonX, gameState.balloonY, BALLOON_RADIUS, BALLOON_COLOR);
    
    M5.Display.drawLine(balloonX, gameState.balloonY + BALLOON_RADIUS, balloonX, gameState.balloonY + BALLOON_RADIUS + 20, TFT_BLACK);
    
    M5.Display.fillCircle(balloonX - BALLOON_RADIUS/3, gameState.balloonY - BALLOON_RADIUS/3, BALLOON_RADIUS/4, TFT_WHITE);
  }
  
  gameState.prevBalloonY = gameState.balloonY;
}

void drawGround() {
  // 常に地面を描画
  M5.Display.fillRect(0, SCREEN_HEIGHT - GROUND_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT, GROUND_COLOR);
  
  for (int i = 0; i < SCREEN_WIDTH; i += 15) {
    M5.Display.drawLine(i, SCREEN_HEIGHT - GROUND_HEIGHT, i + 7, SCREEN_HEIGHT, TFT_DARKGREEN);
  }
  
  for (int i = 0; i < SCREEN_WIDTH; i += 30) {
    M5.Display.fillCircle(i, SCREEN_HEIGHT - GROUND_HEIGHT/2, GROUND_HEIGHT/4, TFT_GREEN);
  }
}

void drawObstacles() {
  for (int i = 0; i < 5; i++) {
    if (gameState.prevObstaclesActive[i]) {
      M5.Display.fillRect(gameState.prevObstaclesX[i], gameState.prevObstaclesY[i], 
                          OBSTACLE_WIDTH, OBSTACLE_HEIGHT, 
                          BACKGROUND_COLOR);
    }
    
    if (obstacles[i].active) {
      M5.Display.fillRect(obstacles[i].x, obstacles[i].y, obstacles[i].width, obstacles[i].height, OBSTACLE_COLOR);
      
      M5.Display.fillCircle(obstacles[i].x + obstacles[i].width/2, obstacles[i].y + obstacles[i].height/2, obstacles[i].width/3, TFT_DARKGREEN);
      
      M5.Display.fillCircle(obstacles[i].x + obstacles[i].width/3, obstacles[i].y + obstacles[i].height/3, obstacles[i].width/6, TFT_GREEN);
      M5.Display.fillCircle(obstacles[i].x + 2*obstacles[i].width/3, obstacles[i].y + 2*obstacles[i].height/3, obstacles[i].width/6, TFT_GREEN);
    }
    
    gameState.prevObstaclesX[i] = obstacles[i].x;
    gameState.prevObstaclesY[i] = obstacles[i].y;
    gameState.prevObstaclesActive[i] = obstacles[i].active;
  }
}

void drawUI() {
  // UI背景をクリア
  M5.Display.fillRect(0, 0, SCREEN_WIDTH, 60, BACKGROUND_COLOR);
  
  // UIの描画
  M5.Display.setTextSize(UI_FONT_SIZE);
  M5.Display.setTextColor(TEXT_COLOR);
  M5.Display.setCursor(15, 10);
  M5.Display.printf("Height: %dm", gameState.height);
  
  M5.Display.setCursor(15, 35);
  M5.Display.printf("Score: %d", gameState.score);

  M5.Display.setCursor(SCREEN_WIDTH - 80, 35);
  M5.Display.printf("Lv:%d", gameState.level);

  if (gameState.showLevelUp) {
    if (millis() - gameState.levelUpTime < LEVEL_UP_DURATION) {
      M5.Display.setTextSize(LEVEL_UP_FONT_SIZE);
      M5.Display.setTextColor(LEVEL_UP_COLOR);
      M5.Display.setTextDatum(top_center);
      M5.Display.setCursor(SCREEN_WIDTH/2, 5);
      M5.Display.printf("\u2605 LEVEL UP! Lv%d \u2605", gameState.level);
      M5.Display.setTextSize(UI_FONT_SIZE);
      M5.Display.setTextColor(TEXT_COLOR);
      M5.Display.setTextDatum(middle_center);
    } else {
      gameState.showLevelUp = false;
    }
  }
  
  if (gameState.isBlowing) {
    M5.Display.setCursor(SCREEN_WIDTH - 100, 10);
    M5.Display.printf("Blowing!");
  }
}

void drawWaveform() {
  int32_t w = SCREEN_WIDTH;
  if (w > RECORD_LENGTH - 1) {
    w = RECORD_LENGTH - 1;
  }

  for (int32_t x = 0; x < w; ++x) {
    M5.Display.writeFastVLine(x, gameState.prevWaveY[x], gameState.prevWaveH[x], BACKGROUND_COLOR);
    int32_t y1 = (gameState.micData[x] >> WAVE_SHIFT);
    int32_t y2 = (gameState.micData[x + 1] >> WAVE_SHIFT);
    if (y1 > y2) {
      int32_t t = y1;
      y1 = y2;
      y2 = t;
    }
    int32_t y = WAVE_POS_Y + (WAVE_HEIGHT >> 1) + y1;
    int32_t h = WAVE_POS_Y + (WAVE_HEIGHT >> 1) + y2 + 1 - y;
    gameState.prevWaveY[x] = y;
    gameState.prevWaveH[x] = h;
    M5.Display.writeFastVLine(x, gameState.prevWaveY[x], gameState.prevWaveH[x], WAVE_COLOR);
  }
}

void drawGame() {
  drawGround();
  drawWaveform();
  drawObstacles();
  drawBalloon();
  drawUI();

  M5.Display.display();
}

void gameOver() {
  while (millis() - gameState.burstTime < 150) {
    delay(10);
  }
  
  M5.Display.fillScreen(GAME_OVER_COLOR);
  M5.Display.setTextSize(GAME_FONT_SIZE);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setCursor(SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 40);
  M5.Display.printf("GAME OVER");
  
  M5.Display.setTextSize(UI_FONT_SIZE);
  M5.Display.setCursor(SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 10);
  M5.Display.printf("Height: %dm", gameState.height);
  M5.Display.setCursor(SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 10);
  M5.Display.printf("Score: %d", gameState.score);

  M5.Display.setCursor(SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 25);
  M5.Display.printf("Level: %d", gameState.level);
  
  M5.Display.setTextSize(UI_FONT_SIZE);
  M5.Display.setCursor(SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 40);
  M5.Display.printf("Touch to restart");
  
  M5.Display.display();
  
  while (true) {
    M5.update();
    if (M5.Touch.getCount() && M5.Touch.getDetail(0).wasClicked()) {
      initGame();
      break;
    }
    delay(100);
  }
}

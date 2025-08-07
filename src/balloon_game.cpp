#include "game_config.h"
#include <M5Unified.h>
#include "balloon_game.h"

GameState gameState;
Obstacle obstacles[5];

void initGame() {
  gameState.balloonY = SCREEN_HEIGHT / 2;
  gameState.balloonVelocity = 0;
  gameState.score = 0;
  gameState.height = 0;
  gameState.blowPower = 0;
  gameState.isBurst = false;
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
    obstacles[i].param1 = 0;
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

void updateBlow() {
  if (gameState.isBurst) {
    gameState.blowPower = 0;
    return;
  }
  
  if (M5.Mic.record(gameState.micData, RECORD_LENGTH, RECORD_SAMPLERATE)) {
    int16_t maxAmplitude = 0;
    for (int i = 0; i < RECORD_LENGTH; i++) {
      if (abs(gameState.micData[i]) > maxAmplitude) {
        maxAmplitude = abs(gameState.micData[i]);
      }
    }
    
    if (maxAmplitude > MIC_THRESHOLD) {
      // Map amplitude to blow power. Let's use a simple linear mapping for now.
      // A more sophisticated mapping could be used for better feel.
      // e.g. log scale, or a curve.
      // The max amplitude from the mic is 32767, but it rarely reaches that.
      // Let's set a practical max for mapping.
      int practicalMaxAmplitude = 10000;
      gameState.blowPower = map(maxAmplitude, MIC_THRESHOLD, practicalMaxAmplitude, 1, MAX_BLOW_POWER);
      if (gameState.blowPower > MAX_BLOW_POWER) {
        gameState.blowPower = MAX_BLOW_POWER;
      }
    } else {
      gameState.blowPower = 0;
    }
  } else {
    gameState.blowPower = 0;
  }
}

void updateBalloon() {
  if (gameState.isBurst) return;

  // Apply forces based on blow power and gravity
  float upwardForce = (float)gameState.blowPower * BALLOON_LIFT;
  
  gameState.balloonVelocity -= upwardForce;
  gameState.balloonVelocity += BALLOON_GRAVITY;
  
  // Terminal velocity
  if (gameState.balloonVelocity > MAX_VELOCITY_DOWN) gameState.balloonVelocity = MAX_VELOCITY_DOWN; // Downward
  if (gameState.balloonVelocity < MAX_VELOCITY_UP) gameState.balloonVelocity = MAX_VELOCITY_UP; // Upward
  
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
    M5.Mic.end();
    M5.Speaker.begin();
    M5.Speaker.setVolume(200);
    M5.Speaker.tone(880, 200);
    while(M5.Speaker.isPlaying()) {
      delay(5);
    }
    M5.Speaker.end();
    M5.Mic.begin();
  }
}

void updateObstacles() {
  if (gameState.isBurst) return;

  // === Obstacle Spawning ===
  // This section determines when and what type of new obstacles to create.
  unsigned long currentTime = millis();
  unsigned long interval = BASE_OBSTACLE_INTERVAL - (gameState.level - 1) * 100;
  if (interval < MIN_OBSTACLE_INTERVAL) interval = MIN_OBSTACLE_INTERVAL;

  if (currentTime - gameState.lastObstacleTime > interval) {
    int obstacleTypeRoll = random(100);
    int free_slot = -1;
    for(int i=0; i<5; ++i) {
      if(!obstacles[i].active) {
        free_slot = i;
        break;
      }
    }

    if (free_slot != -1) {
        gameState.lastObstacleTime = currentTime;
        Obstacle& o = obstacles[free_slot];
        o.active = true;
        o.x = SCREEN_WIDTH;
        o.width = OBSTACLE_WIDTH;
        o.height = OBSTACLE_HEIGHT;
        o.param1 = 0;

        // Determine obstacle type based on level
        if (gameState.level < 3) { // Levels 1-2: Static obstacles only
            o.type = STATIC;
            o.y = random(GROUND_HEIGHT + 30, SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT - 30);
        } else if (gameState.level < 5) { // Levels 3-4: Introduce moving obstacles
            if (obstacleTypeRoll < 70) {
                o.type = STATIC;
                o.y = random(GROUND_HEIGHT + 30, SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT - 30);
            } else {
                o.type = MOVING;
                o.y = random(GROUND_HEIGHT + 30 + MOVING_OBSTACLE_AMPLITUDE, SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT - 30 - MOVING_OBSTACLE_AMPLITUDE);
            }
        } else { // Level 5+: Introduce passages
            if (obstacleTypeRoll < 50) {
                o.type = STATIC;
                o.y = random(GROUND_HEIGHT + 30, SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT - 30);
            } else if (obstacleTypeRoll < 80) {
                o.type = MOVING;
                o.y = random(GROUND_HEIGHT + 30 + MOVING_OBSTACLE_AMPLITUDE, SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT - 30 - MOVING_OBSTACLE_AMPLITUDE);
            } else {
                // Find another free slot for the pair
                int free_slot2 = -1;
                for(int i = free_slot + 1; i < 5; ++i) {
                    if(!obstacles[i].active) {
                        free_slot2 = i;
                        break;
                    }
                }
                if (free_slot2 != -1) {
                    Obstacle& o2 = obstacles[free_slot2];
                    int passage_y = random(GROUND_HEIGHT + 50, SCREEN_HEIGHT - GROUND_HEIGHT - 50 - PASSAGE_GAP);

                    o.type = PASSAGE_TOP;
                    o.y = passage_y - o.height;

                    o2.active = true;
                    o2.x = SCREEN_WIDTH;
                    o2.width = OBSTACLE_WIDTH;
                    o2.height = OBSTACLE_HEIGHT;
                    o2.param1 = 0;
                    o2.type = PASSAGE_BOTTOM;
                    o2.y = passage_y + PASSAGE_GAP;
                } else {
                  // Not enough space for a passage, spawn a static one instead
                  o.type = STATIC;
                  o.y = random(GROUND_HEIGHT + 30, SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT - 30);
                }
            }
        }
    }
  }

  // === Obstacle Updating ===
  // This section updates the position of all active obstacles.
  for (int i = 0; i < 5; i++) {
    if (obstacles[i].active) {
      obstacles[i].x -= GAME_SPEED + (gameState.level - 1) * 1.5f;

      switch(obstacles[i].type) {
        case STATIC:
        case PASSAGE_TOP:
        case PASSAGE_BOTTOM:
          // No special movement
          break;
        case MOVING:
          obstacles[i].param1 += 0.05f * (1 + gameState.level * 0.1f); // increase speed with level
          obstacles[i].y += sin(obstacles[i].param1) * 2.0f;
          break;
        case BONUS:
          // To be implemented
          break;
      }

      // Deactivate if off-screen
      if (obstacles[i].x < -OBSTACLE_WIDTH) {
        obstacles[i].active = false;
        if (obstacles[i].type != PASSAGE_TOP && obstacles[i].type != PASSAGE_BOTTOM) {
            gameState.score += 10;
        } else {
            // Give more points for clearing a passage
            if (obstacles[i].type == PASSAGE_BOTTOM) {
                gameState.score += 25;
            }
        }
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
    // Erase previous obstacle
    if (gameState.prevObstaclesActive[i]) {
        // A bit of extra padding to erase circular moving obstacles
      M5.Display.fillRect(gameState.prevObstaclesX[i] - 5, gameState.prevObstaclesY[i] - 5,
                          OBSTACLE_WIDTH + 10, OBSTACLE_HEIGHT + 10,
                          BACKGROUND_COLOR);
    }
    
    if (obstacles[i].active) {
      auto& o = obstacles[i];
      switch(o.type) {
        case STATIC:
        case PASSAGE_TOP:
        case PASSAGE_BOTTOM:
          M5.Display.fillRect(o.x, o.y, o.width, o.height, OBSTACLE_COLOR);
          M5.Display.drawRect(o.x, o.y, o.width, o.height, TFT_DARKGREEN);
          break;
        case MOVING:
          M5.Display.fillCircle(o.x + o.width/2, o.y + o.height/2, o.width/2, OBSTACLE_MOVING_COLOR);
          M5.Display.drawCircle(o.x + o.width/2, o.y + o.height/2, o.width/2, TFT_RED);
          break;
        case BONUS:
          M5.Display.fillCircle(o.x + o.width/2, o.y + o.height/2, o.width/2, BONUS_COLOR);
          break;
      }
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
      M5.Display.setCursor(SCREEN_WIDTH/2, 10);
      M5.Display.printf("LEVEL %d!", gameState.level);
      M5.Display.setTextSize(UI_FONT_SIZE);
      M5.Display.setTextColor(TEXT_COLOR);
    } else {
      gameState.showLevelUp = false;
    }
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

#include <M5Unified.h>
#include "balloon_game.h"
#include "game_config.h"

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  
  initGame();
  
  M5.Display.fillScreen(TITLE_COLOR);
  M5.Display.setTextSize(TITLE_FONT_SIZE);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setCursor(SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 30);
  M5.Display.printf("Balloon Jump");
  
  M5.Display.setTextSize(UI_FONT_SIZE);
  M5.Display.setCursor(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
  M5.Display.printf("Blow to jump!");
  
  M5.Display.setCursor(SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 30);
  M5.Display.printf("Touch to start");
  
  M5.Display.display();
  
  while (true) {
    M5.update();
    if (M5.Touch.getCount() && M5.Touch.getDetail(0).wasClicked()) {
      break;
    }
    delay(100);
  }
  
  M5.Display.fillScreen(BACKGROUND_COLOR);
  M5.Display.display();
}

void loop() {
  M5.update();
  
  updateBlow();
  updateBalloon();
  updateObstacles();
  
  if (checkCollision()) {
    gameOver();
    return;
  }
  
  if (gameState.isBurst) {
    if (millis() - gameState.burstTime > 150) {
      gameOver();
      return;
    }
  }
  
  drawGame();
  
  delay(25);
}

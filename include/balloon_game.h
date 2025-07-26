#ifndef BALLOON_GAME_H
#define BALLOON_GAME_H

#include "game_config.h"

struct GameState {
  int balloonY;
  int balloonVelocity;
  int score;
  int height;
  bool isBlowing;
  bool isBurst;
  unsigned long blowStartTime;
  unsigned long lastObstacleTime;
  unsigned long burstTime;
  int16_t micData[RECORD_LENGTH];
  int prevBalloonY;
  int prevUIY;
  int prevObstaclesX[5];
  int prevObstaclesY[5];
  bool prevObstaclesActive[5];
  int16_t prevWaveY[RECORD_LENGTH];
  int16_t prevWaveH[RECORD_LENGTH];
  int level;
  int nextLevelHeight;
  bool showLevelUp;
  unsigned long levelUpTime;
};

extern GameState gameState;
extern struct Obstacle obstacles[5];

void initGame();
bool detectBlow();
void updateBalloon();
void updateObstacles();
bool checkCollision();
void drawGame();
void drawWaveform();
void gameOver();

#endif

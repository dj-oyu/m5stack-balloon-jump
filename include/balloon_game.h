#ifndef BALLOON_GAME_H
#define BALLOON_GAME_H

#include "game_config.h"

enum ObstacleType {
  STATIC,
  MOVING,
  PASSAGE_TOP,
  PASSAGE_BOTTOM,
  BONUS
};

struct Obstacle {
  int x;
  int y;
  int width;
  int height;
  bool active;
  ObstacleType type;
  float param1; // Used for sine wave movement angle, etc.
};

struct GameState {
  int balloonY;
  int balloonVelocity;
  int score;
  int height;
  int blowPower;
  bool isBurst;
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
extern Obstacle obstacles[5];

void initGame();
void updateBlow();
void updateBalloon();
void updateObstacles();
bool checkCollision();
void drawGame();
void drawWaveform();
void gameOver();

#endif

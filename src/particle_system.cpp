#include "particle_system.h"
#include "game_config.h"
#include <M5Unified.h>

// Particle pool
Particle particles[MAX_PARTICLES];

void initParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].life = 0;  // Inactive
  }
}

void spawnParticle(int16_t x, int16_t y, int8_t vx, int8_t vy,
                   uint8_t life, uint16_t color, uint8_t size) {
  // Find an inactive particle slot
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].life == 0) {
      particles[i].x = x;
      particles[i].y = y;
      particles[i].vx = vx;
      particles[i].vy = vy;
      particles[i].life = life;
      particles[i].maxLife = life;
      particles[i].color = color;
      particles[i].size = size;
      return;
    }
  }
  // If no slots available, particle is dropped (graceful degradation)
}

void updateParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].life > 0) {
      // Update position
      particles[i].x += particles[i].vx;
      particles[i].y += particles[i].vy;

      // Simple gravity (for some particle types)
      if (particles[i].vy < 10) {
        particles[i].vy += 1;  // Gravity
      }

      // Decrease lifetime
      particles[i].life--;

      // Bounds checking - deactivate if off-screen
      if (particles[i].x < -10 || particles[i].x > SCREEN_WIDTH + 10 ||
          particles[i].y < -10 || particles[i].y > SCREEN_HEIGHT + 10) {
        particles[i].life = 0;
      }
    }
  }
}

void drawParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].life > 0) {
      // Fade effect based on remaining life
      uint8_t alpha = (particles[i].life * 255) / particles[i].maxLife;

      // For retro look, use simple shapes
      if (particles[i].size == 1) {
        // Single pixel for small particles
        M5.Display.drawPixel(particles[i].x, particles[i].y, particles[i].color);
      } else {
        // Small filled rectangle for larger particles
        // Adjust brightness based on life for fade effect
        if (alpha > 128) {
          M5.Display.fillRect(particles[i].x, particles[i].y,
                              particles[i].size, particles[i].size,
                              particles[i].color);
        } else if (alpha > 64) {
          // Draw smaller when fading
          M5.Display.fillRect(particles[i].x, particles[i].y,
                              particles[i].size - 1, particles[i].size - 1,
                              particles[i].color);
        } else {
          // Just a pixel when very faded
          M5.Display.drawPixel(particles[i].x, particles[i].y, particles[i].color);
        }
      }
    }
  }
}

// Emit small puff when balloon jumps/blows
void emitJumpParticles(int x, int y) {
  // Only 3-5 particles for performance
  int count = random(3, 6);
  for (int i = 0; i < count; i++) {
    int8_t vx = random(-3, 4);
    int8_t vy = random(-5, -2);  // Upward bias
    uint16_t color = TFT_WHITE;
    spawnParticle(x, y, vx, vy, random(15, 30), color, 1);
  }
}

// Large explosion when balloon bursts
void emitBurstParticles(int x, int y) {
  // 10-15 particles radiating outward
  int count = random(10, 16);
  for (int i = 0; i < count; i++) {
    // Radial distribution
    int8_t vx = random(-8, 9);
    int8_t vy = random(-8, 9);
    uint16_t color;

    // Mix of yellow, orange, red for explosion
    int colorChoice = random(0, 3);
    if (colorChoice == 0) color = TFT_YELLOW;
    else if (colorChoice == 1) color = TFT_ORANGE;
    else color = TFT_RED;

    spawnParticle(x + random(-5, 6), y + random(-5, 6),
                  vx, vy, random(20, 40), color, random(2, 4));
  }
}

// Celebration effect for level up
void emitLevelUpParticles(int x, int y) {
  // 12-18 colorful particles
  int count = random(12, 19);
  for (int i = 0; i < count; i++) {
    int8_t vx = random(-6, 7);
    int8_t vy = random(-10, -3);  // Upward like fireworks
    uint16_t color;

    // Rainbow colors for celebration
    int colorChoice = random(0, 6);
    switch(colorChoice) {
      case 0: color = TFT_RED; break;
      case 1: color = TFT_YELLOW; break;
      case 2: color = TFT_GREEN; break;
      case 3: color = TFT_CYAN; break;
      case 4: color = TFT_BLUE; break;
      default: color = TFT_MAGENTA; break;
    }

    spawnParticle(x + random(-10, 11), y + random(-10, 11),
                  vx, vy, random(30, 50), color, random(2, 4));
  }
}

// Subtle sparkle when passing obstacles
void emitSparkleParticles(int x, int y) {
  // Just 2-4 particles
  int count = random(2, 5);
  for (int i = 0; i < count; i++) {
    int8_t vx = random(-2, 3);
    int8_t vy = random(-3, 2);
    uint16_t color = TFT_YELLOW;
    spawnParticle(x, y, vx, vy, random(10, 20), color, 1);
  }
}

// Impact particles on collision
void emitCollisionParticles(int x, int y) {
  // 6-8 particles at collision point
  int count = random(6, 9);
  for (int i = 0; i < count; i++) {
    int8_t vx = random(-5, 6);
    int8_t vy = random(-5, 6);
    uint16_t color = TFT_WHITE;
    spawnParticle(x, y, vx, vy, random(15, 25), color, 2);
  }
}

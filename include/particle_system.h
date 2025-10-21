#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <stdint.h>

// Maximum number of particles (conservative for IoT devices)
#define MAX_PARTICLES 50

// Particle types for different effects
enum ParticleType {
  PARTICLE_SPARKLE,    // Small bright dots
  PARTICLE_EXPLOSION,  // Burst outward
  PARTICLE_SMOKE,      // Upward drift
  PARTICLE_CONFETTI    // Celebration
};

// Lightweight particle structure (9 bytes per particle)
struct Particle {
  int16_t x;          // X position (2 bytes)
  int16_t y;          // Y position (2 bytes)
  int8_t vx;          // X velocity (1 byte, -128 to 127)
  int8_t vy;          // Y velocity (1 byte, -128 to 127)
  uint8_t life;       // Remaining life (1 byte, 0-255)
  uint8_t maxLife;    // Initial life for fade calculation (1 byte)
  uint16_t color;     // 16-bit color (2 bytes)
  uint8_t size;       // Particle size in pixels (1 byte, 1-4)
};

// Particle system state
extern Particle particles[MAX_PARTICLES];

// Initialize particle system
void initParticles();

// Update all particles (physics and lifetime)
void updateParticles();

// Draw all active particles
void drawParticles();

// Emit particles for various effects
void emitJumpParticles(int x, int y);              // Small puff when jumping
void emitBurstParticles(int x, int y);             // Explosion on burst
void emitLevelUpParticles(int x, int y);           // Celebration effect
void emitSparkleParticles(int x, int y);           // Subtle sparkle
void emitCollisionParticles(int x, int y);         // Impact effect

// Helper function to spawn a single particle
void spawnParticle(int16_t x, int16_t y, int8_t vx, int8_t vy,
                   uint8_t life, uint16_t color, uint8_t size);

#endif

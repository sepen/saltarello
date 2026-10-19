#ifndef SALTARELLO_GAME_H
#define SALTARELLO_GAME_H

// Game state and rules. This module does not depend on SDL, so it can be
// tested and changed without touching rendering, audio or input handling.

#include <cstdint>
#include <vector>

// Constants for game settings
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int GROUND_HEIGHT = 100;
const int GROUND_Y = SCREEN_HEIGHT - GROUND_HEIGHT;
const int PLAYER_WIDTH = 40;
const int PLAYER_HEIGHT = 40;
const int OBSTACLE_WIDTH = 30;
const int OBSTACLE_HEIGHT = 60;
const int STONE_HEIGHT = 30;
const int BIRD_WIDTH = 20;
const int BIRD_HEIGHT = 20;
const int GRAVITY = 1;
const int JUMP_STRENGTH = 18;
const int MOVE_SPEED = 5;
const int BULLET_WIDTH = 10;
const int BULLET_HEIGHT = 5;
const int BULLET_SPEED = 8;
const uint32_t FIRE_RATE = 1000;
const int BASE_OBSTACLE_SPEED = 5;
const int BIRD_MIN_SCORE = 10;

// Struct representing the player character
struct Saltarello {
    int x, y;
    int velocityY;
    bool isJumping;
};

// Struct representing obstacles in the game
struct Obstacle {
    int x, y;
    bool active;
    bool isStone;
};

// Struct representing bullets fired by the player
struct Bullet {
    int x, y;
};

// Full game state, so a restart is a single assignment
struct GameState {
    Saltarello player;
    std::vector<Obstacle> obstacles;
    std::vector<Bullet> bullets;
    Obstacle bird;
    int score;
    int obstacleSpeed;
    bool gameOver;
    uint32_t lastFireTime;
    bool fireReleased;
};

// Keys held during the current frame
struct Input {
    bool up, down, left, right, fire;
};

// What happened during a frame, so the caller can play sounds
struct GameEvents {
    bool jumped;
    bool shot;
    bool crashed;
};

int ObstacleHeight(const Obstacle& obs);

GameState NewGame();

// Advances the game one frame. `now` is the current time in milliseconds.
GameEvents UpdateGame(GameState& game, const Input& input, uint32_t now);

#endif

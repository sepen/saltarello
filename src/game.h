#ifndef SALTARELLO_GAME_H
#define SALTARELLO_GAME_H

// Game state and rules. This module does not depend on SDL, so it can be
// tested and changed without touching rendering, audio or input handling.

#include <vector>

// Constants for game settings
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int GROUND_HEIGHT = 100;
const int GROUND_Y = SCREEN_HEIGHT - GROUND_HEIGHT;
const int PLAYER_WIDTH = 40;
const int PLAYER_HEIGHT = 40;
const int DUCK_HEIGHT = 20;  // player height while holding DOWN on the ground
const int OBSTACLE_WIDTH = 30;
const int OBSTACLE_HEIGHT = 60;
const int STONE_HEIGHT = 30;
const int BIRD_WIDTH = 20;
const int BIRD_HEIGHT = 20;
const int BIRD_Y = GROUND_Y - 50;  // head height: hits a standing player, clears a ducking one
const int GRAVITY = 1;
const int JUMP_STRENGTH = 18;
const int MOVE_SPEED = 5;
const int BULLET_WIDTH = 10;
const int BULLET_HEIGHT = 5;
const int BULLET_SPEED = 8;
const int TICKS_PER_SECOND = 60;     // UpdateGame() runs at this fixed rate
const int FIRE_COOLDOWN_TICKS = 60;  // 1 s between shots
const int BASE_OBSTACLE_SPEED = 5;
const int SPAWN_SPREAD = 300;  // random extra distance when an obstacle respawns
const int BIRD_MIN_SCORE = 10;

struct Rect {
    int x, y, w, h;
};

// Struct representing the player character. (x, y) is the top-left corner
// of the standing player; use PlayerBox() for the current hitbox.
struct Saltarello {
    int x, y;
    int velocityY;
    bool isJumping;
    bool isDucking;
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
    int fireCooldown; // ticks until the next shot is allowed
    bool fireReleased;
};

// Keys held during the current tick
struct Input {
    bool up, down, left, right, fire;
};

// What happened during a tick, so the caller can play sounds
struct GameEvents {
    bool jumped;
    bool shot;
    bool crashed;
};

int ObstacleHeight(const Obstacle& obs);

// Minimum free space between two obstacles at a given speed: enough to jump
// over one, land and jump again before the next one arrives
int MinObstacleGap(int obstacleSpeed);

// The player's current hitbox (shorter while ducking, feet stay on the ground)
Rect PlayerBox(const Saltarello& player);

GameState NewGame();

// Advances the game one tick (1 / TICKS_PER_SECOND seconds)
GameEvents UpdateGame(GameState& game, const Input& input);

#endif

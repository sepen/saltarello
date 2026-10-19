#ifndef SALTARELLO_GAME_H
#define SALTARELLO_GAME_H

// Game state and rules. This module does not depend on SDL, so it can be
// tested and changed without touching rendering, audio or input handling.

#include <vector>

// Constants for game settings, in world units. The world is 4:3 so it fills
// a fullscreen window; the window scales it to fit the display.
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int GROUND_HEIGHT = 100;
const int GROUND_Y = SCREEN_HEIGHT - GROUND_HEIGHT;
const int PLAYER_X = 100;  // where the chicken starts
const int PLAYER_WIDTH = 80;
const int PLAYER_HEIGHT = 80;
// Holding DOWN on the ground squashes the chicken: shorter and a bit wider
const int DUCK_WIDTH = 104;
const int DUCK_HEIGHT = 40;
const int OBSTACLE_WIDTH = 60;
const int TREE_HEIGHT = 120;
const int POOP_HEIGHT = 60;
const int PIG_WIDTH = 72;
const int PIG_HEIGHT = 48;
// Flying pig at head height: its belly (GROUND_Y - 60) hits a standing
// chicken and clears a ducking one
const int PIG_Y = GROUND_Y - 60 - PIG_HEIGHT;
const int PIG_SLOWDOWN = 6;  // the pig flies this much slower than the obstacles
const int GRAVITY = 2;
const int JUMP_STRENGTH = 36;  // a full jump lasts 2 * 36 / 2 = 36 ticks, 342 high
const int JUMP_CUT_SPEED = 16;  // releasing UP caps the rising speed: short hop
const int JUMP_BUFFER_TICKS = 6;  // a press this close to landing still jumps (100 ms)
const int MOVE_SPEED = 10;
const int BULLET_WIDTH = 20;
const int BULLET_HEIGHT = 16;  // bullets are eggs
const int BULLET_SPEED = 16;
const int TICKS_PER_SECOND = 60;     // UpdateGame() runs at this fixed rate
const int FIRE_COOLDOWN_TICKS = 60;  // 1 s between shots
// Obstacles cross the screen in ~1.9 s at the start; +1 every 10 points
const int BASE_OBSTACLE_SPEED = 7;
const int SPAWN_SPREAD = 600;  // random extra distance when an obstacle respawns
const int PIG_MIN_SCORE = 10;

struct Rect {
    int x, y, w, h;
};

// Struct representing the player character, a chicken. (x, y) is the
// top-left corner of the standing chicken; use PlayerBox() for its bounds.
struct Saltarello {
    int x, y;
    int velocityY;
    bool isJumping;
    bool isDucking;
};

// Struct representing obstacles in the game: a smiling tree (tall) or a
// smiling poop (short)
struct Obstacle {
    int x, y;
    bool active;
    bool isPoop;
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
    Obstacle pig;
    int score;
    int obstacleSpeed;
    bool gameOver;
    int fireCooldown; // ticks until the next shot is allowed
    bool fireReleased;
    bool upReleased;
    int jumpBuffer; // ticks left to perform a buffered jump
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

// Collision shape of an obstacle, following its drawing: crown + trunk for
// a tree, base + upper tiers for a poop
struct Hitbox {
    Rect parts[2];
    int count;
};
Hitbox ObstacleHitbox(const Obstacle& obs);
bool PlayerHits(const Saltarello& player, const Hitbox& hitbox);

// Collision rectangle of the flying pig: its body, without the empty
// corners of the sprite
Rect PigHitbox(const Obstacle& pig);

// Minimum free space between two obstacles at a given speed: enough to jump
// over one, land and jump again before the next one arrives
int MinObstacleGap(int obstacleSpeed);

// Bounding box of the chicken: squashed while ducking, centered on the same
// spot and resting on the ground
Rect PlayerBox(const Saltarello& player);

// Whether the chicken (approximated by the ellipse inside PlayerBox)
// touches the rectangle
bool PlayerHits(const Saltarello& player, const Rect& r);

GameState NewGame();

// Advances the game one tick (1 / TICKS_PER_SECOND seconds)
GameEvents UpdateGame(GameState& game, const Input& input);

#endif

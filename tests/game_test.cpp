// Unit tests for the game rules (src/game.cpp). No SDL needed.

#include "game.h"

#include <cstdio>

static int failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::fprintf(stderr, "%s:%d: CHECK(%s) failed\n",           \
                         __FILE__, __LINE__, #cond);                    \
            failures++;                                                 \
        }                                                               \
    } while (0)

static const Input NO_KEYS = {false, false, false, false, false};
static const Input DOWN = {false, true, false, false, false};
static const Input UP = {true, false, false, false, false};

// A game with no obstacles, the bird active and right on top of the player
static GameState GameWithBirdOnPlayer() {
    GameState game = NewGame();
    game.obstacles.clear();
    game.score = BIRD_MIN_SCORE;
    game.bird.x = game.player.x + 5;
    return game;
}

static void TestStandingPlayerHitsBird() {
    GameState game = GameWithBirdOnPlayer();
    UpdateGame(game, NO_KEYS);
    CHECK(game.gameOver);
}

static void TestDuckingPlayerClearsBird() {
    GameState game = GameWithBirdOnPlayer();
    for (int i = 0; i < 5; i++) UpdateGame(game, DOWN);
    CHECK(game.player.isDucking);
    CHECK(!game.gameOver);
}

static void TestDuckingShrinksHitboxKeepingFeetOnGround() {
    GameState game = NewGame();
    UpdateGame(game, DOWN);
    Rect box = PlayerBox(game.player);
    CHECK(box.h == DUCK_HEIGHT);
    CHECK(box.y + box.h == GROUND_Y);
}

static void TestCannotDuckInTheAir() {
    GameState game = NewGame();
    game.obstacles.clear();
    UpdateGame(game, UP);
    UpdateGame(game, DOWN);
    CHECK(game.player.isJumping);
    CHECK(!game.player.isDucking);
}

static void TestJumpClearsBird() {
    GameState game = NewGame();
    game.obstacles.clear();
    game.score = BIRD_MIN_SCORE;
    UpdateGame(game, UP);
    for (int i = 0; i < 10; i++) UpdateGame(game, NO_KEYS);
    game.bird.x = game.player.x + 5;
    UpdateGame(game, NO_KEYS);
    CHECK(game.player.isJumping);
    CHECK(!game.gameOver);
}

static void TestDuckingDoesNotAvoidGroundObstacles() {
    GameState game = NewGame();
    game.obstacles = {{game.player.x + 5 + BASE_OBSTACLE_SPEED, GROUND_Y - STONE_HEIGHT, true, true}};
    UpdateGame(game, DOWN);
    CHECK(game.gameOver);
}

int main() {
    TestStandingPlayerHitsBird();
    TestDuckingPlayerClearsBird();
    TestDuckingShrinksHitboxKeepingFeetOnGround();
    TestCannotDuckInTheAir();
    TestJumpClearsBird();
    TestDuckingDoesNotAvoidGroundObstacles();

    if (failures) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("All game tests passed\n");
    return 0;
}

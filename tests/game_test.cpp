// Unit tests for the game rules (src/game.cpp). No SDL needed.

#include "game.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

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

static void TestMinObstacleGapGrowsWithSpeed() {
    // A jump lasts 2 * 18 / 1 = 36 ticks
    CHECK(MinObstacleGap(5) == 36 * 5 + PLAYER_WIDTH);
    CHECK(MinObstacleGap(15) > MinObstacleGap(5));
}

// Runs many ticks, shooting all the time so obstacles respawn both after
// leaving the screen and after being destroyed, and checks every gap.
static void TestObstaclesKeepMinimumGap() {
    const Input FIRE = {false, false, false, false, true};
    const Input NONE = NO_KEYS;
    for (unsigned seed = 1; seed <= 20; seed++) {
        std::srand(seed);
        GameState game = NewGame();
        bool ok = true;
        for (int tick = 0; tick < 20000 && ok; tick++) {
            // Keep the player out of the way and the speed constant
            game.player.x = -1000;
            game.gameOver = false;
            game.score = 0;
            UpdateGame(game, tick % 2 ? FIRE : NONE);

            std::vector<int> xs;
            for (const auto& obs : game.obstacles) xs.push_back(obs.x);
            std::sort(xs.begin(), xs.end());
            for (size_t i = 1; i < xs.size(); i++) {
                if (xs[i] - (xs[i - 1] + OBSTACLE_WIDTH) < MinObstacleGap(BASE_OBSTACLE_SPEED)) {
                    std::fprintf(stderr, "seed %u tick %d: gap %d\n", seed, tick,
                                 xs[i] - (xs[i - 1] + OBSTACLE_WIDTH));
                    ok = false;
                }
            }
        }
        CHECK(ok);
    }
}

int main() {
    TestStandingPlayerHitsBird();
    TestDuckingPlayerClearsBird();
    TestDuckingShrinksHitboxKeepingFeetOnGround();
    TestCannotDuckInTheAir();
    TestJumpClearsBird();
    TestDuckingDoesNotAvoidGroundObstacles();
    TestMinObstacleGapGrowsWithSpeed();
    TestObstaclesKeepMinimumGap();

    if (failures) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("All game tests passed\n");
    return 0;
}

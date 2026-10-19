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

// A game with no obstacles, the pig active and right on top of the player
static GameState GameWithPigOnPlayer() {
    GameState game = NewGame();
    game.obstacles.clear();
    game.score = PIG_MIN_SCORE;
    game.pig.x = game.player.x + 5;
    return game;
}

static void TestStandingPlayerHitsPig() {
    GameState game = GameWithPigOnPlayer();
    UpdateGame(game, NO_KEYS);
    CHECK(game.gameOver);
}

static void TestDuckingPlayerClearsPig() {
    GameState game = GameWithPigOnPlayer();
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

static void TestJumpClearsPig() {
    GameState game = NewGame();
    game.obstacles.clear();
    game.score = PIG_MIN_SCORE;
    // Full jump: UP held
    for (int i = 0; i < 11; i++) UpdateGame(game, UP);
    game.pig.x = game.player.x + 5;
    UpdateGame(game, UP);
    CHECK(game.player.isJumping);
    CHECK(!game.gameOver);
}

static void TestDuckingDoesNotAvoidGroundObstacles() {
    GameState game = NewGame();
    game.obstacles = {{game.player.x + 5 + BASE_OBSTACLE_SPEED, GROUND_Y - POOP_HEIGHT, true, true}};
    UpdateGame(game, DOWN);
    CHECK(game.gameOver);
}

static void TestMinObstacleGapGrowsWithSpeed() {
    // A jump lasts 2 * JUMP_STRENGTH / GRAVITY = 36 ticks
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

// Highest point (pixels above the ground) of a jump holding UP for `holdTicks`
static int JumpPeak(int holdTicks) {
    GameState game = NewGame();
    game.obstacles.clear();
    const int ground = game.player.y;
    int peak = 0;
    for (int tick = 0; tick < 200; tick++) {
        UpdateGame(game, tick < holdTicks ? UP : NO_KEYS);
        peak = std::max(peak, ground - game.player.y);
        if (tick > 0 && !game.player.isJumping) break;
    }
    return peak;
}

static void TestVariableJumpHeight() {
    int tap = JumpPeak(1);
    int held = JumpPeak(40);
    CHECK(tap < held);
    CHECK(JumpPeak(6) > tap);
    CHECK(JumpPeak(6) < held);
    // A tap clears a poop but not a tall obstacle
    CHECK(tap > POOP_HEIGHT);
    CHECK(tap < TREE_HEIGHT);
    // Holding UP keeps the full jump: 36 ticks, 342 high
    CHECK(held == 342);
}

// Jumps with a 1-tick tap, then presses UP once more `pressBeforeLanding`
// ticks before the first jump lands (or never, if negative) and keeps
// playing a few ticks past landing. Returns how many jumps started.
static int JumpsWithSecondPress(int pressBeforeLanding) {
    // Find out how many ticks the tap jump lasts
    GameState ref = NewGame();
    ref.obstacles.clear();
    int landing = 0;
    UpdateGame(ref, UP);
    while (ref.player.isJumping) {
        UpdateGame(ref, NO_KEYS);
        landing++;
    }

    GameState game = NewGame();
    game.obstacles.clear();
    int jumps = UpdateGame(game, UP).jumped ? 1 : 0;
    for (int i = 1; i <= landing + 3; i++) {
        bool press = pressBeforeLanding >= 0 && i == landing - pressBeforeLanding;
        if (UpdateGame(game, press ? UP : NO_KEYS).jumped) jumps++;
    }
    return jumps;
}

static void TestJumpBuffering() {
    CHECK(JumpsWithSecondPress(-1) == 1);
    // Pressed slightly before landing: jumps again on landing
    CHECK(JumpsWithSecondPress(3) == 2);
    CHECK(JumpsWithSecondPress(JUMP_BUFFER_TICKS - 1) == 2);
    // Pressed too early: the press is forgotten
    CHECK(JumpsWithSecondPress(JUMP_BUFFER_TICKS) == 1);
    CHECK(JumpsWithSecondPress(JUMP_BUFFER_TICKS + 5) == 1);
}

static void TestHoldingUpDoesNotRepeatJumps() {
    GameState game = NewGame();
    game.obstacles.clear();
    int jumps = 0;
    for (int tick = 0; tick < 120; tick++) {
        if (UpdateGame(game, UP).jumped) jumps++;
    }
    CHECK(jumps == 1);
}

static void TestChickenCollisionIgnoresBoxCorners() {
    Saltarello chicken = NewGame().player;
    Rect box = PlayerBox(chicken);
    // Overlaps the bounding box corner, but not the round body
    CHECK(!PlayerHits(chicken, Rect{box.x - 10, box.y - 10, 12, 12}));
    // Touches the top of the body
    CHECK(PlayerHits(chicken, Rect{box.x + box.w / 2 - 2, box.y - 10, 4, 12}));
}

static void TestSquashedChickenIsWiderAndCentered() {
    Saltarello chicken = NewGame().player;
    Rect standing = PlayerBox(chicken);
    chicken.isDucking = true;
    Rect squashed = PlayerBox(chicken);
    CHECK(squashed.w == DUCK_WIDTH);
    CHECK(squashed.w > standing.w);
    CHECK(squashed.x + squashed.w / 2 == standing.x + standing.w / 2);
}

static void TestSquashedChickenHitsWhatIsBesideIt() {
    Saltarello chicken = NewGame().player;
    // Low and just to the right of the standing chicken
    Rect beside = {chicken.x + PLAYER_WIDTH + 1, GROUND_Y - 10, 10, 10};
    CHECK(!PlayerHits(chicken, beside));
    chicken.isDucking = true;
    CHECK(PlayerHits(chicken, beside));
}

static void TestNothingBesideTheTreeTrunk() {
    Saltarello chicken = NewGame().player;
    chicken.isDucking = true;
    // A tree right in front of the squashed chicken: its low front edge sits
    // beside the trunk, under the crown, where the tree has no wood
    Obstacle tree = {chicken.x + PLAYER_WIDTH, GROUND_Y - TREE_HEIGHT, true, false};
    CHECK(!PlayerHits(chicken, ObstacleHitbox(tree)));
    // Moving closer reaches the trunk
    tree.x -= PLAYER_WIDTH * 3 / 10;
    CHECK(PlayerHits(chicken, ObstacleHitbox(tree)));
}

static const Input FIRE_KEY = {false, false, false, false, true};

// A game with nothing to hit or be hit by
static GameState EmptyGame() {
    GameState game = NewGame();
    game.obstacles.clear();
    return game;
}

// Presses X at the given ticks (releasing it in between) and runs `ticks`
// ticks in total. Returns how many eggs were thrown.
static int ThrowAt(GameState& game, std::vector<int> pressTicks, int ticks) {
    int thrown = 0;
    for (int t = 0; t < ticks; t++) {
        bool press = std::find(pressTicks.begin(), pressTicks.end(), t) != pressTicks.end();
        if (UpdateGame(game, press ? FIRE_KEY : NO_KEYS).shot) thrown++;
    }
    return thrown;
}

static void TestThrowingIsFree() {
    GameState game = EmptyGame();
    game.score = 5;
    CHECK(ThrowAt(game, {0}, 1) == 1);
    CHECK(game.score == 5);
    CHECK(game.eggs == MAX_EGGS - 1);
}

static void TestEggsRunOut() {
    GameState game = EmptyGame();
    // Four presses, well spaced but before any egg comes back
    CHECK(ThrowAt(game, {0, 20, 40, 60}, 70) == MAX_EGGS);
    CHECK(game.eggs == 0);
}

static void TestThrowCooldown() {
    GameState game = EmptyGame();
    CHECK(ThrowAt(game, {0, FIRE_COOLDOWN_TICKS - 5}, FIRE_COOLDOWN_TICKS) == 1);
}

static void TestEggsRecharge() {
    GameState game = EmptyGame();
    ThrowAt(game, {0}, 1);
    for (int i = 0; i < EGG_RECHARGE_TICKS - 1; i++) UpdateGame(game, NO_KEYS);
    CHECK(game.eggs == MAX_EGGS - 1);
    UpdateGame(game, NO_KEYS);
    CHECK(game.eggs == MAX_EGGS);
}

static void TestBreakingObstacleScores() {
    GameState game = NewGame();
    // A tree ahead and an egg about to hit it
    game.obstacles = {{400, GROUND_Y - TREE_HEIGHT, true, false}};
    game.bullets = {{395, GROUND_Y - 50}};
    UpdateGame(game, NO_KEYS);
    CHECK(game.bullets.empty());
    CHECK(game.score == 1);
    CHECK(game.obstacles[0].x > SCREEN_WIDTH);  // replaced by a new one
}

// A game where the pig is active and an egg is about to hit it
static GameState EggAboutToHitPig() {
    GameState game = EmptyGame();
    game.score = PIG_MIN_SCORE;
    game.pig.active = true;
    game.pig.x = 400;
    Rect body = PigHitbox(game.pig);
    game.bullets = {{body.x - BULLET_SPEED, body.y + body.h / 2}};
    return game;
}

static void TestEggHitsPig() {
    GameState game = EggAboutToHitPig();
    UpdateGame(game, NO_KEYS);
    CHECK(game.bullets.empty());
    CHECK(game.score == PIG_MIN_SCORE + 1);
    CHECK(game.pig.x >= SCREEN_WIDTH);  // a new pig comes later
}

static void TestEggIgnoresInactivePig() {
    GameState game = EggAboutToHitPig();
    game.score = 0;
    game.pig.active = false;
    UpdateGame(game, NO_KEYS);
    CHECK(game.bullets.size() == 1);
    CHECK(game.score == 0);
}

static void TestEggThrownStandingPassesUnderPig() {
    GameState game = EmptyGame();
    game.score = PIG_MIN_SCORE;
    game.pig.active = true;
    game.pig.x = game.player.x + PLAYER_WIDTH + 40;
    UpdateGame(game, FIRE_KEY);
    CHECK(game.bullets.size() == 1);
    Rect egg = {game.bullets[0].x, game.bullets[0].y, BULLET_WIDTH, BULLET_HEIGHT};
    Rect body = PigHitbox(game.pig);
    CHECK(egg.y >= body.y + body.h);  // below the pig's belly
}

int main() {
    TestStandingPlayerHitsPig();
    TestDuckingPlayerClearsPig();
    TestDuckingShrinksHitboxKeepingFeetOnGround();
    TestCannotDuckInTheAir();
    TestJumpClearsPig();
    TestDuckingDoesNotAvoidGroundObstacles();
    TestMinObstacleGapGrowsWithSpeed();
    TestObstaclesKeepMinimumGap();
    TestVariableJumpHeight();
    TestJumpBuffering();
    TestHoldingUpDoesNotRepeatJumps();
    TestChickenCollisionIgnoresBoxCorners();
    TestSquashedChickenIsWiderAndCentered();
    TestSquashedChickenHitsWhatIsBesideIt();
    TestNothingBesideTheTreeTrunk();
    TestThrowingIsFree();
    TestEggsRunOut();
    TestThrowCooldown();
    TestEggsRecharge();
    TestBreakingObstacleScores();
    TestEggHitsPig();
    TestEggIgnoresInactivePig();
    TestEggThrownStandingPassesUnderPig();

    if (failures) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("All game tests passed\n");
    return 0;
}

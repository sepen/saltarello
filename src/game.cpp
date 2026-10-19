#include "game.h"

#include <algorithm>
#include <cstdlib>

namespace {

Obstacle MakeObstacle(int x, bool isStone) {
    return {x, GROUND_Y - (isStone ? STONE_HEIGHT : OBSTACLE_HEIGHT), true, isStone};
}

bool Overlaps(const Rect& a, const Rect& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

Rect ObstacleBox(const Obstacle& obs) {
    return {obs.x, obs.y, OBSTACLE_WIDTH, ObstacleHeight(obs)};
}

// Respawns `obs` off-screen to the right, behind every other obstacle plus
// the minimum gap
void Respawn(Obstacle& obs, bool isStone, const std::vector<Obstacle>& obstacles, int speed) {
    int x = SCREEN_WIDTH + std::rand() % SPAWN_SPREAD;
    for (const auto& other : obstacles) {
        if (&other == &obs) continue;
        x = std::max(x, other.x + OBSTACLE_WIDTH + MinObstacleGap(speed));
    }
    obs = MakeObstacle(x, isStone);
}

} // namespace

int MinObstacleGap(int obstacleSpeed) {
    const int jumpTicks = 2 * JUMP_STRENGTH / GRAVITY;
    return jumpTicks * obstacleSpeed + PLAYER_WIDTH;
}

int ObstacleHeight(const Obstacle& obs) {
    return obs.isStone ? STONE_HEIGHT : OBSTACLE_HEIGHT;
}

Rect PlayerBox(const Saltarello& player) {
    if (player.isDucking) {
        return {player.x, player.y + PLAYER_HEIGHT - DUCK_HEIGHT, PLAYER_WIDTH, DUCK_HEIGHT};
    }
    return {player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT};
}

GameState NewGame() {
    GameState s;
    s.player = {50, GROUND_Y - PLAYER_HEIGHT, 0, false, false};
    s.obstacles = {
        MakeObstacle(SCREEN_WIDTH, false),
        MakeObstacle(SCREEN_WIDTH + 400, true),
    };
    s.bird = {SCREEN_WIDTH + 400, BIRD_Y, false, false};
    s.score = 0;
    s.obstacleSpeed = BASE_OBSTACLE_SPEED;
    s.gameOver = false;
    s.fireCooldown = 0;
    s.fireReleased = true;
    return s;
}

GameEvents UpdateGame(GameState& game, const Input& input) {
    GameEvents events = {false, false, false};
    if (game.gameOver) return events;

    Saltarello& player = game.player;
    std::vector<Obstacle>& obstacles = game.obstacles;
    std::vector<Bullet>& bullets = game.bullets;
    Obstacle& bird = game.bird;

    // Jump with UP key
    if (input.up && !player.isJumping) {
        player.velocityY = -JUMP_STRENGTH;
        player.isJumping = true;
        events.jumped = true;
    }

    // Duck with DOWN key, only while on the ground
    player.isDucking = input.down && !player.isJumping;

    // Move left
    if (input.left && player.x > 0) {
        player.x -= MOVE_SPEED;
    }

    // Move right
    if (input.right && player.x < SCREEN_WIDTH - PLAYER_WIDTH) {
        player.x += MOVE_SPEED;
    }

    // Fire bullets with X key (one shot per key press)
    if (game.fireCooldown > 0) game.fireCooldown--;
    if (input.fire) {
        if (game.fireReleased && game.fireCooldown == 0) {
            Rect box = PlayerBox(player);
            bullets.push_back({box.x + box.w, box.y + box.h / 2});
            events.shot = true;
            game.fireCooldown = FIRE_COOLDOWN_TICKS;
            game.score = std::max(0, game.score - 1);
        }
        game.fireReleased = false;
    } else {
        game.fireReleased = true;
    }

    // Update player position
    player.y += player.velocityY;
    player.velocityY += GRAVITY;
    if (player.y >= GROUND_Y - PLAYER_HEIGHT) {
        player.y = GROUND_Y - PLAYER_HEIGHT;
        player.velocityY = 0;
        player.isJumping = false;
    }

    // Update obstacle positions, then respawn and score the ones that left the screen
    for (auto& obs : obstacles) {
        obs.x -= game.obstacleSpeed;
    }
    for (auto& obs : obstacles) {
        if (obs.x + OBSTACLE_WIDTH < 0) {
            Respawn(obs, obs.isStone, obstacles, game.obstacleSpeed);
            game.score++;
        }
    }

    // Check for collision between player and obstacles
    for (const auto& obs : obstacles) {
        if (Overlaps(PlayerBox(player), ObstacleBox(obs))) {
            game.gameOver = true;
            break;
        }
    }

    // Move bullets and check for collision with obstacles
    for (auto it_bullet = bullets.begin(); it_bullet != bullets.end(); ) {
        it_bullet->x += BULLET_SPEED;
        bool bulletDestroyed = false;

        for (auto& obs : obstacles) {
            if (obs.active &&
                Overlaps({it_bullet->x, it_bullet->y, BULLET_WIDTH, BULLET_HEIGHT}, ObstacleBox(obs))) {
                obs.active = false;  // Only this obstacle is destroyed
                bulletDestroyed = true;
                break;
            }
        }

        if (bulletDestroyed || it_bullet->x > SCREEN_WIDTH) {
            it_bullet = bullets.erase(it_bullet);
        } else {
            ++it_bullet;
        }
    }

    // Replace destroyed obstacles so their number stays the same
    for (auto& obs : obstacles) {
        if (!obs.active) {
            Respawn(obs, std::rand() % 2 == 0, obstacles, game.obstacleSpeed);
        }
    }

    // Add bird obstacle after the score reaches BIRD_MIN_SCORE
    if (game.score >= BIRD_MIN_SCORE) {
        bird.active = true;
        bird.x -= std::max(1, game.obstacleSpeed - 4);

        if (bird.x + BIRD_WIDTH < 0) {
            bird.x = SCREEN_WIDTH;
        }

        // Duck under the bird or jump over it
        if (Overlaps(PlayerBox(player), {bird.x, bird.y, BIRD_WIDTH, BIRD_HEIGHT})) {
            game.gameOver = true;
        }
    }

    events.crashed = game.gameOver;

    // Increase obstacle speed every 10 points
    game.obstacleSpeed = BASE_OBSTACLE_SPEED + game.score / 10;

    return events;
}

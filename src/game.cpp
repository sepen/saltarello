#include "game.h"

#include <algorithm>
#include <cstdlib>

namespace {

Obstacle MakeObstacle(int x, bool isStone) {
    return {x, GROUND_Y - (isStone ? STONE_HEIGHT : OBSTACLE_HEIGHT), true, isStone};
}

bool Overlaps(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

} // namespace

int ObstacleHeight(const Obstacle& obs) {
    return obs.isStone ? STONE_HEIGHT : OBSTACLE_HEIGHT;
}

GameState NewGame() {
    GameState s;
    s.player = {50, GROUND_Y - PLAYER_HEIGHT, 0, false};
    s.obstacles = {
        MakeObstacle(SCREEN_WIDTH, false),
        MakeObstacle(SCREEN_WIDTH + 400, true),
    };
    s.bird = {SCREEN_WIDTH + 400, GROUND_Y - 120, false, false};
    s.score = 0;
    s.obstacleSpeed = BASE_OBSTACLE_SPEED;
    s.gameOver = false;
    s.lastFireTime = 0;
    s.fireReleased = true;
    return s;
}

GameEvents UpdateGame(GameState& game, const Input& input, uint32_t now) {
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

    // Move left
    if (input.left && player.x > 0) {
        player.x -= MOVE_SPEED;
    }

    // Move right
    if (input.right && player.x < SCREEN_WIDTH - PLAYER_WIDTH) {
        player.x += MOVE_SPEED;
    }

    // Fire bullets with X key (one shot per key press)
    if (input.fire) {
        if (game.fireReleased && now - game.lastFireTime >= FIRE_RATE) {
            bullets.push_back({player.x + PLAYER_WIDTH, player.y + PLAYER_HEIGHT / 2});
            events.shot = true;
            game.lastFireTime = now;
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

    // Update obstacle positions; respawn and score the ones that left the screen
    for (auto& obs : obstacles) {
        obs.x -= game.obstacleSpeed;
        if (obs.x + OBSTACLE_WIDTH < 0) {
            obs = MakeObstacle(SCREEN_WIDTH + (std::rand() % 300), obs.isStone);
            game.score++;
        }
    }

    // Check for collision between player and obstacles
    for (const auto& obs : obstacles) {
        if (Overlaps(player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT,
                     obs.x, obs.y, OBSTACLE_WIDTH, ObstacleHeight(obs))) {
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
                Overlaps(it_bullet->x, it_bullet->y, BULLET_WIDTH, BULLET_HEIGHT,
                         obs.x, obs.y, OBSTACLE_WIDTH, ObstacleHeight(obs))) {
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
            obs = MakeObstacle(SCREEN_WIDTH + (std::rand() % 300), std::rand() % 2 == 0);
        }
    }

    // Add bird obstacle after the score reaches BIRD_MIN_SCORE
    if (game.score >= BIRD_MIN_SCORE) {
        bird.active = true;
        bird.x -= std::max(1, game.obstacleSpeed - 4);

        if (bird.x + BIRD_WIDTH < 0) {
            bird.x = SCREEN_WIDTH;
        }

        // Holding DOWN ducks under the bird
        if (!input.down &&
            Overlaps(player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT,
                     bird.x, bird.y, BIRD_WIDTH, BIRD_HEIGHT)) {
            game.gameOver = true;
        }
    }

    events.crashed = game.gameOver;

    // Increase obstacle speed every 10 points
    game.obstacleSpeed = BASE_OBSTACLE_SPEED + game.score / 10;

    return events;
}

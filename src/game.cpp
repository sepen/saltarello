#include "game.h"

#include <algorithm>
#include <cstdlib>

namespace {

Obstacle MakeObstacle(int x, bool isPoop) {
    return {x, GROUND_Y - (isPoop ? POOP_HEIGHT : TREE_HEIGHT), true, isPoop};
}

bool Overlaps(const Rect& a, const Rect& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

bool Overlaps(const Rect& r, const Hitbox& hitbox) {
    for (int i = 0; i < hitbox.count; i++) {
        if (Overlaps(r, hitbox.parts[i])) return true;
    }
    return false;
}

// Respawns `obs` off-screen to the right, behind every other obstacle plus
// the minimum gap
void Respawn(Obstacle& obs, bool isPoop, const std::vector<Obstacle>& obstacles, int speed) {
    int x = SCREEN_WIDTH + std::rand() % SPAWN_SPREAD;
    for (const auto& other : obstacles) {
        if (&other == &obs) continue;
        x = std::max(x, other.x + OBSTACLE_WIDTH + MinObstacleGap(speed));
    }
    obs = MakeObstacle(x, isPoop);
}

} // namespace

int MinObstacleGap(int obstacleSpeed) {
    const int jumpTicks = 2 * JUMP_STRENGTH / GRAVITY;
    return jumpTicks * obstacleSpeed + PLAYER_WIDTH;
}

bool PlayerHits(const Saltarello& player, const Hitbox& hitbox) {
    for (int i = 0; i < hitbox.count; i++) {
        if (PlayerHits(player, hitbox.parts[i])) return true;
    }
    return false;
}

Hitbox ObstacleHitbox(const Obstacle& obs) {
    // Proportions of the drawings in sprites.cpp
    const int w = OBSTACLE_WIDTH;
    if (obs.isPoop) {
        // Wide base (lower 13/30) and narrower upper tiers (middle 2/3)
        const int h = POOP_HEIGHT;
        return {{{obs.x, obs.y + h * 17 / 30, w, h * 13 / 30},
                 {obs.x + w / 6, obs.y + h * 4 / 30, w * 2 / 3, h * 13 / 30}}, 2};
    }
    // Round crown (top 3/5) on a trunk (middle third)
    const int crown = TREE_HEIGHT * 3 / 5;
    return {{{obs.x, obs.y, w, crown}, {obs.x + w / 3, obs.y + crown, w / 3, TREE_HEIGHT - crown}}, 2};
}

Rect PigHitbox(const Obstacle& pig) {
    // The body, without the snout tip, tail, wing and legs in the corners
    return {pig.x + PIG_WIDTH / 12, pig.y + PIG_HEIGHT / 6, PIG_WIDTH * 5 / 6, PIG_HEIGHT * 17 / 24};
}

int ObstacleHeight(const Obstacle& obs) {
    return obs.isPoop ? POOP_HEIGHT : TREE_HEIGHT;
}

bool PlayerHits(const Saltarello& player, const Rect& r) {
    // Scale space so the ellipse becomes a unit circle (the rectangle stays a
    // rectangle), then test the rectangle's closest point to the center
    Rect box = PlayerBox(player);
    double a = box.w / 2.0, b = box.h / 2.0;
    double cx = box.x + a, cy = box.y + b;
    double px = std::min(std::max(cx, static_cast<double>(r.x)), static_cast<double>(r.x + r.w));
    double py = std::min(std::max(cy, static_cast<double>(r.y)), static_cast<double>(r.y + r.h));
    double dx = (px - cx) / a, dy = (py - cy) / b;
    return dx * dx + dy * dy < 1;
}

Rect PlayerBox(const Saltarello& player) {
    if (player.isDucking) {
        return {player.x - (DUCK_WIDTH - PLAYER_WIDTH) / 2, player.y + PLAYER_HEIGHT - DUCK_HEIGHT,
                DUCK_WIDTH, DUCK_HEIGHT};
    }
    return {player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT};
}

GameState NewGame() {
    GameState s;
    s.player = {PLAYER_X, GROUND_Y - PLAYER_HEIGHT, 0, false, false};
    s.obstacles = {
        MakeObstacle(SCREEN_WIDTH, false),
        MakeObstacle(SCREEN_WIDTH + 400, true),
    };
    s.pig = {SCREEN_WIDTH + 400, PIG_Y, false, false};
    s.score = 0;
    s.obstacleSpeed = BASE_OBSTACLE_SPEED;
    s.gameOver = false;
    s.fireCooldown = 0;
    s.eggs = MAX_EGGS;
    s.eggRecharge = 0;
    s.fireReleased = true;
    s.upReleased = true;
    s.jumpBuffer = 0;
    return s;
}

GameEvents UpdateGame(GameState& game, const Input& input) {
    GameEvents events = {false, false, false};
    if (game.gameOver) return events;

    Saltarello& player = game.player;
    std::vector<Obstacle>& obstacles = game.obstacles;
    std::vector<Bullet>& bullets = game.bullets;
    Obstacle& pig = game.pig;

    // Jump when UP is pressed. A press shortly before landing is buffered
    // and performed as soon as the player touches the ground.
    bool upPressed = input.up && game.upReleased;
    game.upReleased = !input.up;
    if (upPressed) game.jumpBuffer = JUMP_BUFFER_TICKS;
    if (game.jumpBuffer > 0) {
        if (!player.isJumping) {
            player.velocityY = -JUMP_STRENGTH;
            player.isJumping = true;
            events.jumped = true;
            game.jumpBuffer = 0;
        } else if (!upPressed) {
            game.jumpBuffer--; // the press tick itself does not count
        }
    }

    // Releasing UP while rising cuts the jump short (variable jump height)
    if (player.isJumping && !input.up && player.velocityY < -JUMP_CUT_SPEED) {
        player.velocityY = -JUMP_CUT_SPEED;
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

    // Recharge eggs, one at a time
    if (game.eggs < MAX_EGGS && ++game.eggRecharge >= EGG_RECHARGE_TICKS) {
        game.eggs++;
        game.eggRecharge = 0;
    }

    // Throw an egg with X key (one per key press)
    if (game.fireCooldown > 0) game.fireCooldown--;
    if (input.fire) {
        if (game.fireReleased && game.fireCooldown == 0 && game.eggs > 0) {
            Rect box = PlayerBox(player);
            bullets.push_back({box.x + box.w, box.y + box.h / 2 - BULLET_HEIGHT / 2});
            events.shot = true;
            game.fireCooldown = FIRE_COOLDOWN_TICKS;
            game.eggs--;
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
            Respawn(obs, obs.isPoop, obstacles, game.obstacleSpeed);
            game.score++;
        }
    }

    // Check for collision between player and obstacles
    for (const auto& obs : obstacles) {
        if (PlayerHits(player, ObstacleHitbox(obs))) {
            game.gameOver = true;
            break;
        }
    }

    // Move bullets and check for collision with obstacles and the pig
    for (auto it_bullet = bullets.begin(); it_bullet != bullets.end(); ) {
        it_bullet->x += BULLET_SPEED;
        const Rect egg = {it_bullet->x, it_bullet->y, BULLET_WIDTH, BULLET_HEIGHT};
        bool bulletDestroyed = false;

        for (auto& obs : obstacles) {
            if (obs.active && Overlaps(egg, ObstacleHitbox(obs))) {
                obs.active = false;  // Only this obstacle is destroyed
                game.score++;        // same as dodging it
                bulletDestroyed = true;
                break;
            }
        }

        // The pig flies above the chicken's beak: it has to be hit mid-jump
        if (!bulletDestroyed && pig.active && Overlaps(egg, PigHitbox(pig))) {
            pig.x = SCREEN_WIDTH + std::rand() % SPAWN_SPREAD;  // a new pig, later
            game.score++;
            bulletDestroyed = true;
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

    // Add pig obstacle after the score reaches PIG_MIN_SCORE
    if (game.score >= PIG_MIN_SCORE) {
        pig.active = true;
        pig.x -= std::max(1, game.obstacleSpeed - PIG_SLOWDOWN);

        if (pig.x + PIG_WIDTH < 0) {
            pig.x = SCREEN_WIDTH;
        }

        // Duck under the pig or jump over it
        if (PlayerHits(player, PigHitbox(pig))) {
            game.gameOver = true;
        }
    }

    events.crashed = game.gameOver;

    // Increase obstacle speed every 10 points
    game.obstacleSpeed = BASE_OBSTACLE_SPEED + game.score / 10;

    return events;
}

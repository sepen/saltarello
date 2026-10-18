#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

#ifndef PKGDATADIR
#define PKGDATADIR ""
#endif

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
const Uint32 FIRE_RATE = 1000;
const int BASE_OBSTACLE_SPEED = 5;
const int BIRD_MIN_SCORE = 10;
const Uint32 FRAME_MS = 16; // ~60 FPS

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
    Uint32 lastFireTime;
};

int ObstacleHeight(const Obstacle& obs) {
    return obs.isStone ? STONE_HEIGHT : OBSTACLE_HEIGHT;
}

Obstacle MakeObstacle(int x, bool isStone) {
    return {x, GROUND_Y - (isStone ? STONE_HEIGHT : OBSTACLE_HEIGHT), true, isStone};
}

bool Overlaps(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
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
    return s;
}

// Resolve an asset path: next to the executable, then the current directory,
// then the install location ($(pkgdatadir)).
std::string AssetPath(const std::string& name) {
    std::vector<std::string> candidates;
    if (char* base = SDL_GetBasePath()) {
        candidates.push_back(std::string(base) + "assets/" + name);
        SDL_free(base);
    }
    candidates.push_back("assets/" + name);
    if (PKGDATADIR[0] != '\0') {
        candidates.push_back(std::string(PKGDATADIR) + "/assets/" + name);
    }
    for (const auto& path : candidates) {
        if (SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "rb")) {
            SDL_RWclose(rw);
            return path;
        }
    }
    return "assets/" + name;
}

// Function to render text on screen
void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color) {
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!textSurface) return;
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture) {
        SDL_Rect textRect = {x, y, textSurface->w, textSurface->h};
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_DestroyTexture(textTexture);
    }
    SDL_FreeSurface(textSurface);
}

// Plays a sound if audio is available (chunk may be NULL)
void PlaySound(Mix_Chunk* chunk) {
    if (chunk) Mix_PlayChannel(-1, chunk, 0);
}

// argc/argv are required so SDL_main works on Windows
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Initialize SDL and fonts; audio is optional
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    bool audioOk = SDL_InitSubSystem(SDL_INIT_AUDIO) == 0 &&
                   Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == 0;
    if (!audioOk) {
        std::cerr << "Audio disabled: " << Mix_GetError() << std::endl;
    }

    // Load sounds and music
    Mix_Chunk* jumpSound = nullptr;
    Mix_Chunk* collisionSound = nullptr;
    Mix_Chunk* shootSound = nullptr;
    Mix_Music* bgMusic = nullptr;
    if (audioOk) {
        jumpSound = Mix_LoadWAV(AssetPath("jump.wav").c_str());
        collisionSound = Mix_LoadWAV(AssetPath("collision.wav").c_str());
        shootSound = Mix_LoadWAV(AssetPath("shoot.wav").c_str());
        bgMusic = Mix_LoadMUS(AssetPath("background.mp3").c_str());
        // Set background music volume
        if (bgMusic) {
            Mix_PlayMusic(bgMusic, -1);
            Mix_VolumeMusic(MIX_MAX_VOLUME * 4 / 100);
        }
    }

    // Create SDL window and renderer (fall back to software rendering)
    SDL_Window* window = SDL_CreateWindow("Saltarello", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = nullptr;
    if (window) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    // Load font for rendering text
    TTF_Font* font = renderer ? TTF_OpenFont(AssetPath("font.ttf").c_str(), 24) : nullptr;

    if (!window || !renderer || !font) {
        if (!window || !renderer) {
            std::cerr << "Failed to create window/renderer: " << SDL_GetError() << std::endl;
        } else {
            std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        }
        Mix_FreeChunk(jumpSound);
        Mix_FreeChunk(collisionSound);
        Mix_FreeChunk(shootSound);
        Mix_FreeMusic(bgMusic);
        if (audioOk) Mix_CloseAudio();
        TTF_Quit();
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize game objects
    GameState game = NewGame();
    Saltarello& player = game.player;
    std::vector<Obstacle>& obstacles = game.obstacles;
    std::vector<Bullet>& bullets = game.bullets;
    Obstacle& bird = game.bird;

    bool quit = false;
    bool fireReleased = true;

    SDL_Event e;

    // Main game loop
    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();

        // Event handling
        while (SDL_PollEvent(&e)) {
            // Quit game on window close
            if (e.type == SDL_QUIT) quit = true;

            // Quit game on ESC key press
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) quit = true;

            // Restart game on ENTER key press after game over
            if (game.gameOver && e.type == SDL_KEYDOWN &&
                (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)) {
                game = NewGame();
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (!game.gameOver) {
            // Jump with UP key
            if (keys[SDL_SCANCODE_UP] && !player.isJumping) {
                player.velocityY = -JUMP_STRENGTH;
                player.isJumping = true;
                PlaySound(jumpSound);
            }

            // Move left
            if (keys[SDL_SCANCODE_LEFT] && player.x > 0) {
                player.x -= MOVE_SPEED;
            }

            // Move right
            if (keys[SDL_SCANCODE_RIGHT] && player.x < SCREEN_WIDTH - PLAYER_WIDTH) {
                player.x += MOVE_SPEED;
            }

            // Fire bullets with X key (one shot per key press)
            if (keys[SDL_SCANCODE_X]) {
                if (fireReleased && currentTime - game.lastFireTime >= FIRE_RATE) {
                    bullets.push_back({player.x + PLAYER_WIDTH, player.y + PLAYER_HEIGHT / 2});
                    PlaySound(shootSound);
                    game.lastFireTime = currentTime;
                    game.score = std::max(0, game.score - 1);
                }
                fireReleased = false;
            } else {
                fireReleased = true;
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
                if (!keys[SDL_SCANCODE_DOWN] &&
                    Overlaps(player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT,
                             bird.x, bird.y, BIRD_WIDTH, BIRD_HEIGHT)) {
                    game.gameOver = true;
                }
            }

            if (game.gameOver) PlaySound(collisionSound);

            // Increase obstacle speed every 10 points
            game.obstacleSpeed = BASE_OBSTACLE_SPEED + game.score / 10;
        }

        // Clear screen and set background color
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Draw ground
        SDL_SetRenderDrawColor(renderer, 47, 47, 47, 255);
        SDL_Rect ground = {0, GROUND_Y, SCREEN_WIDTH, GROUND_HEIGHT};
        SDL_RenderFillRect(renderer, &ground);

        // Draw player (Saltarello)
        SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
        SDL_Rect playerRect = {player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT};
        SDL_RenderFillRect(renderer, &playerRect);

        // Draw obstacles
        for (const auto& obs : obstacles) {
            if (obs.isStone) {
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); // Gray for stone
            } else {
                SDL_SetRenderDrawColor(renderer, 144, 238, 144, 255); // Green for normal obstacles
            }
            SDL_Rect obstacleRect = {obs.x, obs.y, OBSTACLE_WIDTH, ObstacleHeight(obs)};
            SDL_RenderFillRect(renderer, &obstacleRect);
        }

        // Draw bullets
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Black for bullets
        for (const auto& bullet : bullets) {
            SDL_Rect bulletRect = {bullet.x, bullet.y, BULLET_WIDTH, BULLET_HEIGHT};
            SDL_RenderFillRect(renderer, &bulletRect);
        }

        // Draw bird
        if (bird.active) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_Rect birdRect = {bird.x, bird.y, BIRD_WIDTH, BIRD_HEIGHT};
            SDL_RenderFillRect(renderer, &birdRect);
        }

        // Display score on screen
        SDL_Color darkgrey = {47, 47, 47, 255};
        RenderText(renderer, font, "Score: " + std::to_string(game.score), 10, 10, darkgrey);

        // Display game over message
        if (game.gameOver) {
            SDL_Color red = {255, 0, 0, 255};
            RenderText(renderer, font, "GAME OVER", SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 - 120, red);
            RenderText(renderer, font, "Press ENTER to Restart", SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 - 60, red);
        }

        // Present updated frame
        SDL_RenderPresent(renderer);

        // Cap the frame rate without depending on the display refresh rate
        Uint32 elapsed = SDL_GetTicks() - currentTime;
        if (elapsed < FRAME_MS) SDL_Delay(FRAME_MS - elapsed);
    }

    Mix_FreeChunk(jumpSound);
    Mix_FreeChunk(collisionSound);
    Mix_FreeChunk(shootSound);
    Mix_FreeMusic(bgMusic);
    if (audioOk) Mix_CloseAudio();
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

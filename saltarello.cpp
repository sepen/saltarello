#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

// Constants for game settings
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int GROUND_HEIGHT = 100;
const int PLAYER_WIDTH = 40;
const int PLAYER_HEIGHT = 40;
const int OBSTACLE_WIDTH = 30;
const int OBSTACLE_HEIGHT = 60;
const int BIRD_WIDTH = 20;
const int BIRD_HEIGHT = 20;
const int GRAVITY = 1;
const int JUMP_STRENGTH = 18;
const int MOVE_SPEED = 5;
const int BULLET_WIDTH = 10;
const int BULLET_HEIGHT = 5;
const int BULLET_SPEED = 8;
const int FIRE_RATE = 1000;

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
    bool active;
};

// Function to render text on screen
void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color) {
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {x, y, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

int main() {
    // Initialize SDL, fonts, and audio
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    // Load sounds and music
    Mix_Chunk* jumpSound = Mix_LoadWAV("assets/jump.wav");
    Mix_Chunk* collisionSound = Mix_LoadWAV("assets/collision.wav");
    Mix_Chunk* shootSound = Mix_LoadWAV("assets/shoot.wav");
    Mix_Music* bgMusic = Mix_LoadMUS("assets/background.mp3");
    // Set background music volume
    if (bgMusic) {
        Mix_PlayMusic(bgMusic, -1);
        Mix_VolumeMusic(MIX_MAX_VOLUME * 0.04);
    }

    // Create SDL window and renderer
    SDL_Window* window = SDL_CreateWindow("Saltarello", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    // Load font for rendering text
    TTF_Font* font = TTF_OpenFont("assets/font.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        Mix_FreeChunk(jumpSound);
        Mix_FreeChunk(collisionSound);
        Mix_FreeChunk(shootSound);
        Mix_FreeMusic(bgMusic);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize game objects
    Saltarello player = {50, SCREEN_HEIGHT - GROUND_HEIGHT - PLAYER_HEIGHT, 0, false};
    std::vector<Obstacle> obstacles = {
        {SCREEN_WIDTH, SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT, true, false}, // Normal obstacle
        {SCREEN_WIDTH + 400, SCREEN_HEIGHT - GROUND_HEIGHT - 30, true, true} // Stone obstacle
    };
    std::vector<Bullet> bullets;
    Obstacle bird = {SCREEN_WIDTH + 400, SCREEN_HEIGHT - GROUND_HEIGHT - 120, false};

    bool quit = false, gameOver = false;
    int score = 0, obstacleSpeed = 5;
    SDL_Event e;
    Uint32 lastTime = SDL_GetTicks(), lastFireTime = 0;
    bool fireReleased = true;

    // Main game loop
    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();
        lastTime = currentTime;

        // Event handling
        while (SDL_PollEvent(&e)) {
            // Quit game on window close
            if (e.type == SDL_QUIT) quit = true;

            // Quit game on ESC key press
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) quit = true;

            // Restart game on ENTER key press after game over
            if (gameOver && e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) {
                // Reset game on ENTER key press after game over
                gameOver = false;
                score = 0;
                obstacleSpeed = 5;
                player = {50, SCREEN_HEIGHT - GROUND_HEIGHT - PLAYER_HEIGHT, 0, false};
                obstacles = {{SCREEN_WIDTH, SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT, true}};
                bird = {SCREEN_WIDTH + 400, SCREEN_HEIGHT - GROUND_HEIGHT - 120, false};
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (!gameOver) {
            // Jump with UP key
            if (keys[SDL_SCANCODE_UP] && !player.isJumping) {
                player.velocityY = -JUMP_STRENGTH;
                player.isJumping = true;
                Mix_PlayChannel(-1, jumpSound, 0);
            }

            // Move left
            if (keys[SDL_SCANCODE_LEFT] && player.x > 0) {
                player.x -= MOVE_SPEED;
            }

            // Move right
            if (keys[SDL_SCANCODE_RIGHT] && player.x < SCREEN_WIDTH - PLAYER_WIDTH) {
                player.x += MOVE_SPEED;
            }

            // Fire bullets with X key
            if (keys[SDL_SCANCODE_X]) {
                if (fireReleased && currentTime - lastFireTime >= FIRE_RATE) {
                    bullets.push_back({player.x + PLAYER_WIDTH, player.y + PLAYER_HEIGHT / 2, true});
                    Mix_PlayChannel(-1, shootSound, 0);
                    lastFireTime = currentTime;
                    score = std::max(0, score - 1);
                }
                fireReleased = false;
            } else {
                fireReleased = true;
            }

            // Update player position
            player.y += player.velocityY;
            player.velocityY += GRAVITY;
            if (player.y >= SCREEN_HEIGHT - GROUND_HEIGHT - PLAYER_HEIGHT) {
                player.y = SCREEN_HEIGHT - GROUND_HEIGHT - PLAYER_HEIGHT;
                player.isJumping = false;
            }

            // Update obstacle positions and check for collisions
            std::vector<Obstacle> newObstacles;
            for (auto& obs : obstacles) {
                obs.x -= obstacleSpeed;
                
                if (obs.x + OBSTACLE_WIDTH < 0) {
                    obs.x = SCREEN_WIDTH + (rand() % 300); // Random respawn position
                    obs.active = true;

                    if (obs.isStone) {
                        obs.y = SCREEN_HEIGHT - GROUND_HEIGHT - 30; // Stone at ground level
                    } else {
                        obs.y = SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT; // Normal obstacle
                    }
                    score++;
                
                    // Add more obstacles as score increases
                    if (score % 2 == 0) {
                        obstacleSpeed--;
                        newObstacles.push_back({SCREEN_WIDTH + 300, SCREEN_HEIGHT - GROUND_HEIGHT - OBSTACLE_HEIGHT, true});
                    }

                    // Increase speed every 5 points
                    if (score % 5 == 0) {
                        obstacleSpeed++;
                    }
                }

                // Check for collision between player and obstacles
                for (auto& obs : obstacles) {
                    if (player.x < obs.x + OBSTACLE_WIDTH && player.x + PLAYER_WIDTH > obs.x &&
                        player.y < obs.y + (obs.isStone ? 30 : OBSTACLE_HEIGHT) &&
                        player.y + PLAYER_HEIGHT > obs.y) {
                        gameOver = true;
                        Mix_PlayChannel(-1, collisionSound, 0);
                    }
                }
            }

            // Check for collision between bullets and obstacles
            for (auto it_bullet = bullets.begin(); it_bullet != bullets.end(); ) {
                it_bullet->x += BULLET_SPEED; // Move the bullet forward
                bool bulletDestroyed = false;
            
                for (auto it_obstacle = obstacles.begin(); it_obstacle != obstacles.end(); ++it_obstacle) {
                    if (it_bullet->x < it_obstacle->x + OBSTACLE_WIDTH &&
                        it_bullet->x + BULLET_WIDTH > it_obstacle->x &&
                        it_bullet->y < it_obstacle->y + OBSTACLE_HEIGHT &&
                        it_bullet->y + BULLET_HEIGHT > it_obstacle->y) {
                        
                        it_obstacle->active = false;  // Only this obstacle is deactivated
                        it_bullet = bullets.erase(it_bullet);  // Bullet is removed
                        bulletDestroyed = true;
                        break; // Stop checking obstacles
                    }
                }
            
                if (!bulletDestroyed) {
                    ++it_bullet; // Move to next bullet only if it wasn't erased
                }
            }

            // Add bird obstacle after score reaches 10
            if (score >= 10) {
                bird.active = true;
                bird.x -= obstacleSpeed - 4;

                if (bird.x + BIRD_WIDTH < 0) {
                    bird.x = SCREEN_WIDTH;
                }

                if (player.x < bird.x + BIRD_WIDTH && player.x + PLAYER_WIDTH > bird.x &&
                    player.y < bird.y + BIRD_HEIGHT && player.y + PLAYER_HEIGHT > bird.y && !keys[SDL_SCANCODE_DOWN]) {
                    gameOver = true;
                    Mix_PlayChannel(-1, collisionSound, 0);
                }
            }

            // Store the number of active obstacles BEFORE removing them
            int numObstaclesBefore = obstacles.size();

            // Remove inactive obstacles
            obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(), 
                [](Obstacle& obs) { return !obs.active; }), 
                obstacles.end());

            // Ensure the same number of obstacles is maintained
            while (obstacles.size() < numObstaclesBefore) {
                bool spawnStone = (rand() % 2 == 0); // 50% chance for a stone
                obstacles.push_back({
                    SCREEN_WIDTH + (rand() % 300),
                    SCREEN_HEIGHT - GROUND_HEIGHT - (spawnStone ? 30 : OBSTACLE_HEIGHT),
                    true,
                    spawnStone
                });
            }

            // Remove bullets that go off-screen
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                [](Bullet& bullet) { return bullet.x > SCREEN_WIDTH; }),
                bullets.end());

            // Remove bird if it goes off-screen
            if (bird.x + BIRD_WIDTH < 0) {
                bird.active = false;
                bird.x = SCREEN_WIDTH + 400;
            }

            // Increase obstacle speed every 10 points
            obstacleSpeed = 5 + score / 10;
        }

        // Clear screen and set background color
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Draw ground
        SDL_SetRenderDrawColor(renderer, 47, 47, 47, 255);
        SDL_Rect ground = {0, SCREEN_HEIGHT - GROUND_HEIGHT, SCREEN_WIDTH, GROUND_HEIGHT};
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
            SDL_Rect obstacleRect = {obs.x, obs.y, OBSTACLE_WIDTH, obs.isStone ? 30 : OBSTACLE_HEIGHT};
            SDL_RenderFillRect(renderer, &obstacleRect);
        }

        // Draw bullets
        for (const auto& bullet : bullets) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Black for  bullets
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
        RenderText(renderer, font, "Score: " + std::to_string(score), 10, 10, darkgrey);

        // Display game over message
        if (gameOver) {
            SDL_Color red = {255, 0, 0, 255};
            RenderText(renderer, font, "GAME OVER", SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 - 120, red);
            RenderText(renderer, font, "Press ENTER to Restart", SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 - 60, red);
        }

        // Present updated frame
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // Limit frame rate to ~60 FPS
    }

    Mix_FreeChunk(jumpSound);
    Mix_FreeChunk(collisionSound);
    Mix_FreeChunk(shootSound);
    Mix_FreeMusic(bgMusic);
    Mix_CloseAudio();
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

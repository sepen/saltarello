#include "render.h"

#include <string>

namespace {

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

} // namespace

void RenderGame(SDL_Renderer* renderer, TTF_Font* font, const GameState& game) {
    // Clear screen and set background color
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Draw ground
    SDL_SetRenderDrawColor(renderer, 47, 47, 47, 255);
    SDL_Rect ground = {0, GROUND_Y, SCREEN_WIDTH, GROUND_HEIGHT};
    SDL_RenderFillRect(renderer, &ground);

    // Draw player (Saltarello)
    SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
    SDL_Rect playerRect = {game.player.x, game.player.y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_RenderFillRect(renderer, &playerRect);

    // Draw obstacles
    for (const auto& obs : game.obstacles) {
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
    for (const auto& bullet : game.bullets) {
        SDL_Rect bulletRect = {bullet.x, bullet.y, BULLET_WIDTH, BULLET_HEIGHT};
        SDL_RenderFillRect(renderer, &bulletRect);
    }

    // Draw bird
    if (game.bird.active) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect birdRect = {game.bird.x, game.bird.y, BIRD_WIDTH, BIRD_HEIGHT};
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
}

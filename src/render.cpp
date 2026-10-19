#include "render.h"
#include "assets.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

const int FONT_SIZE = 28; // world units

// Renders text at (x, y) in world units, or centered on x if `centered`.
// The font is opened at the real pixel size, so the text is drawn back at
// 1 / scale to land on the world grid at full sharpness.
void RenderText(SDL_Renderer* renderer, const Graphics& graphics, const std::string& text,
                int x, int y, SDL_Color color, bool centered = false) {
    SDL_Surface* textSurface = TTF_RenderText_Blended(graphics.font, text.c_str(), color);
    if (!textSurface) return;
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture) {
        int w = static_cast<int>(std::lround(textSurface->w / graphics.scale));
        int h = static_cast<int>(std::lround(textSurface->h / graphics.scale));
        SDL_Rect textRect = {centered ? x - w / 2 : x, y, w, h};
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_DestroyTexture(textTexture);
    }
    SDL_FreeSurface(textSurface);
}

// Draws a sprite, or a plain rectangle if the sprite could not be created
void DrawSprite(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Rect dst, SDL_Color fallback) {
    if (texture) {
        SDL_RenderCopy(renderer, texture, NULL, &dst);
    } else {
        SDL_SetRenderDrawColor(renderer, fallback.r, fallback.g, fallback.b, fallback.a);
        SDL_RenderFillRect(renderer, &dst);
    }
}

} // namespace

Graphics CreateGraphics(SDL_Renderer* renderer) {
    int outW = SCREEN_WIDTH, outH = SCREEN_HEIGHT;
    SDL_GetRendererOutputSize(renderer, &outW, &outH);
    float scale = std::max(1.0f, std::min(static_cast<float>(outW) / SCREEN_WIDTH,
                                          static_cast<float>(outH) / SCREEN_HEIGHT));
    TTF_Font* font = TTF_OpenFont(AssetPath("font.ttf").c_str(),
                                  static_cast<int>(std::lround(FONT_SIZE * scale)));
    return {scale, font, CreateSprites(renderer, scale)};
}

void DestroyGraphics(Graphics& graphics) {
    if (graphics.font) TTF_CloseFont(graphics.font);
    DestroySprites(graphics.sprites);
    graphics.font = nullptr;
}

void RenderGame(SDL_Renderer* renderer, const Graphics& graphics, const GameState& game) {
    const Sprites& sprites = graphics.sprites;

    // Clear screen and set background color
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Draw ground
    SDL_SetRenderDrawColor(renderer, 47, 47, 47, 255);
    SDL_Rect ground = {0, GROUND_Y, SCREEN_WIDTH, GROUND_HEIGHT};
    SDL_RenderFillRect(renderer, &ground);

    // Draw player (Saltarello): a chicken, squashed while ducking
    Rect box = PlayerBox(game.player);
    DrawSprite(renderer, game.player.isDucking ? sprites.chickenSquashed : sprites.chicken,
               {box.x, box.y, box.w, box.h}, {255, 255, 255, 255});

    // Draw obstacles: smiling trees and poops
    for (const auto& obs : game.obstacles) {
        DrawSprite(renderer, obs.isPoop ? sprites.poop : sprites.tree,
                   {obs.x, obs.y, OBSTACLE_WIDTH, ObstacleHeight(obs)},
                   obs.isPoop ? SDL_Color{135, 85, 40, 255} : SDL_Color{90, 190, 70, 255});
    }

    // Draw bullets (eggs)
    for (const auto& bullet : game.bullets) {
        DrawSprite(renderer, sprites.egg, {bullet.x, bullet.y, BULLET_WIDTH, BULLET_HEIGHT},
                   {255, 250, 235, 255});
    }

    // Draw flying pig
    if (game.pig.active) {
        DrawSprite(renderer, sprites.pig, {game.pig.x, game.pig.y, PIG_WIDTH, PIG_HEIGHT},
                   {255, 175, 195, 255});
    }

    // Display score on screen
    SDL_Color darkgrey = {47, 47, 47, 255};
    RenderText(renderer, graphics, "Score: " + std::to_string(game.score), 16, 12, darkgrey);

    // Display game over message
    if (game.gameOver) {
        SDL_Color red = {255, 0, 0, 255};
        RenderText(renderer, graphics, "GAME OVER", SCREEN_WIDTH / 2, GROUND_Y / 2 - 60, red, true);
        RenderText(renderer, graphics, "Press ENTER to Restart", SCREEN_WIDTH / 2, GROUND_Y / 2 - 15, red, true);
    }
}

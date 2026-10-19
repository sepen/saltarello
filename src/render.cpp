#include "render.h"
#include "assets.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

const int FONT_SIZE = 28; // world units

enum class Align { Left, Center, Right };

// Renders text at (x, y) in world units, aligned on x. The font is opened at
// the real pixel size, so the text is drawn back at 1 / scale to land on the
// world grid at full sharpness.
void RenderText(SDL_Renderer* renderer, const Graphics& graphics, const std::string& text,
                int x, int y, SDL_Color color, Align align = Align::Left) {
    SDL_Surface* textSurface = TTF_RenderText_Blended(graphics.font, text.c_str(), color);
    if (!textSurface) return;
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture) {
        int w = static_cast<int>(std::lround(textSurface->w / graphics.scale));
        int h = static_cast<int>(std::lround(textSurface->h / graphics.scale));
        if (align == Align::Center) x -= w / 2;
        if (align == Align::Right) x -= w;
        SDL_Rect textRect = {x, y, w, h};
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

// Draws the scene: sky, ground, chicken, obstacles and eggs
void DrawWorld(SDL_Renderer* renderer, const Graphics& graphics, const GameState& game) {
    const Sprites& sprites = graphics.sprites;

    // Clear screen and set background color
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Draw ground
    SDL_SetRenderDrawColor(renderer, 47, 47, 47, 255);
    SDL_Rect ground = {0, GROUND_Y, game.width, GROUND_HEIGHT};
    SDL_RenderFillRect(renderer, &ground);

    // Draw player (Saltarello): a chicken, squashed while ducking
    Rect box = PlayerBox(game.player);
    DrawSprite(renderer, game.player.isDucking ? sprites.chickenSquashed : sprites.chicken,
               {box.x, box.y, box.w, box.h}, {255, 255, 255, 255});

    // Draw obstacles: smiling trees and poops, flying pigs
    for (const auto& obs : game.obstacles) {
        SDL_Texture* texture = sprites.tree;
        SDL_Color fallback = {90, 190, 70, 255};
        if (obs.kind == ObstacleKind::Poop) {
            texture = sprites.poop;
            fallback = {135, 85, 40, 255};
        } else if (obs.kind == ObstacleKind::Pig) {
            texture = sprites.pig;
            fallback = {255, 175, 195, 255};
        }
        DrawSprite(renderer, texture, {obs.x, obs.y, ObstacleWidth(obs), ObstacleHeight(obs)}, fallback);
    }

    // Draw bullets (eggs)
    for (const auto& bullet : game.bullets) {
        DrawSprite(renderer, sprites.egg, {bullet.x, bullet.y, BULLET_WIDTH, BULLET_HEIGHT},
                   {255, 250, 235, 255});
    }
}

} // namespace

Graphics CreateGraphics(SDL_Renderer* renderer, int width) {
    int outW = width, outH = SCREEN_HEIGHT;
    SDL_GetRendererOutputSize(renderer, &outW, &outH);
    float scale = std::max(1.0f, std::min(static_cast<float>(outW) / width,
                                          static_cast<float>(outH) / SCREEN_HEIGHT));
    TTF_Font* font = TTF_OpenFont(AssetPath("font.ttf").c_str(),
                                  static_cast<int>(std::lround(FONT_SIZE * scale)));
    return {width, scale, font, CreateSprites(renderer, scale)};
}

void DestroyGraphics(Graphics& graphics) {
    if (graphics.font) TTF_CloseFont(graphics.font);
    DestroySprites(graphics.sprites);
    graphics.font = nullptr;
}

void RenderGame(SDL_Renderer* renderer, const Graphics& graphics, const GameState& game) {
    const Sprites& sprites = graphics.sprites;
    DrawWorld(renderer, graphics, game);

    // Display score on screen
    SDL_Color darkgrey = {47, 47, 47, 255};
    RenderText(renderer, graphics, "Score: " + std::to_string(game.score), 16, 12, darkgrey);

    // Eggs left, top-right: full ones solid, the one recharging fading in
    const int iconW = 24, iconH = 19, spacing = 30;
    for (int i = 0; i < MAX_EGGS; i++) {
        float fill = i < game.eggs ? 1.0f
                   : i == game.eggs ? static_cast<float>(game.eggRecharge) / EGG_RECHARGE_TICKS
                   : 0.0f;
        SDL_Rect icon = {game.width - 16 - (MAX_EGGS - i) * spacing + (spacing - iconW), 16, iconW, iconH};
        if (sprites.egg) {
            SDL_SetTextureAlphaMod(sprites.egg, static_cast<Uint8>(50 + 205 * fill));
            SDL_RenderCopy(renderer, sprites.egg, NULL, &icon);
            SDL_SetTextureAlphaMod(sprites.egg, 255);
        }
    }

    // Display game over message
    if (game.gameOver) {
        SDL_Color red = {255, 0, 0, 255};
        RenderText(renderer, graphics, "GAME OVER", game.width / 2, GROUND_Y / 2 - 60, red, Align::Center);
        RenderText(renderer, graphics, "Press ENTER to Restart", game.width / 2, GROUND_Y / 2 - 15, red, Align::Center);
    }
}

void RenderTitle(SDL_Renderer* renderer, const Graphics& graphics) {
    // The chicken waiting on an empty track
    GameState scene = NewGame(graphics.width);
    scene.obstacles.clear();
    DrawWorld(renderer, graphics, scene);

    const SDL_Color title = {225, 45, 45, 255};
    const SDL_Color key = {47, 47, 47, 255};
    const SDL_Color text = {90, 90, 90, 255};
    const int center = graphics.width / 2;
    RenderText(renderer, graphics, "SALTARELLO", center, 40, title, Align::Center);

    const char* legend[][2] = {
        {"UP", "Jump (hold: higher)"},
        {"DOWN", "Duck under flying pigs"},
        {"LEFT / RIGHT", "Move"},
        {"X", "Throw an egg"},
        {"F / F11", "Fullscreen"},
        {"ESC", "Quit"},
    };
    int y = 110;
    for (const auto& line : legend) {
        // Columns placed for the narrowest (800) world, centered in wider ones
        const int left = (graphics.width - SCREEN_WIDTH) / 2;
        RenderText(renderer, graphics, line[0], left + 250, y, key, Align::Right);
        RenderText(renderer, graphics, line[1], left + 280, y, text);
        y += 40;
    }
    RenderText(renderer, graphics, "Press ENTER to start", center, y + 30, title, Align::Center);
}

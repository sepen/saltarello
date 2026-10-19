#ifndef SALTARELLO_RENDER_H
#define SALTARELLO_RENDER_H

#include "game.h"
#include "sprites.h"

#include <SDL.h>
#include <SDL_ttf.h>

// Everything needed to draw a frame. The renderer works in world units
// (SDL_RenderSetLogicalSize) and scales them to the window; sprites and text
// are created at the real pixel density so they stay sharp. Recreate it
// when the window size changes.
struct Graphics {
    float scale;  // output pixels per world unit
    TTF_Font* font;
    Sprites sprites;
};

// On failure `font` is NULL (sprites fall back to plain rectangles)
Graphics CreateGraphics(SDL_Renderer* renderer);
void DestroyGraphics(Graphics& graphics);

// Draws a full frame (without presenting it)
void RenderGame(SDL_Renderer* renderer, const Graphics& graphics, const GameState& game);

#endif

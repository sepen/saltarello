#ifndef SALTARELLO_RENDER_H
#define SALTARELLO_RENDER_H

#include "game.h"
#include "sprites.h"

#include <SDL.h>
#include <SDL_ttf.h>

// Draws a full frame (without presenting it)
void RenderGame(SDL_Renderer* renderer, TTF_Font* font, const Sprites& sprites, const GameState& game);

#endif

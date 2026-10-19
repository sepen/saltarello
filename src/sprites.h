#ifndef SALTARELLO_SPRITES_H
#define SALTARELLO_SPRITES_H

// Sprites drawn in code at startup, so the game needs no image files or
// image libraries.

#include <SDL.h>

// The player: an orange ball face with wide eyes, a grin and its tongue
// out. The face fills a w x h ellipse, so a wider, shorter size gives the
// same ball squashed. Returns an RGBA surface the caller must free.
SDL_Surface* CreateBallSurface(int w, int h);

// Same as CreateBallSurface(), uploaded as a texture with alpha blending
SDL_Texture* CreateBallTexture(SDL_Renderer* renderer, int w, int h);

// All the game's sprites, created once at startup
struct Sprites {
    SDL_Texture* ball;          // standing player
    SDL_Texture* ballSquashed;  // ducking player
};

Sprites CreateSprites(SDL_Renderer* renderer);
void DestroySprites(Sprites& sprites);

#endif

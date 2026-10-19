#ifndef SALTARELLO_SPRITES_H
#define SALTARELLO_SPRITES_H

// Sprites drawn in code at startup, so the game needs no image files or
// image libraries.

#include <SDL.h>

enum class SpriteKind {
    Chicken,  // the player, facing right
    Pig,      // flying pig, facing left
    Tree,     // tall obstacle, smiling
    Poop,     // short obstacle, smiling
    Egg,      // bullet
};

// Draws a sprite into a new w x h RGBA surface the caller must free. The
// drawing keeps its own proportions and is stretched to fill w x h, so the
// chicken drawn at a wider, shorter size comes out squashed.
SDL_Surface* CreateSpriteSurface(SpriteKind kind, int w, int h);

// All the game's sprites, created once at startup
struct Sprites {
    SDL_Texture* chicken;
    SDL_Texture* chickenSquashed;  // ducking
    SDL_Texture* pig;
    SDL_Texture* tree;
    SDL_Texture* poop;
    SDL_Texture* egg;
};

// Sprites are drawn at `scale` output pixels per world unit, to be shown at
// their world size
Sprites CreateSprites(SDL_Renderer* renderer, float scale);
void DestroySprites(Sprites& sprites);

#endif

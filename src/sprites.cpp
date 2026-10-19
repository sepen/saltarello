#include "sprites.h"
#include "game.h"

#include <algorithm>

namespace {

struct Color {
    float r, g, b, a;
};

const Color OUTLINE = {120, 50, 0, 1};
const Color BODY_DARK = {240, 130, 0, 1};
const Color BODY_LIGHT = {255, 205, 60, 1};
const Color HIGHLIGHT = {255, 255, 255, 0.55f};
const Color FEATURE_LINE = {90, 35, 0, 1};
const Color EYE_WHITE = {255, 255, 255, 1};
const Color PUPIL = {40, 20, 10, 1};
const Color MOUTH = {110, 20, 30, 1};
const Color TEETH = {255, 255, 255, 1};
const Color TONGUE_LINE = {150, 30, 50, 1};
const Color TONGUE = {255, 95, 120, 1};
const Color TONGUE_GROOVE = {220, 60, 90, 1};

// Paints `top` over `base`
Color Over(const Color& base, const Color& top) {
    float a = top.a;
    return {base.r + (top.r - base.r) * a,
            base.g + (top.g - base.g) * a,
            base.b + (top.b - base.b) * a,
            base.a + (1 - base.a) * a};
}

bool InEllipse(float u, float v, float cx, float cy, float rx, float ry) {
    float du = (u - cx) / rx;
    float dv = (v - cy) / ry;
    return du * du + dv * dv <= 1;
}

// Color of the face at (u, v), both in [-1, 1] across the sprite
Color Shade(float u, float v) {
    float d = u * u + v * v;
    if (d > 1) return {0, 0, 0, 0};

    // Body, lit from the top-left
    float light = std::min(1.0f, std::max(0.0f, 0.5f - 0.35f * u - 0.45f * v));
    Color c = {BODY_DARK.r + (BODY_LIGHT.r - BODY_DARK.r) * light,
               BODY_DARK.g + (BODY_LIGHT.g - BODY_DARK.g) * light,
               BODY_DARK.b + (BODY_LIGHT.b - BODY_DARK.b) * light, 1};
    if (InEllipse(u, v, -0.42f, -0.6f, 0.22f, 0.1f)) c = Over(c, HIGHLIGHT);

    // Wide eyes looking slightly right and down
    for (float s : {-1.0f, 1.0f}) {
        float ex = s * 0.32f, ey = -0.25f;
        if (InEllipse(u, v, ex, ey, 0.23f, 0.29f)) c = FEATURE_LINE;
        if (InEllipse(u, v, ex, ey, 0.19f, 0.25f)) c = EYE_WHITE;
        if (InEllipse(u, v, ex + 0.04f, ey + 0.05f, 0.1f, 0.13f)) c = PUPIL;
        if (InEllipse(u, v, ex + 0.08f, ey - 0.02f, 0.04f, 0.04f)) c = EYE_WHITE;
    }

    // Open grin with the upper teeth showing
    if (v >= 0.08f && InEllipse(u, v, 0, 0.12f, 0.63f, 0.6f)) c = FEATURE_LINE;
    bool inMouth = v >= 0.12f && InEllipse(u, v, 0, 0.12f, 0.58f, 0.55f);
    if (inMouth) c = v < 0.24f ? TEETH : MOUTH;

    // Tongue out, from inside the mouth down past its lower lip
    if (v > 0.3f) {
        if (InEllipse(u, v, 0.14f, 0.62f, 0.28f, 0.3f)) c = TONGUE_LINE;
        if (InEllipse(u, v, 0.14f, 0.62f, 0.24f, 0.26f)) {
            c = (v > 0.5f && u > 0.115f && u < 0.165f) ? TONGUE_GROOVE : TONGUE;
        }
    }

    // Outline around the ball
    if (d > 0.9f * 0.9f) c = OUTLINE;
    return c;
}

} // namespace

SDL_Surface* CreateBallSurface(int w, int h) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) return nullptr;

    // 4x4 supersampling for smooth edges
    const int SS = 4;
    SDL_LockSurface(surface);
    for (int y = 0; y < h; y++) {
        Uint32* row = reinterpret_cast<Uint32*>(static_cast<Uint8*>(surface->pixels) + y * surface->pitch);
        for (int x = 0; x < w; x++) {
            float r = 0, g = 0, b = 0, a = 0;
            for (int sy = 0; sy < SS; sy++) {
                for (int sx = 0; sx < SS; sx++) {
                    float u = (x + (sx + 0.5f) / SS) / w * 2 - 1;
                    float v = (y + (sy + 0.5f) / SS) / h * 2 - 1;
                    Color c = Shade(u, v);
                    r += c.r * c.a;
                    g += c.g * c.a;
                    b += c.b * c.a;
                    a += c.a;
                }
            }
            Uint8 alpha = static_cast<Uint8>(a / (SS * SS) * 255 + 0.5f);
            if (a > 0) {
                row[x] = SDL_MapRGBA(surface->format, static_cast<Uint8>(r / a + 0.5f),
                                     static_cast<Uint8>(g / a + 0.5f),
                                     static_cast<Uint8>(b / a + 0.5f), alpha);
            } else {
                row[x] = SDL_MapRGBA(surface->format, 0, 0, 0, 0);
            }
        }
    }
    SDL_UnlockSurface(surface);
    return surface;
}

SDL_Texture* CreateBallTexture(SDL_Renderer* renderer, int w, int h) {
    SDL_Surface* surface = CreateBallSurface(w, h);
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (texture) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

Sprites CreateSprites(SDL_Renderer* renderer) {
    Rect standing = PlayerBox({0, 0, 0, false, false});
    Rect ducking = PlayerBox({0, 0, 0, false, true});
    return {CreateBallTexture(renderer, standing.w, standing.h),
            CreateBallTexture(renderer, ducking.w, ducking.h)};
}

void DestroySprites(Sprites& sprites) {
    if (sprites.ball) SDL_DestroyTexture(sprites.ball);
    if (sprites.ballSquashed) SDL_DestroyTexture(sprites.ballSquashed);
    sprites = {nullptr, nullptr};
}

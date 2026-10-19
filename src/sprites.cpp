#include "sprites.h"
#include "game.h"

#include <algorithm>
#include <cmath>

namespace {

struct Color {
    float r, g, b, a;
};

Color Rgb(int r, int g, int b) {
    return {static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), 1};
}

const Color NONE = {0, 0, 0, 0};
const Color WHITE = Rgb(255, 255, 255);
const Color EYE = Rgb(30, 25, 25);

Color Mix(const Color& a, const Color& b, float t) {
    t = std::min(1.0f, std::max(0.0f, t));
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t};
}

// Shape tests. Coordinates: v goes from -1 (top) to 1 (bottom) and u from
// -aspect to aspect (left to right), so circles stay round.

bool InEllipse(float u, float v, float cx, float cy, float rx, float ry) {
    float du = (u - cx) / rx;
    float dv = (v - cy) / ry;
    return du * du + dv * dv <= 1;
}

bool InCircle(float u, float v, float cx, float cy, float r) {
    return InEllipse(u, v, cx, cy, r, r);
}

bool InTriangle(float u, float v, float ax, float ay, float bx, float by, float cx, float cy) {
    float d1 = (u - bx) * (ay - by) - (ax - bx) * (v - by);
    float d2 = (u - cx) * (by - cy) - (bx - cx) * (v - cy);
    float d3 = (u - ax) * (cy - ay) - (cx - ax) * (v - ay);
    bool hasNeg = d1 < 0 || d2 < 0 || d3 < 0;
    bool hasPos = d1 > 0 || d2 > 0 || d3 > 0;
    return !(hasNeg && hasPos);
}

// Thick line from a to b with round ends
bool InSegment(float u, float v, float ax, float ay, float bx, float by, float thickness) {
    float dx = bx - ax, dy = by - ay;
    float t = ((u - ax) * dx + (v - ay) * dy) / (dx * dx + dy * dy);
    t = std::min(1.0f, std::max(0.0f, t));
    float px = ax + t * dx - u, py = ay + t * dy - v;
    return px * px + py * py <= thickness * thickness;
}

// Lower half of an elliptical ring: a smile
bool InSmile(float u, float v, float cx, float cy, float rx, float ry, float thickness) {
    return v >= cy && InEllipse(u, v, cx, cy, rx + thickness, ry + thickness) &&
           !InEllipse(u, v, cx, cy, rx - thickness, ry - thickness);
}

// An eye: dark pupil with a white glint
bool PaintEye(Color& c, float u, float v, float cx, float cy, float r) {
    if (!InCircle(u, v, cx, cy, r)) return false;
    c = InCircle(u, v, cx + r * 0.35f, cy - r * 0.35f, r * 0.4f) ? WHITE : EYE;
    return true;
}

// --- Chicken: white body, red comb and wattle, orange beak and legs ---------

Color ShadeChicken(float u, float v) {
    const Color LINE = Rgb(70, 60, 60);
    const Color FEATHER = WHITE;
    const Color FEATHER_SHADE = Rgb(220, 220, 232);
    const Color COMB = Rgb(225, 45, 45);
    const Color ORANGE = Rgb(245, 160, 30);
    const float g = 0.07f; // outline width
    Color c = NONE;

    // Comb, behind the head
    const float combX[] = {0.27f, 0.42f, 0.57f};
    const float combY[] = {-0.7f, -0.78f, -0.7f};
    for (int i = 0; i < 3; i++) {
        if (InCircle(u, v, combX[i], combY[i], 0.12f + g)) c = LINE;
    }
    for (int i = 0; i < 3; i++) {
        if (InCircle(u, v, combX[i], combY[i], 0.12f)) c = COMB;
    }

    // Legs and feet, behind the body
    auto legs = [&](float t) {
        return InSegment(u, v, -0.2f, 0.55f, -0.22f, 0.92f, t) ||
               InSegment(u, v, -0.22f, 0.92f, -0.05f, 0.92f, t) ||
               InSegment(u, v, 0.15f, 0.55f, 0.17f, 0.92f, t) ||
               InSegment(u, v, 0.17f, 0.92f, 0.34f, 0.92f, t);
    };
    if (legs(0.055f + 0.04f)) c = LINE;
    if (legs(0.055f)) c = ORANGE;

    // Body: tail, body, neck and head as one outlined shape
    auto body = [&](float grow) {
        return InEllipse(u, v, -0.7f, -0.12f, 0.2f + grow, 0.3f + grow) ||
               InEllipse(u, v, -0.05f, 0.18f, 0.68f + grow, 0.47f + grow) ||
               InEllipse(u, v, 0.27f, -0.1f, 0.3f + grow, 0.3f + grow) ||
               InCircle(u, v, 0.42f, -0.38f, 0.3f + grow);
    };
    if (body(g)) c = LINE;
    if (body(0)) c = Mix(FEATHER, FEATHER_SHADE, (v + 0.2f) * 0.9f + u * 0.2f);

    // Wing
    if (InEllipse(u, v, -0.12f, 0.2f, 0.34f + 0.05f, 0.21f + 0.05f)) c = LINE;
    if (InEllipse(u, v, -0.12f, 0.2f, 0.34f, 0.21f)) c = Mix(FEATHER_SHADE, FEATHER, -v * 2);

    // Beak, wattle and eye
    if (InTriangle(u, v, 0.62f, -0.54f, 0.62f, -0.2f, 1.0f, -0.37f)) c = LINE;
    if (InTriangle(u, v, 0.67f, -0.48f, 0.67f, -0.26f, 0.93f, -0.37f)) c = ORANGE;
    if (InEllipse(u, v, 0.64f, -0.12f, 0.075f + 0.04f, 0.12f + 0.04f)) c = LINE;
    if (InEllipse(u, v, 0.64f, -0.12f, 0.075f, 0.12f)) c = COMB;
    PaintEye(c, u, v, 0.48f, -0.45f, 0.085f);
    return c;
}

// --- Flying pig: pink body, white wing, curly tail, smiling ---------------

Color ShadePig(float u, float v) {
    u = -u; // drawn facing right, mirrored to face left (where it flies)
    const Color LINE = Rgb(120, 50, 70);
    const Color PINK = Rgb(255, 175, 195);
    const Color PINK_SHADE = Rgb(240, 135, 165);
    const Color SNOUT = Rgb(250, 140, 165);
    const Color WING_SHADE = Rgb(215, 225, 240);
    const float g = 0.08f;
    Color c = NONE;

    // Curly tail
    float tailRing = std::sqrt((u + 1.22f) * (u + 1.22f) + (v + 0.05f) * (v + 0.05f));
    if (std::fabs(tailRing - 0.15f) < 0.05f + 0.03f) c = LINE;
    if (std::fabs(tailRing - 0.15f) < 0.05f) c = PINK_SHADE;

    // Legs
    const float legX[] = {-0.7f, -0.35f, 0.35f, 0.7f};
    for (float x : legX) {
        if (InEllipse(u, v, x, 0.72f, 0.13f + g, 0.2f + g)) c = LINE;
    }
    for (float x : legX) {
        if (InEllipse(u, v, x, 0.72f, 0.13f, 0.2f)) c = PINK_SHADE;
    }

    // Body and ear
    auto ear = [&](float grow) {
        return InTriangle(u, v, 0.4f - grow, -0.45f + grow, 0.8f + grow, -0.5f + grow, 0.55f, -0.95f - grow);
    };
    if (ear(g) || InEllipse(u, v, 0, 0.05f, 1.1f + g, 0.68f + g)) c = LINE;
    if (ear(0)) c = PINK_SHADE;
    if (InEllipse(u, v, 0, 0.05f, 1.1f, 0.68f)) c = Mix(PINK, PINK_SHADE, (v + 0.1f) * 1.2f);

    // Snout with nostrils
    if (InEllipse(u, v, 1.12f, 0.1f, 0.24f + g, 0.28f + g)) c = LINE;
    if (InEllipse(u, v, 1.12f, 0.1f, 0.24f, 0.28f)) c = SNOUT;
    if (InEllipse(u, v, 1.06f, 0.08f, 0.045f, 0.08f) || InEllipse(u, v, 1.19f, 0.08f, 0.045f, 0.08f)) c = LINE;

    // Eye and smile
    PaintEye(c, u, v, 0.66f, -0.18f, 0.09f);
    if (InSmile(u, v, 0.66f, 0.28f, 0.2f, 0.13f, 0.04f)) c = LINE;

    // Wing on its back
    auto wing = [&](float grow) {
        return InEllipse(u, v, -0.2f, -0.55f, 0.42f + grow, 0.32f + grow) ||
               InEllipse(u, v, -0.5f, -0.7f, 0.28f + grow, 0.2f + grow);
    };
    if (wing(0.06f)) c = LINE;
    if (wing(0)) c = Mix(WHITE, WING_SHADE, (v + 0.75f) * 2);
    if (wing(0) && std::fabs(u + 0.25f + (v + 0.5f) * 0.6f) < 0.03f) c = WING_SHADE; // feather line
    return c;
}

// --- Tree: brown trunk, round green crown with a smiling face -------------

Color ShadeTree(float u, float v) {
    const Color LINE = Rgb(30, 85, 30);
    const Color TRUNK_LINE = Rgb(80, 45, 20);
    const Color TRUNK = Rgb(150, 95, 50);
    const Color LEAVES_DARK = Rgb(70, 165, 60);
    const Color LEAVES_LIGHT = Rgb(135, 215, 90);
    const Color CHEEK = {255, 140, 150, 0.6f};
    const float g = 0.05f;
    Color c = NONE;

    // Trunk
    if (std::fabs(u) < 0.13f + g && v > 0.05f) c = TRUNK_LINE;
    if (std::fabs(u) < 0.13f && v > 0.05f && v < 1 - g) c = TRUNK;

    // Crown: overlapping circles
    auto crown = [&](float grow) {
        return InCircle(u, v, 0, -0.5f, 0.42f + grow) ||
               InCircle(u, v, -0.19f, -0.18f, 0.25f + grow) ||
               InCircle(u, v, 0.19f, -0.18f, 0.25f + grow) ||
               InCircle(u, v, 0, -0.1f, 0.3f + grow);
    };
    if (crown(g)) c = LINE;
    if (crown(0)) c = Mix(LEAVES_LIGHT, LEAVES_DARK, (u + v + 0.6f) * 0.9f);

    // Face
    PaintEye(c, u, v, -0.13f, -0.45f, 0.07f);
    PaintEye(c, u, v, 0.13f, -0.45f, 0.07f);
    if (InEllipse(u, v, -0.25f, -0.3f, 0.07f, 0.045f) || InEllipse(u, v, 0.25f, -0.3f, 0.07f, 0.045f)) {
        c = Mix(c, {CHEEK.r, CHEEK.g, CHEEK.b, 1}, CHEEK.a);
    }
    if (InSmile(u, v, 0, -0.33f, 0.15f, 0.12f, 0.03f)) c = LINE;
    return c;
}

// --- Poop: three brown tiers and a tip, with a big smile ------------------

Color ShadePoop(float u, float v) {
    const Color LINE = Rgb(70, 40, 15);
    const Color BROWN = Rgb(135, 85, 40);
    const Color BROWN_LIGHT = Rgb(175, 115, 60);
    const float g = 0.07f;
    Color c = NONE;

    // Tiers from the bottom up, each one outlined over the previous
    struct Tier {
        float cx, cy, rx, ry;
    };
    const Tier tiers[] = {
        {0, 0.6f, 0.88f, 0.32f},
        {0, 0.17f, 0.68f, 0.28f},
        {0.03f, -0.2f, 0.46f, 0.24f},
        {0.12f, -0.52f, 0.2f, 0.17f},
    };
    for (const Tier& t : tiers) {
        if (InEllipse(u, v, t.cx, t.cy, t.rx + g, t.ry + g)) c = LINE;
        if (InEllipse(u, v, t.cx, t.cy, t.rx, t.ry)) {
            c = Mix(BROWN_LIGHT, BROWN, (v - t.cy) / t.ry * 0.5f + 0.5f);
        }
    }
    // Curled tip
    if (InTriangle(u, v, 0.02f, -0.6f, 0.3f, -0.6f, 0.28f, -0.97f)) c = LINE;
    if (InTriangle(u, v, 0.09f, -0.62f, 0.25f, -0.62f, 0.25f, -0.86f)) c = BROWN_LIGHT;

    // Face: big white eyes and a smile
    for (float s : {-1.0f, 1.0f}) {
        float ex = s * 0.28f, ey = 0.08f;
        if (InCircle(u, v, ex, ey, 0.17f + 0.05f)) c = LINE;
        if (InCircle(u, v, ex, ey, 0.17f)) c = WHITE;
        PaintEye(c, u, v, ex + 0.03f, ey + 0.03f, 0.09f);
    }
    if (InSmile(u, v, 0, 0.42f, 0.3f, 0.17f, 0.05f)) c = LINE;
    return c;
}

// --- Egg -------------------------------------------------------------------

Color ShadeEgg(float u, float v) {
    // Pointier towards the front (right)
    float ry = 0.85f * (1 - 0.12f * u);
    if (!InEllipse(u, v, 0, 0, 1.2f, ry)) return NONE;
    if (!InEllipse(u, v, 0, 0, 1.2f - 0.22f, ry - 0.22f)) return Rgb(150, 125, 95);
    return Mix(Rgb(255, 250, 235), Rgb(240, 220, 185), (u + v + 0.6f) * 0.7f);
}

Color Shade(SpriteKind kind, float u, float v) {
    switch (kind) {
        case SpriteKind::Chicken: return ShadeChicken(u, v);
        case SpriteKind::Pig: return ShadePig(u, v);
        case SpriteKind::Tree: return ShadeTree(u, v);
        case SpriteKind::Poop: return ShadePoop(u, v);
        case SpriteKind::Egg: return ShadeEgg(u, v);
    }
    return NONE;
}

// Width / height of each drawing's own coordinate space
float DrawingAspect(SpriteKind kind) {
    switch (kind) {
        case SpriteKind::Chicken: return 1.0f;
        case SpriteKind::Pig: return 1.5f;
        case SpriteKind::Tree: return 0.5f;
        case SpriteKind::Poop: return 1.0f;
        case SpriteKind::Egg: return 1.25f;
    }
    return 1.0f;
}

// Creates the texture for a w x h (world units) sprite at `scale` pixels per unit
SDL_Texture* CreateSpriteTexture(SDL_Renderer* renderer, float scale, SpriteKind kind, int w, int h) {
    SDL_Surface* surface = CreateSpriteSurface(kind, static_cast<int>(std::lround(w * scale)),
                                               static_cast<int>(std::lround(h * scale)));
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (texture) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

} // namespace

SDL_Surface* CreateSpriteSurface(SpriteKind kind, int w, int h) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) return nullptr;

    const float aspect = DrawingAspect(kind);
    const int SS = 4; // 4x4 supersampling for smooth edges
    SDL_LockSurface(surface);
    for (int y = 0; y < h; y++) {
        Uint32* row = reinterpret_cast<Uint32*>(static_cast<Uint8*>(surface->pixels) + y * surface->pitch);
        for (int x = 0; x < w; x++) {
            float r = 0, g = 0, b = 0, a = 0;
            for (int sy = 0; sy < SS; sy++) {
                for (int sx = 0; sx < SS; sx++) {
                    float u = ((x + (sx + 0.5f) / SS) / w * 2 - 1) * aspect;
                    float v = (y + (sy + 0.5f) / SS) / h * 2 - 1;
                    Color c = Shade(kind, u, v);
                    r += c.r * c.a;
                    g += c.g * c.a;
                    b += c.b * c.a;
                    a += c.a;
                }
            }
            if (a > 0) {
                row[x] = SDL_MapRGBA(surface->format, static_cast<Uint8>(r / a + 0.5f),
                                     static_cast<Uint8>(g / a + 0.5f), static_cast<Uint8>(b / a + 0.5f),
                                     static_cast<Uint8>(a / (SS * SS) * 255 + 0.5f));
            } else {
                row[x] = SDL_MapRGBA(surface->format, 0, 0, 0, 0);
            }
        }
    }
    SDL_UnlockSurface(surface);
    return surface;
}

Sprites CreateSprites(SDL_Renderer* renderer, float scale) {
    Rect standing = PlayerBox({0, 0, 0, false, false});
    Rect ducking = PlayerBox({0, 0, 0, false, true});
    return {
        CreateSpriteTexture(renderer, scale, SpriteKind::Chicken, standing.w, standing.h),
        CreateSpriteTexture(renderer, scale, SpriteKind::Chicken, ducking.w, ducking.h),
        CreateSpriteTexture(renderer, scale, SpriteKind::Pig, PIG_WIDTH, PIG_HEIGHT),
        CreateSpriteTexture(renderer, scale, SpriteKind::Tree, OBSTACLE_WIDTH, TREE_HEIGHT),
        CreateSpriteTexture(renderer, scale, SpriteKind::Poop, OBSTACLE_WIDTH, POOP_HEIGHT),
        CreateSpriteTexture(renderer, scale, SpriteKind::Egg, BULLET_WIDTH, BULLET_HEIGHT),
    };
}

void DestroySprites(Sprites& sprites) {
    for (SDL_Texture* t : {sprites.chicken, sprites.chickenSquashed, sprites.pig, sprites.tree,
                           sprites.poop, sprites.egg}) {
        if (t) SDL_DestroyTexture(t);
    }
    sprites = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
}

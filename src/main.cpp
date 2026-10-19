#include "audio.h"
#include "game.h"
#include "render.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

const int MAX_TICKS_PER_FRAME = 5;

// World width matching the renderer's aspect ratio, so the view fills the
// window (and a widescreen fullscreen) instead of showing bars on the sides
int WorldWidthFor(SDL_Renderer* renderer) {
    int w = SCREEN_WIDTH, h = SCREEN_HEIGHT;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    if (h <= 0) return SCREEN_WIDTH;
    long width = std::lround(static_cast<double>(SCREEN_HEIGHT) * w / h);
    return static_cast<int>(std::min<long>(MAX_SCREEN_WIDTH, std::max<long>(SCREEN_WIDTH, width)));
}

// Fits the world to the window and recreates the graphics for its size
Graphics SetUpView(SDL_Renderer* renderer, GameState& game) {
    int width = WorldWidthFor(renderer);
    SDL_RenderSetLogicalSize(renderer, width, SCREEN_HEIGHT);
    game.width = width;
    return CreateGraphics(renderer, width);
}

// Initial window scale: the largest that fits 90% of the display's usable
// area, between 1x and 3x the world size
float InitialWindowScale() {
    SDL_Rect bounds;
    if (SDL_GetDisplayUsableBounds(0, &bounds) != 0) return 1.5f;
    float scale = std::min(bounds.w * 0.9f / SCREEN_WIDTH, bounds.h * 0.9f / SCREEN_HEIGHT);
    return std::max(1.0f, std::min(scale, 3.0f));
}

// argc/argv are required so SDL_main works on Windows
int main(int argc, char* argv[]) {
    // --frames N: quit after N frames (smoke tests; combine with
    // SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy to run without a display)
    long maxFrames = -1;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--frames" && i + 1 < argc) {
            maxFrames = std::strtol(argv[++i], nullptr, 10);
        } else {
            std::cerr << "Usage: " << argv[0] << " [--frames N]" << std::endl;
            return 2;
        }
    }

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Initialize SDL and fonts; audio is optional
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    Audio audio = OpenAudio();

    // Create a resizable window scaled to the display, and a renderer that
    // works in world units and scales them to the window keeping the aspect
    // ratio (fall back to software rendering)
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    float windowScale = InitialWindowScale();
    SDL_Window* window = SDL_CreateWindow("Saltarello", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          static_cast<int>(SCREEN_WIDTH * windowScale),
                                          static_cast<int>(SCREEN_HEIGHT * windowScale),
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* renderer = nullptr;
    if (window) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    // World size, font and sprites for the window's size and pixel density
    GameState game = NewGame();
    Graphics graphics = renderer ? SetUpView(renderer, game) : Graphics{SCREEN_WIDTH, 1, nullptr, {}};

    if (!window || !renderer || !graphics.font) {
        if (!window || !renderer) {
            std::cerr << "Failed to create window/renderer: " << SDL_GetError() << std::endl;
        } else {
            std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        }
        DestroyGraphics(graphics);
        CloseAudio(audio);
        TTF_Quit();
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    const Sprites& sprites = graphics.sprites;
    if (!sprites.chicken || !sprites.chickenSquashed || !sprites.pig || !sprites.tree ||
        !sprites.poop || !sprites.egg) {
        std::cerr << "Failed to create sprites: " << SDL_GetError() << std::endl;
    }

    bool onTitle = true;  // the key legend shows until ENTER starts the game
    bool cursorHidden = false;
    bool quit = false;
    long frames = 0;
    SDL_Event e;

    // Fixed timestep: the game advances in ticks of 1 / TICKS_PER_SECOND seconds
    // of real time, however long each frame takes to draw.
    const double counterFreq = static_cast<double>(SDL_GetPerformanceFrequency());
    const double tickSeconds = 1.0 / TICKS_PER_SECOND;
    Uint64 previousCounter = SDL_GetPerformanceCounter();
    double accumulator = tickSeconds; // run the first tick right away

    // Main game loop
    while (!quit && (maxFrames < 0 || frames++ < maxFrames)) {
        Uint64 counter = SDL_GetPerformanceCounter();
        accumulator += (counter - previousCounter) / counterFreq;
        previousCounter = counter;

        // Avoid a burst of catch-up ticks after a stall (window drag, debugger...)
        if (accumulator > MAX_TICKS_PER_FRAME * tickSeconds) {
            accumulator = MAX_TICKS_PER_FRAME * tickSeconds;
        }

        // Event handling
        while (SDL_PollEvent(&e)) {
            // Quit game on window close
            if (e.type == SDL_QUIT) quit = true;

            // Redraw sprites and text for the new pixel size; also needed when
            // the renderer loses its textures (e.g. Direct3D device reset)
            bool resized = e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED;
            if (resized || e.type == SDL_RENDER_TARGETS_RESET || e.type == SDL_RENDER_DEVICE_RESET) {
                DestroyGraphics(graphics);
                graphics = SetUpView(renderer, game);
                if (!graphics.font) {
                    std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
                    quit = true;
                }
            }

            // Toggle fullscreen with F or F11 (the 4:3 world fills the height)
            if (e.type == SDL_KEYDOWN && !e.key.repeat &&
                (e.key.keysym.sym == SDLK_f || e.key.keysym.sym == SDLK_F11)) {
                bool fullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
                SDL_SetWindowFullscreen(window, fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
            }

            // Quit game on ESC key press
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) quit = true;

            // ENTER starts the game from the title screen, or restarts it after
            // game over
            bool enter = e.type == SDL_KEYDOWN &&
                         (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER);
            if (enter && (onTitle || game.gameOver)) {
                onTitle = false;
                game = NewGame(graphics.width);
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(NULL);
        Input input = {
            keys[SDL_SCANCODE_UP] != 0,
            keys[SDL_SCANCODE_DOWN] != 0,
            keys[SDL_SCANCODE_LEFT] != 0,
            keys[SDL_SCANCODE_RIGHT] != 0,
            keys[SDL_SCANCODE_X] != 0,
        };

        // Hide the mouse pointer only while playing
        bool playing = !onTitle && !game.gameOver;
        if (playing != cursorHidden) {
            SDL_ShowCursor(playing ? SDL_DISABLE : SDL_ENABLE);
            cursorHidden = playing;
        }

        while (accumulator >= tickSeconds) {
            accumulator -= tickSeconds;
            if (onTitle) continue;
            GameEvents events = UpdateGame(game, input);
            if (events.jumped) PlaySound(audio.jump);
            if (events.shot) PlaySound(audio.shoot);
            if (events.crashed) PlaySound(audio.collision);
        }

        if (onTitle) {
            RenderTitle(renderer, graphics);
        } else {
            RenderGame(renderer, graphics, game);
        }
        SDL_RenderPresent(renderer);

        // Sleep until the next tick is due; drawing more often than the game
        // updates would only show the same frame again
        double untilNextTick = tickSeconds - accumulator
            - (SDL_GetPerformanceCounter() - previousCounter) / counterFreq;
        if (untilNextTick > 0) SDL_Delay(static_cast<Uint32>(std::ceil(untilNextTick * 1000)));
    }

    DestroyGraphics(graphics);
    CloseAudio(audio);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

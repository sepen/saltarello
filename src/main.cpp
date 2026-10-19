#include "assets.h"
#include "audio.h"
#include "game.h"
#include "render.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

const int MAX_TICKS_PER_FRAME = 5;

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

    // Create SDL window and renderer (fall back to software rendering)
    SDL_Window* window = SDL_CreateWindow("Saltarello", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = nullptr;
    if (window) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    // Load font for rendering text
    TTF_Font* font = renderer ? TTF_OpenFont(AssetPath("font.ttf").c_str(), 24) : nullptr;

    if (!window || !renderer || !font) {
        if (!window || !renderer) {
            std::cerr << "Failed to create window/renderer: " << SDL_GetError() << std::endl;
        } else {
            std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        }
        CloseAudio(audio);
        TTF_Quit();
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    GameState game = NewGame();
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

            // Quit game on ESC key press
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) quit = true;

            // Restart game on ENTER key press after game over
            if (game.gameOver && e.type == SDL_KEYDOWN &&
                (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)) {
                game = NewGame();
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

        while (accumulator >= tickSeconds) {
            GameEvents events = UpdateGame(game, input);
            if (events.jumped) PlaySound(audio.jump);
            if (events.shot) PlaySound(audio.shoot);
            if (events.crashed) PlaySound(audio.collision);
            accumulator -= tickSeconds;
        }

        RenderGame(renderer, font, game);
        SDL_RenderPresent(renderer);

        // Sleep until the next tick is due; drawing more often than the game
        // updates would only show the same frame again
        double untilNextTick = tickSeconds - accumulator
            - (SDL_GetPerformanceCounter() - previousCounter) / counterFreq;
        if (untilNextTick > 0) SDL_Delay(static_cast<Uint32>(std::ceil(untilNextTick * 1000)));
    }

    CloseAudio(audio);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

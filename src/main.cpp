#include "assets.h"
#include "audio.h"
#include "game.h"
#include "render.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <cstdlib>
#include <ctime>
#include <iostream>

const Uint32 FRAME_MS = 16; // ~60 FPS

// argc/argv are required so SDL_main works on Windows
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

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
    SDL_Event e;

    // Main game loop
    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();

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

        GameEvents events = UpdateGame(game, input, currentTime);
        if (events.jumped) PlaySound(audio.jump);
        if (events.shot) PlaySound(audio.shoot);
        if (events.crashed) PlaySound(audio.collision);

        RenderGame(renderer, font, game);
        SDL_RenderPresent(renderer);

        // Cap the frame rate without depending on the display refresh rate
        Uint32 elapsed = SDL_GetTicks() - currentTime;
        if (elapsed < FRAME_MS) SDL_Delay(FRAME_MS - elapsed);
    }

    CloseAudio(audio);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

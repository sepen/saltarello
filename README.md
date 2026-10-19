# Saltarello

[![Build](https://github.com/sepen/saltarello/actions/workflows/build.yml/badge.svg)](https://github.com/sepen/saltarello/actions/workflows/build.yml)

## Table of Contents
- [Project Overview](#project-overview)
- [Features](#features)
- [How it Works](#how-it-works)
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Project Overview
Saltarello is a 2D endless runner built with SDL2, inspired by Chrome's offline dinosaur game. Instead of a dinosaur you play a chicken that jumps over smiling trees and smiling poops, ducks under flying pigs and throws eggs. The objective is to keep running as long as possible while scoring points and avoiding collisions.

## Features
- **Jumping Mechanics**: Players can jump over obstacles using the UP arrow key.
- **Shooting**: The chicken throws eggs with the X key to break obstacles (+1 point, like dodging them). It carries 3 eggs, shown top-right, and gets one back every 2 seconds.
- **Dynamic Obstacles**: Smiling trees (tall), smiling poops (short) and, from 10 points on, a flying pig at head height. The game speeds up as the score rises.
- **Score Tracking**: Players earn points by avoiding obstacles and can see their score displayed on the screen.
- **Game Over State**: The game ends when the player collides with an obstacle, with an option to restart.

## How it Works
Saltarello uses the SDL2 library for graphics, audio, and input handling. The game initializes a window and renderer, loads necessary assets (fonts, sounds, and music), and enters a main game loop that handles events, updates game state, and renders graphics. The player character can move left and right, jump, and shoot bullets to interact with obstacles. The game dynamically generates obstacles based on the player's score, increasing the challenge as the game progresses.

The code lives in `src/`:

| File | Responsibility |
|------|----------------|
| `game.{h,cpp}` | Game state and rules. No SDL dependency: takes an `Input`, returns `GameEvents` |
| `render.{h,cpp}` | Draws a frame from the `GameState`; scales the 800x600 world to the window |
| `sprites.{h,cpp}` | Draws all sprites (chicken, flying pig, tree, poop, egg) in code at startup |
| `audio.{h,cpp}` | Loads and plays sounds and music (optional) |
| `assets.{h,cpp}` | Locates files under `assets/` |
| `main.cpp` | SDL setup, event loop and frame timing |

## Installation
To run Saltarello, follow these steps:

1. **Install dependencies**:
   Ensure you have SDL2, SDL2_mixer, and SDL2_ttf installed. You can install them via your package manager.

   For example, on Ubuntu:
   ```bash
   sudo apt-get install build-essential autoconf automake pkg-config libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev
   ```
   On macOS:
   ```bash
   brew install autoconf automake pkg-config sdl2 sdl2_mixer sdl2_ttf
   ```

2. **Build the project** (requires autoconf, automake and pkg-config):
   ```bash
   autoreconf -fi
   ./configure
   make
   ```
   `make check` runs the unit tests for the game rules (`tests/`) and a smoke test
   that starts the game without a display and runs 60 frames. CI builds and runs it on Linux, macOS and Windows (MSYS2).

   Optional: `./configure --enable-asan` builds with AddressSanitizer. It works on
   Linux; on macOS it aborts at startup because Homebrew's `sdl2` (`sdl2-compat`)
   cannot be loaded under ASan.

3. **Run the application**:
   ```bash
   ./saltarello
   ```
   Assets are looked up next to the executable, in `./assets`, and in the install
   directory (`$prefix/share/saltarello/assets`), so `make install` also works.

   `./saltarello --frames N` quits after N frames; with `SDL_VIDEODRIVER=dummy` and
   `SDL_AUDIODRIVER=dummy` it runs headless.

## Usage
- Press the `UP` arrow key to jump. Tap it for a short hop, hold it to jump higher.
  A press just before landing is remembered, so you can queue the next jump.
- Hold the `DOWN` arrow key to squash the chicken and pass under flying pigs (only on
  the ground). The squashed chicken is wider, so watch the sides.
- Use the `LEFT` and `RIGHT` arrow keys to move the player.
- Press the `X` key to throw an egg (3 at most, recharging over time).
- Avoid obstacles to keep playing and increase your score.
- Press `ENTER` to restart the game after a game over.
- Press `F` or `F11` to toggle fullscreen.
- Press `ESC` to quit.

The game is 4:3, so it fills a fullscreen window. The window starts as large as fits
your screen and can be resized; the game keeps its proportions and redraws its
sprites and text at the new size.

## Contributing
Contributions are welcome! If you would like to contribute to Saltarello, please follow these steps:
1. Fork the repository.
2. Create a new branch (`git checkout -b feature/YourFeature`).
3. Make your changes and commit them (`git commit -m 'Add some feature'`).
4. Push to the branch (`git push origin feature/YourFeature`).
5. Open a pull request.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.


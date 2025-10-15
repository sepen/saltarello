# Saltarello

## Table of Contents
- [Project Overview](#project-overview)
- [Features](#features)
- [How it Works](#how-it-works)
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Project Overview
Saltarello is a 2D platformer game built using the SDL2 library. The game combines elements of jumping, shooting, and obstacle avoidance, providing an engaging experience for players. The objective is to navigate through a series of obstacles while scoring points and avoiding collisions.

## Features
- **Jumping Mechanics**: Players can jump over obstacles using the UP arrow key.
- **Shooting**: Players can shoot bullets using the X key to destroy obstacles.
- **Dynamic Obstacles**: The game features various types of obstacles, including stones and birds, that increase in difficulty as the player's score rises.
- **Score Tracking**: Players earn points by avoiding obstacles and can see their score displayed on the screen.
- **Game Over State**: The game ends when the player collides with an obstacle, with an option to restart.

## How it Works
Saltarello uses the SDL2 library for graphics, audio, and input handling. The game initializes a window and renderer, loads necessary assets (fonts, sounds, and music), and enters a main game loop that handles events, updates game state, and renders graphics. The player character can move left and right, jump, and shoot bullets to interact with obstacles. The game dynamically generates obstacles based on the player's score, increasing the challenge as the game progresses.

## Installation
To run Saltarello, follow these steps:

1. **Install dependencies**:
   Ensure you have SDL2, SDL2_mixer, and SDL2_ttf installed. You can install them via your package manager.

   For example, on Ubuntu:
   ```bash
   sudo apt-get install libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev
   ```
   On macOS:
   ```bash
   brew install sdl2 sdl2_mixer sdl2_ttf
   ```

2. **Build the project**:
   ```bash
   ./configure
   make
   ```

3. **Run the application**:
   ```bash
   ./melontron
   ```

## Usage
- Use the `UP` arrow key to jump.
- Use the `LEFT` and `RIGHT` arrow keys to move the player.
- Press the `X` key to shoot bullets.
- Avoid obstacles to keep playing and increase your score.
- Press `ENTER` to restart the game after a game over.

## Contributing
Contributions are welcome! If you would like to contribute to Saltarello, please follow these steps:
1. Fork the repository.
2. Create a new branch (`git checkout -b feature/YourFeature`).
3. Make your changes and commit them (`git commit -m 'Add some feature'`).
4. Push to the branch (`git push origin feature/YourFeature`).
5. Open a pull request.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.


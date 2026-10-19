# TODO

Roadmap for Saltarello, in the order we plan to tackle it. Each item lands as
its own commit on `main`. Tick the box in that same commit.

## 1. Foundations

- [x] **Split `saltarello.cpp` into modules** — `game.{h,cpp}` (state and update),
  `render.cpp`, `audio.cpp`, `main.cpp`. `GameState` already groups the state,
  so this is mostly moving code.
- [ ] **CI on GitHub Actions** — build on Linux, macOS and Windows (MSYS2) on every
  push and PR, so portability is checked continuously instead of only on one Mac.

## 2. Game feel

- [ ] **Frame-rate independent physics** — movement is currently per frame, so a slow
  frame slows the game down. Use a fixed timestep driven by real elapsed time.
- [ ] **Real ducking** — holding DOWN only makes the bird ignore the player. Halve the
  player's height while ducking so it is visible and the collision is honest.
- [ ] **Minimum gap between obstacles** — respawn uses `rand() % 300` and obstacles can
  end up stacked, sometimes impossible to clear. Guarantee a minimum gap that grows
  with speed.
- [ ] **Variable jump height** — releasing UP early cuts the jump short.
- [ ] **Coyote time and jump buffering** — a jump pressed a few milliseconds before
  landing (or just after leaving the ground) still counts.

## 3. Mechanics

- [ ] **Rebalance shooting** — shooting costs 1 point with a 1 s cooldown and barely
  pays off. Options: limited ammo that recharges, or +2 points per destroyed obstacle.
- [ ] **Bullets hit birds** — they currently pass through.
- [ ] **More birds at different heights** — some must be jumped, some ducked.
- [ ] **Simple power-ups** — one-hit shield, double jump, slow motion.

## 4. Presentation

- [ ] **Persistent high score** — stored under `SDL_GetPrefPath()` (Application Support
  on macOS, AppData on Windows, `~/.local/share` on Linux).
- [ ] **Title screen and pause** — P key and pause on focus loss; replace the
  `gameOver` flag with a `Menu / Playing / Paused / GameOver` state.
- [ ] **Better text rendering** — `TTF_RenderText_Blended` for smooth edges, and cache
  the score texture instead of creating it every frame.
- [ ] **Parallax background** — 2–3 layers of hills/clouds for a sense of speed.
- [ ] **Juice** — screen shake and particles on collisions and destroyed obstacles.
- [ ] **Sprites instead of rectangles** — via SDL2_image, or at least simple animation
  (squash on landing, blink on death).

## 5. Input

- [ ] **Game controller support** — `SDL_GameController`, plus alternative keys
  (WASD / Space).

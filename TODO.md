# TODO

Roadmap for Saltarello, in the order we plan to tackle it. Each item lands as
its own commit on `main`. Tick the box in that same commit.

## 1. Foundations

- [x] **Split `saltarello.cpp` into modules** — `game.{h,cpp}` (state and update),
  `render.cpp`, `audio.cpp`, `main.cpp`. `GameState` already groups the state,
  so this is mostly moving code.
- [x] **CI on GitHub Actions** — build on Linux, macOS and Windows (MSYS2) on every
  push and PR, so portability is checked continuously instead of only on one Mac.

## 2. Game feel

- [x] **Frame-rate independent physics** — movement is currently per frame, so a slow
  frame slows the game down. Use a fixed timestep driven by real elapsed time.
- [x] **Real ducking** — holding DOWN only makes the pig ignore the player. Halve the
  player's height while ducking so it is visible and the collision is honest.
- [x] **Minimum gap between obstacles** — respawn uses `rand() % 300` and obstacles can
  end up stacked, sometimes impossible to clear. Guarantee a minimum gap that grows
  with speed.
- [x] **Variable jump height** — releasing UP early cuts the jump short.
- [x] **Jump buffering** — a jump pressed a few milliseconds before landing still
  counts. (Coyote time dropped: the ground is flat, there are no ledges to run off.)

## 3. Mechanics

- [x] **Rebalance shooting** — throwing is free with 3 eggs that recharge (1 every
  2 s, shown top-right), and breaking an obstacle scores +1 like dodging it.
- [x] **Eggs hit pigs** — +1 and the pig comes back later. The pig flies above the
  beak, so it has to be hit mid-jump.
- [x] **More pigs at different heights** — pigs are part of the obstacle stream (1 in 3
  from 10 points on), flying low (jump), at head height (duck) or high (only hits
  mid-jump).
- [ ] **Simple power-ups** — one-hit shield, double jump, slow motion.

## 4. Presentation

- [ ] **Persistent high score** — stored under `SDL_GetPrefPath()` (Application Support
  on macOS, AppData on Windows, `~/.local/share` on Linux).
- [x] **Title screen** — key legend; ENTER starts. The mouse pointer is hidden while
  playing.
- [ ] **Pause** — P key and pause on focus loss.
- [ ] **Cache text textures** — the score texture is created every frame. (Smooth,
  resolution-aware text with `TTF_RenderText_Blended` is done.)
- [ ] **Parallax background** — 2–3 layers of hills/clouds for a sense of speed.
- [ ] **Juice** — screen shake and particles on collisions and destroyed obstacles.
- [x] **Sprites instead of rectangles** — chicken, flying pig, smiling tree, smiling
  poop and eggs, drawn in code (`src/sprites.cpp`).
- [ ] **Simple animation** — squash on landing, flapping pig wings, blink on death.

## 5. Input

- [ ] **Game controller support** — `SDL_GameController`, plus alternative keys
  (WASD / Space).

# Soccer Engine

A 6v6 soccer simulation written in C with SDL2. The engine provides physics, rendering, and match flow; this repository implements the **referee** rule layer and **coach** player AI on top of that core.

Originally developed as a Fundamentals of Programming (FOP) course project (Fall 2025).

---

## Project Overview

The simulation runs a continuous 6-a-side match between two teams (red / left and blue / right). Each player is a small state machine (`IDLE`, `MOVING`, `SHOOTING`, `INTERCEPTING`) driven by coach-assigned logic functions. A separate referee module validates talents, movement, shooting, and match events such as goals and out-of-bounds.

The executable builds as `soccerengine` via CMake and opens an SDL2 window that draws the pitch, players, ball, and score.

---

## Screenshots / Gameplay

![Gameplay](assets/screenshots/gameplay-1.png)

![Gameplay](assets/screenshots/gameplay-2.png)

---

## Features

- 6v6 match simulation with team scores and timed match flow
- Custom 2D kinematics for players and the ball (velocity, friction, collisions)
- Role-based player AI (attackers, midfielders, defenders, goalkeeper)
- Ball possession tracking and interception logic
- Referee checks for goals, outs, talent limits, speed caps, and kickoff direction
- SDL2 rendering with team icons and on-screen text (embedded font)

---

## Referee System

Implemented in `engine/logic/referee.c`. The referee runs as part of the simulation update and enforces match rules separately from physics.

| Area | What is implemented |
| --- | --- |
| **Goals** | Detects when the entire ball crosses a goal line inside the goal mouth; updates the scoring team and returns `GOAL` |
| **Out of bounds** | Detects when the ball is fully outside the pitch (excluding valid goal entries) and returns `OUT` |
| **Talent validation** | `verify_talents` checks each skill is in range and the total does not exceed `MAX_TALENT_PER_PLAYER` |
| **State checks** | `verify_state` prevents `SHOOTING` unless the player currently possesses the ball |
| **Movement limits** | `verify_movement` clamps player velocity to a max derived from the Agility talent |
| **Shoot / kickoff** | `verify_shoot` clamps ball speed by the Shooting talent and requires kickoff passes into the team's own half |

---

## Coach AI

Implemented in `engine/logic/coach.c`. The coach assigns per-player function pointers for movement, shooting, and state changes, plus talents and kickoff positions.

| Area | What is implemented |
| --- | --- |
| **Role-based movement** | Kit roles drive different patterns: attackers/midfielders push up; defenders hold shape; kit `3` is the goalkeeper and stays in a box near goal |
| **Interception** | Players switch to `INTERCEPTING` when the free or opposed ball is within a role-dependent radius |
| **Passing & shooting** | Possessors choose between intelligent passes to open teammates and shots toward goal corners (including one-on-one finishes) |
| **Defensive positioning** | Defenders position between the ball/opponent and their own goal rather than always chasing the ball |
| **Kickoff & restarts** | Kickoff direction is forced into the own half; restart/out kicks are constrained near boundaries |
| **Talents & formation** | Per-player talent distributions and kickoff positions are defined for both teams |

By default, `coach_both_teams` is enabled so the same AI logic drives both sides.

---

## Tech Stack

| Component | Details |
| --- | --- |
| Language | C (C99) |
| Build | CMake ≥ 3.20 |
| Graphics | SDL2, SDL_image, SDL_ttf |
| Physics | Custom 2D vectors (`engine/core/vec2`) |
| Dependencies | System SDL2 if available; otherwise fetched via CMake `FetchContent` |

---

## Project Structure

```
.
├── main.c                 # Entry point: init, game loop, render
├── CMakeLists.txt         # Build configuration
├── cmake/
│   └── LinkSDL2.cmake     # SDL2 discovery / FetchContent helper
└── engine/
    ├── assets/            # Fonts and player/app icons
    ├── core/              # Constants and 2D vector math
    ├── entities/          # Ball, player, team, field definitions
    ├── game/              # Scene update, match state, possession
    ├── graphics/          # SDL2 renderer
    └── logic/             # Referee rules and coach AI
```

Important modules:

- **`engine/core/`** — pitch layout, talent/speed limits, vector helpers  
- **`engine/entities/`** — `Player`, `Team`, `Ball`, and field types  
- **`engine/game/`** — scene lifecycle, possession, set pieces  
- **`engine/graphics/`** — window, drawing, scene update entry used by `main.c`  
- **`engine/logic/`** — `referee.c` (rules) and `coach.c` (AI factory)

---

## Build & Run

### Prerequisites

- CMake 3.20 or newer
- A C99 compiler (GCC, Clang, or MSVC with a compatible toolchain)
- `xxd` (used at build time to embed `DejaVuSans.ttf`)
- SDL2, SDL_image, and SDL_ttf (optional if you rely on CMake FetchContent)

### Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

The executable is written to `build/bin/soccerengine` (or `soccerengine.exe` on Windows). Player icons are copied to `build/bin/assets/icons/`.

### Run

From the `build` directory:

```bash
# Linux / macOS
./bin/soccerengine

# Windows
.\bin\soccerengine.exe
```

Close the window to quit. There is no separate CLI; the match starts automatically.

---

## Credits / Course Project

This project builds on the **Soccer Engine AI Challenge** framework provided for the FOP course.

Engine / assignment base by:

- [MatinB02](https://github.com/MatinB02)
- [Mani Ebrahimi](https://github.com/maniebra)

The referee and coach implementations in this repository complete the two course phases described above.

---

## Author

**Maani Safari** — student implementation (referee + coach AI)

Repository: [maani-safari/FOP-Project](https://github.com/maani-safari/FOP-Project)

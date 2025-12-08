#pragma once

#include <cstddef>

// Screen / world settings
inline constexpr int   SCREEN_WIDTH    = 800;
inline constexpr int   SCREEN_HEIGHT   = 800;

// Simulation settings
inline constexpr float SPAWN_DELAY     = 0.0005f;
inline constexpr int   PARTICLE_COUNT  = 5000;
inline constexpr std::size_t CELL_SIZE = 8;
inline constexpr int   GRID_COLS       = (SCREEN_WIDTH + CELL_SIZE - 1) / CELL_SIZE;
inline constexpr int   GRID_ROWS       = (SCREEN_HEIGHT + CELL_SIZE - 1) / CELL_SIZE;
inline constexpr int   ITERATIONS      = 4;

// Mouse interaction
inline constexpr float MOUSE_STRENGTH  = 5000.f;
inline constexpr float MOUSE_RADIUS    = 200.f;


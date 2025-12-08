#include "World.hpp"
#include <cmath>

// neighbour offsets (cell adjacency)
namespace {
    constexpr std::pair<int,int> ds[4] = {
        {1,0}, {0,1}, {1,1}, {1,-1}
    };
}

World::World()
    : startingVel_(0.f, 500.f)
    , goingUp_(false)
{
    particles_.reserve(PARTICLE_COUNT);
    Particle::GRAVITY = {0.f, 2000.f};
}

void World::spawnIfNeeded(float dt) {
    if (particles_.size() >= PARTICLE_COUNT) return;

    if (spawnClock_.getElapsedTime().asSeconds() >= SPAWN_DELAY) {
        sf::Vector2f startPos(static_cast<float>(SCREEN_WIDTH) / 2.f, 10.f);
        particles_.emplace_back(startPos, 3.f);

        auto& latest = particles_.back();
        latest.prev_position = latest.position - startingVel_ * dt;
        updateStartingVel();
        spawnClock_.restart();
    }
}

void World::update(float dt, const InputState& input) {
    applyInputGravity(input);

    const int   SubSteps = 4;
    const float subDt    = dt / static_cast<float>(SubSteps);

    for (int s = 0; s < SubSteps; ++s) {
        applyForcesAndIntegrate(subDt, input);
        rebuildGrid();

        for (int k = 0; k < ITERATIONS; ++k) {
            resolveCollisions();
        }
    }
}

void World::resolveCollision(Particle& a, Particle& b) {
    sf::Vector2f v = a.position - b.position;
    float square_dist = v.x * v.x + v.y * v.y;
    float min_dist    = a.radius + b.radius;

    if (square_dist == 0.f || square_dist >= min_dist * min_dist) {
        return;
    }

    float dist = std::sqrt(square_dist);
    sf::Vector2f normal  = v / dist;
    float overlap        = min_dist - dist;
    const float stiffness = 0.85f;
    float correction     = overlap * 0.5f * stiffness;

    a.position += normal * correction;
    b.position -= normal * correction;
}

void World::updateStartingVel() {
    if (goingUp_) {
        if (startingVel_.x + 25.f < 500.f) {
            startingVel_.x += 25.f;
        } else {
            startingVel_.x = 500.f;
            goingUp_ = false;
        }
    } else {
        if (startingVel_.x - 25.f > -500.f) {
            startingVel_.x -= 25.f;
        } else {
            startingVel_.x = -500.f;
            goingUp_ = true;
        }
    }
}

void World::applyInputGravity(const InputState& input) {
    if (input.leftPressed) {
        Particle::GRAVITY = {-2000.f, 0.f};
    } else if (input.downPressed) {
        Particle::GRAVITY = {0.f, 2000.f};
    } else if (input.rightPressed) {
        Particle::GRAVITY = {2000.f, 0.f};
    } else if (input.upPressed) {
        Particle::GRAVITY = {0.f, -2000.f};
    }
}

void World::applyMouseForce(Particle& particle, const InputState& input,
                            int mouseCellX, int mouseCellY, int mouseCellRadius)
{
    int pcx = static_cast<int>(particle.position.x / CELL_SIZE);
    int pcy = static_cast<int>(particle.position.y / CELL_SIZE);

    if (pcx < mouseCellX - mouseCellRadius || pcx > mouseCellX + mouseCellRadius ||
        pcy < mouseCellY - mouseCellRadius || pcy > mouseCellY + mouseCellRadius) {
        return;
    }

    sf::Vector2f dir = input.mousePos - particle.position;
    float distSq = dir.x * dir.x + dir.y * dir.y;

    if (distSq > MOUSE_RADIUS * MOUSE_RADIUS || distSq < 1e-3f) {
        return;
    }

    float dist = std::sqrt(distSq);
    sf::Vector2f normalized = dir / dist;

    particle.acceleration += normalized * MOUSE_STRENGTH;
}

void World::applyForcesAndIntegrate(float subDt, const InputState& input) {
    int mouseCellX = 0;
    int mouseCellY = 0;
    int mouseCellRadius = 0;

    if (input.mouseHeld) {
        mouseCellX      = static_cast<int>(input.mousePos.x / CELL_SIZE);
        mouseCellY      = static_cast<int>(input.mousePos.y / CELL_SIZE);
        mouseCellRadius = static_cast<int>(MOUSE_RADIUS / CELL_SIZE) + 1;
    }

    for (auto& particle : particles_) {
        if (input.mouseHeld) {
            applyMouseForce(particle, input, mouseCellX, mouseCellY, mouseCellRadius);
        }

        particle.applyGravity();
        particle.integrate(subDt);
        particle.applyBounds(SCREEN_HEIGHT, SCREEN_WIDTH);
    }
}

void World::rebuildGrid() {
    // clear
    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            grid_[row][col].clear();
        }
    }

    // fill
    for (int i = 0; i < static_cast<int>(particles_.size()); ++i) {
        const auto& p = particles_[i];
        int cx = static_cast<int>(p.position.x / CELL_SIZE);
        int cy = static_cast<int>(p.position.y / CELL_SIZE);

        if (cx < 0) cx = 0;
        if (cy < 0) cy = 0;
        if (cx >= GRID_COLS) cx = GRID_COLS - 1;
        if (cy >= GRID_ROWS) cy = GRID_ROWS - 1;

        grid_[cy][cx].push_back(i);
    }
}

void World::resolveCollisions() {
    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            auto& cell = grid_[row][col];
            if (cell.empty()) continue;

            // within cell
            for (int a = 0; a < static_cast<int>(cell.size()); ++a) {
                for (int b = a + 1; b < static_cast<int>(cell.size()); ++b) {
                    resolveCollision(particles_[cell[a]], particles_[cell[b]]);
                }
            }

            // neighbours
            for (auto [dr, dc] : ds) {
                int nr = row + dr;
                int nc = col + dc;
                if (nc < 0 || nc >= GRID_COLS || nr < 0 || nr >= GRID_ROWS) continue;

                auto& neighbour = grid_[nr][nc];
                if (neighbour.empty()) continue;

                for (int a : cell) {
                    for (int b : neighbour) {
                        resolveCollision(particles_[a], particles_[b]);
                    }
                }
            }
        }
    }
}


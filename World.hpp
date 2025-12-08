#pragma once

#include <vector>
#include <SFML/Graphics.hpp>

#include "Config.hpp"
#include "Particle.hpp"

struct InputState {
    bool        mouseHeld    = false;
    sf::Vector2f mousePos    {0.f, 0.f};
    bool        downPressed  = false;
    bool        leftPressed  = false;
    bool        rightPressed = false;
    bool        upPressed    = false;
};

class World {
public:
    World();

    std::vector<Particle>& particles() { return particles_; }
    std::size_t particleCount() const { return particles_.size(); }

    // dt is in seconds (e.g. 1/60.f)
    void spawnIfNeeded(float dt);
    void update(float dt, const InputState& input);

private:
    std::vector<Particle> particles_;
    std::vector<int> grid_[GRID_ROWS][GRID_COLS];

    sf::Vector2f startingVel_;
    bool         goingUp_;
    sf::Clock    spawnClock_;

    // helpers
    void resolveCollision(Particle& a, Particle& b);
    void updateStartingVel();
    void applyInputGravity(const InputState& input);
    void applyMouseForce(Particle& particle, const InputState& input,
                         int mouseCellX, int mouseCellY, int mouseCellRadius);
    void applyForcesAndIntegrate(float subDt, const InputState& input);
    void rebuildGrid();
    void resolveCollisions();
};


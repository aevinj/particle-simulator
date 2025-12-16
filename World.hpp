
#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <string>
#include <chrono>
#include <iostream>
#include <algorithm>

#include "Config.hpp"
#include "Particle.hpp"
#include "ParticleRenderer.hpp"

class Timer {
    using Clock = std::chrono::steady_clock;
public:
    static std::int64_t nowUs() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now().time_since_epoch()
        ).count();
    }
};

class World {
private:
    const int PARTICLE_COUNT;
    const int SUBSTEPS;

    static const int CELL_SIZE = 4;
    static constexpr int GRID_COLS = (SCREEN_WIDTH  + CELL_SIZE - 1) / CELL_SIZE;
    static constexpr int GRID_ROWS = (SCREEN_HEIGHT + CELL_SIZE - 1) / CELL_SIZE;

    const float MOUSE_RADIUS   = 100.f;
    const float MOUSE_STRENGTH = 5000.f;
    const float SPAWN_DELAY    = 0.00005f;

    const sf::Vector2f startPos = {static_cast<float>(SCREEN_WIDTH) / 2.f, 10.f};
    sf::Vector2f startingVel = {0.f, 500.f};
    bool goingUp = true;

    const float dt = 1.f / 60.f;

    ParticleRenderer renderer = ParticleRenderer(PARTICLE_COUNT);

    std::vector<int> grid[GRID_ROWS][GRID_COLS];

    inline bool inBoundsCell(int cx, int cy) const {
        return (cx >= 0 && cy >= 0 && cx < GRID_COLS && cy < GRID_ROWS);
    }

    void resolveCollision(Particle& a, Particle& b) {
        sf::Vector2f v = a.position - b.position;
        float dist2 = v.x * v.x + v.y * v.y;
        float min_dist = a.radius + b.radius;

        if (dist2 < 1e-12f) { v = {1.f, 0.f}; dist2 = 1.f; }

        float min2 = min_dist * min_dist;
        if (dist2 >= min2) return; // not overlapping

        float dist = std::sqrt(dist2);

        float delta = 0.25f * (min_dist - dist);
        sf::Vector2f n = (v / dist) * delta;

        a.position += n;
        b.position -= n;
    }

    void clearGrid() {
        for (int y = 0; y < GRID_ROWS; ++y)
            for (int x = 0; x < GRID_COLS; ++x)
                grid[y][x].clear();
    }

    void buildGrid() {
        clearGrid();

        for (int i = 0; i < (int)particles.size(); ++i) {
            const auto& p = particles[i];
            int cx = (int)(p.position.x / CELL_SIZE);
            int cy = (int)(p.position.y / CELL_SIZE);
            if (!inBoundsCell(cx, cy)) continue;
            grid[cy][cx].push_back(i);
        }
    }

    void checkCollisionsForward() {
        static const int ndx[4] = {  1,  0,  1, -1 };
        static const int ndy[4] = {  0,  1,  1,  1 };

        for (int y = 0; y < GRID_ROWS; ++y) {
            for (int x = 0; x < GRID_COLS; ++x) {
                auto& cell = grid[y][x];
                const std::size_t n = cell.size();
                if (n == 0) continue;

                if (n >= 2) {
                    for (std::size_t i = 0; i < n; ++i) {
                        int aIdx = cell[i];
                        for (std::size_t j = i + 1; j < n; ++j) {
                            int bIdx = cell[j];
                            resolveCollision(particles[aIdx], particles[bIdx]);
                        }
                    }
                }

                for (int k = 0; k < 4; ++k) {
                    int nx = x + ndx[k];
                    int ny = y + ndy[k];
                    if (!inBoundsCell(nx, ny)) continue;

                    auto& ncell = grid[ny][nx];
                    if (ncell.empty()) continue;

                    for (int aIdx : cell) {
                        for (int bIdx : ncell) {
                            resolveCollision(particles[aIdx], particles[bIdx]);
                        }
                    }
                }
            }
        }
    }

    void updateStartingVel() {
        if (goingUp) {
            if (startingVel.x + 25.f < 500.f) startingVel.x += 25.f;
            else { startingVel.x = 500.f; goingUp = false; }
        } else {
            if (startingVel.x - 25.f > -500.f) startingVel.x -= 25.f;
            else { startingVel.x = -500.f; goingUp = true; }
        }
    }

    void handleMouseHeld(Particle& particle, const int cx, const int cy, const sf::Vector2f& mousePos) {
        int pcx = static_cast<int>(particle.position.x / CELL_SIZE);
        int pcy = static_cast<int>(particle.position.y / CELL_SIZE);

        if (pcx <= cx + (MOUSE_RADIUS / CELL_SIZE) &&
            pcx >= cx - (MOUSE_RADIUS / CELL_SIZE) &&
            pcy <= cy + (MOUSE_RADIUS / CELL_SIZE) &&
            pcy >= cy - (MOUSE_RADIUS / CELL_SIZE)) {

            sf::Vector2f dir = mousePos - particle.position;
            float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (dist < 1e-12f) return;

            sf::Vector2f normalized = dir / dist;
            particle.acceleration += normalized * MOUSE_STRENGTH;
        }
    }

public:
    std::vector<Particle> particles;

    World(const int count, const int substeps)
        : PARTICLE_COUNT(count)
        , SUBSTEPS(substeps)
    {
        particles.reserve(count);
    }

    void spawnIfPossible(const float elapsed_time, sf::Clock& spawner) {
        if (elapsed_time >= SPAWN_DELAY && particles.size() < (size_t)PARTICLE_COUNT) {
            sf::Color color(rand() % 255, rand() % 255, rand() % 255);
            const float substep_dt = dt / static_cast<float>(SUBSTEPS);

            sf::Vector2f v(-100.f, 0.f);

            if (particles.size() + 21 > (size_t)PARTICLE_COUNT) {
                int diff = (int)PARTICLE_COUNT - (int)particles.size();
                for (int i = 0; i < diff; ++i) particles.emplace_back(startPos, 2.f, color);

                for (int i = (int)particles.size() - diff; i < (int)particles.size(); ++i)
                    particles[i].prev_position = particles[i].position - startingVel * substep_dt;

            } else {
                for (int i = 0; i < 21; ++i) {
                    particles.emplace_back(startPos + v, 2.f, color);
                    v.x += 10;
                }

                for (int i = (int)particles.size() - 21; i < (int)particles.size(); ++i)
                    particles[i].prev_position = particles[i].position - startingVel * substep_dt;
            }

            updateStartingVel();
            spawner.restart();
        }
    }

    void update(InputState& inpState) {
        if (particles.empty()) return;

        const float substep_dt = dt / static_cast<float>(SUBSTEPS);
        const float dampening  = 0.8f;
        const float padding    = static_cast<float>(CELL_SIZE);

        buildGrid();

        for (int s = 0; s < SUBSTEPS; ++s) {
            int mx = 0, my = 0, rCells = 0;
            if (inpState.mouseHeld) {
                mx = std::max(0, std::min((int)(inpState.mousePos.x / CELL_SIZE), GRID_COLS - 1));
                my = std::max(0, std::min((int)(inpState.mousePos.y / CELL_SIZE), GRID_ROWS - 1));
                rCells = (int)(MOUSE_RADIUS / CELL_SIZE) + 1;
            }

            for (auto& p : particles) {
                p.accelerate(Particle::GRAVITY);
            }

            if (inpState.mouseHeld) {
                int x0 = std::max(0, mx - rCells);
                int x1 = std::min(GRID_COLS - 1, mx + rCells);
                int y0 = std::max(0, my - rCells);
                int y1 = std::min(GRID_ROWS - 1, my + rCells);

                for (int cy = y0; cy <= y1; ++cy) {
                    for (int cx = x0; cx <= x1; ++cx) {
                        auto& cell = grid[cy][cx];
                        for (int idx : cell) {
                            handleMouseHeld(particles[idx], mx, my, inpState.mousePos);
                        }
                    }
                }
            }

            checkCollisionsForward();

            for (auto& p : particles) {
                p.applyBorderBounce((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, padding, dampening);
            }

            for (auto& p : particles) {
                p.integrate(substep_dt);

                sf::Vector2f disp = p.getDisplacement();
                float disp2 = disp.x * disp.x + disp.y * disp.y;
                if (disp2 > 2.f * padding) {
                    p.prev_position = p.position;
                }
            }

            buildGrid();
        }
    }

    void draw(sf::RenderWindow& window) {
        renderer.build(particles);
        renderer.draw(window);
    }
};

class VisualText {
private:
    sf::Font font;
    sf::Text particleCount, timeBetweenFrames;

public:
    VisualText() {
        font.loadFromFile("../assets/arial.ttf");

        particleCount.setFont(font);
        particleCount.setString("--");
        particleCount.setCharacterSize(24);
        particleCount.setFillColor(sf::Color::Black);

        timeBetweenFrames.setFont(font);
        timeBetweenFrames.setString("--");
        timeBetweenFrames.setCharacterSize(24);
        timeBetweenFrames.setFillColor(sf::Color::Black);
        timeBetweenFrames.setPosition(0.f, 30.f);
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(particleCount);
        window.draw(timeBetweenFrames);
    }

    void setParticle(std::string cnt) { particleCount.setString(cnt); }
    void setFrames(std::string cnt)   { timeBetweenFrames.setString(cnt + "ms"); }
};

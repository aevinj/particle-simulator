#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <string>
#include <chrono>
#include <iostream>
#include <stdint.h>

#include "Config.hpp"
#include "Particle.hpp"
#include "ParticleRenderer.hpp"

class Timer {
    using Clock = std::chrono::steady_clock;
    private:
        Clock::time_point start = Clock::now();

    public:
        std::chrono::microseconds getTime() {
            auto currTime = Clock::now();
            auto res = std::chrono::duration_cast<std::chrono::microseconds>(currTime - start);
            return res;
        }

        void restart() {
            start = Clock::now();
        }
};
        

class World {
    private:
        const int PARTICLE_COUNT;
        const int SUBSTEPS;
        static const int CELL_SIZE = 4;
        const float MOUSE_RADIUS = 100.f;
        const float MOUSE_STRENGTH = 5000.f;
        const float SPAWN_DELAY = 0.00005f; 
        const sf::Vector2f startPos = {static_cast<float>(SCREEN_WIDTH) / 2.f, 10.f};
        sf::Vector2f startingVel = {0.f, 500.f}; 
        bool goingUp = true;
        const float dt = 1.f / 60.f;
        static constexpr int GRID_COLS = (SCREEN_WIDTH + CELL_SIZE - 1) / CELL_SIZE;
        static constexpr int GRID_ROWS = (SCREEN_HEIGHT + CELL_SIZE - 1) / CELL_SIZE;
        std::vector<int> grid[GRID_ROWS][GRID_COLS];
        const std::pair<int, int> ds[4] = {
            {1,0},
            {0,1},
            {1,1},
            {1,-1}
        };
        ParticleRenderer renderer = ParticleRenderer(PARTICLE_COUNT);

        void resolveCollision(Particle& a, Particle& b) {
            sf::Vector2f v = a.position - b.position;
            float dist2 = v.x*v.x + v.y*v.y;
            float min_dist = a.radius + b.radius;

            if (dist2 < 1e-12f) { v = {1.f, 0.f}; dist2 = 1.f; }
            if (dist2 >= min_dist * min_dist) return;

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

        void updateGrid() {
            clearGrid();
            for (int i = 0; i < (int)particles.size(); ++i) {
                const auto& p = particles[i];
                int cx = (int)(p.position.x / CELL_SIZE);
                int cy = (int)(p.position.y / CELL_SIZE);
                if (cx < 0 || cy < 0 || cx >= GRID_COLS || cy >= GRID_ROWS) continue;
                grid[cy][cx].push_back(i);
            }
        }

        void updateStartingVel() {
            if (goingUp) {
                if (startingVel.x + 25.f < 500.f) {
                    startingVel.x += 25.f;
                } else {
                    startingVel.x = 500.f;
                    goingUp = false;
                }
            } else {
                if (startingVel.x - 25.f > -500.f) {
                    startingVel.x -= 25.f;
                } else {
                    startingVel.x = -500.f;
                    goingUp = true;
                }
            }
        }

        void handleMouseHeld(Particle &particle, const int cx, const int cy, const sf::Vector2f &mousePos) {
            int pcx = static_cast<int>(particle.position.x / CELL_SIZE);
            int pcy = static_cast<int>(particle.position.y / CELL_SIZE);

            if (pcx <= cx + (MOUSE_RADIUS / CELL_SIZE) &&
                pcx >= cx - (MOUSE_RADIUS / CELL_SIZE) &&
                pcy <= cy + (MOUSE_RADIUS / CELL_SIZE) &&
                pcy >= cy - (MOUSE_RADIUS / CELL_SIZE)) {


                sf::Vector2f dir = mousePos - particle.position;
                float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                sf::Vector2f normalized = dir / dist;

                particle.acceleration += normalized * MOUSE_STRENGTH;
            }
        }

        void collideCells(int x1, int y1, int x2, int y2) {
            auto& c1 = grid[y1][x1];
            auto& c2 = grid[y2][x2];
            for (int id1 : c1) {
                for (int id2 : c2) {
                    if (id1 == id2) continue;
                    resolveCollision(particles[id1], particles[id2]);
                }
            }
        }

        void checkCollisions() {
            static const int dx[5] = { 1, 1, 0, 0, -1 };
            static const int dy[5] = { 0, 1, 0, 1,  1 };

            for (int x = 0; x < GRID_COLS; ++x) {
                for (int y = 0; y < GRID_ROWS; ++y) {
                    if (grid[y][x].empty()) continue;
                    for (int k = 0; k < 5; ++k) {
                        int nx = x + dx[k];
                        int ny = y + dy[k];
                        if (nx < 0 || ny < 0 || nx >= GRID_COLS || ny >= GRID_ROWS) continue;
                        collideCells(x, y, nx, ny);
                    }
                }
            }
        }

    public:
        std::vector<Particle> particles;

        World(const int count, const int substeps) : PARTICLE_COUNT(count), SUBSTEPS(substeps){
            particles.reserve(count);
        }

        void spawnIfPossible(const float elapsed_time, sf::Clock &spawner) {
            if (elapsed_time >= SPAWN_DELAY && particles.size() < PARTICLE_COUNT) {
                sf::Color color(rand() % 255, rand() % 255, rand() % 255);
                const float substep_dt = dt / static_cast<float>(SUBSTEPS);

                sf::Vector2f v(-100.f,0.f);
                if (particles.size() + 21 > PARTICLE_COUNT) {
                    int diff = PARTICLE_COUNT - particles.size();

                    for (int i = 0; i < diff; ++i) {
                        particles.emplace_back(startPos, 2.f, color);
                    }

                    for (int i = particles.size() - diff; i < particles.size(); ++i){
                        particles[i].prev_position = particles[i].position - startingVel * substep_dt;
                    }
                } else {
                    for (int i = 0; i < 21; ++i) {
                        particles.emplace_back(startPos + v, 2.f, color);
                        v.x += 10;
                    }

                    for (int i = particles.size() - 21; i < particles.size(); ++i) {
                        particles[i].prev_position = particles[i].position - startingVel * substep_dt;
                    }
                }

                updateStartingVel();
                spawner.restart();
            }
        }

        void update(InputState& inpState) {
            if (particles.empty()) return;

            const float substep_dt = dt / static_cast<float>(SUBSTEPS);
            const float dampening = 0.8f;
            const float padding = static_cast<float>(CELL_SIZE);

            updateGrid();

            for (int s = 0; s < SUBSTEPS; ++s) {

                for (auto& p : particles) {
                    p.accelerate(Particle::GRAVITY);
                }

                checkCollisions();

                for (auto& p : particles) {
                    p.applyBorderBounce((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, padding, dampening);
                }

                for (auto& p : particles) {
                    p.integrate(substep_dt);

                    sf::Vector2f disp = p.getDisplacement();
                    float disp2 = disp.x*disp.x + disp.y*disp.y;
                    if (disp2 > 2.f * padding) {
                        p.prev_position = p.position; 
                    }
                }

                updateGrid();
            }
        }

        void draw(sf::RenderWindow &window) {
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

        void draw(sf::RenderWindow &window) const {
            window.draw(particleCount);
            window.draw(timeBetweenFrames);
        }

        void setParticle(std::string cnt) {
            particleCount.setString(cnt);
        }

        void setFrames(std::string cnt) {
            timeBetweenFrames.setString(cnt + "ms");
        }
};

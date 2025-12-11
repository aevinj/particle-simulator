#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <string>

#include "Config.hpp"
#include "Particle.hpp"
#include "ParticleRenderer.hpp"

class World {
    private:
        const int PARTICLE_COUNT;
        const int SUBSTEPS;
        static const int CELL_SIZE = 6;
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
        std::vector<std::pair<int, int>> usedIndices;

        void resolveCollision(Particle &a, Particle &b) {
            sf::Vector2f v = a.position - b.position;
            float square_dist = v.x*v.x + v.y*v.y;
            float min_dist = a.radius + b.radius;

            if (square_dist == 0.f || square_dist >= min_dist * min_dist) {
                return;
            }

            float dist = std::sqrt(square_dist);
            sf::Vector2f normal = v / dist;
            float overlap = min_dist - dist;

            const float stiffness = 0.85f;
            float correction = overlap * 0.5f * stiffness;

            a.position += normal * correction;
            b.position -= normal * correction;
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

    public:
        std::vector<Particle> particles;

        World(const int count, const int substeps) : PARTICLE_COUNT(count), SUBSTEPS(substeps){
            particles.reserve(count);
            usedIndices.reserve(GRID_ROWS * GRID_COLS);
        }

        void spawnIfPossible(const float elapsed_time, sf::Clock &spawner) {
            if (elapsed_time >= SPAWN_DELAY && particles.size() < PARTICLE_COUNT) {
                sf::Color color(rand() % 255, rand() % 255, rand() % 255);

                sf::Vector2f v(-50.f,0.f);
                if (particles.size() + 11 > PARTICLE_COUNT) {
                    int diff = PARTICLE_COUNT - particles.size();

                    for (int i = 0; i < diff; ++i) {
                        particles.emplace_back(startPos, 3.f, color);
                    }

                    for (int i = 0; i < diff; ++i) {
                        particles[i].prev_position = particles[i].position - startingVel * dt;
                    }
                } else {
                    for (int i = 0; i < 11; ++i) {
                        particles.emplace_back(startPos + v, 3.f, color);
                        v.x += 10;
                    }

                    for (int i = particles.size() - 11; i < particles.size(); ++i) {
                        particles[i].prev_position = particles[i].position - startingVel * dt;
                    }
                }

                updateStartingVel();
                spawner.restart();
            }
        }

        void update(InputState &inpState) {
            float substep_dt = dt / static_cast<float>(SUBSTEPS);

            int cx, cy;

            if (inpState.mouseHeld) {
                cx = static_cast<int>(inpState.mousePos.x / CELL_SIZE);
                cy = static_cast<int>(inpState.mousePos.y / CELL_SIZE);
            }

            for (int s = 0; s < SUBSTEPS; ++s) {
                for (auto &particle : particles) {
                    if (inpState.mouseHeld) {
                        handleMouseHeld(particle, cx, cy, inpState.mousePos);
                    }

                    particle.applyGravity();
                    particle.integrate(substep_dt); 
                    particle.applyBounds(SCREEN_HEIGHT, SCREEN_WIDTH);
                }

                for (auto& [row, col] : usedIndices) {
                    grid[row][col].clear();
                }
                usedIndices.clear();
                
                for (int i = 0; i < particles.size(); ++i) {
                    const auto &p = particles[i];
                    int cx = static_cast<int>(p.position.x / CELL_SIZE);
                    int cy = static_cast<int>(p.position.y / CELL_SIZE);

                    if (cx < 0) cx = 0;
                    if (cy < 0) cy = 0;
                    if (cx >= GRID_COLS) cx = GRID_COLS - 1;
                    if (cy >= GRID_ROWS) cy = GRID_ROWS - 1;

                    grid[cx][cy].push_back(i); 
                    usedIndices.emplace_back(cx, cy);
                }

                for (int row = 0; row < GRID_ROWS; ++row) {
                    for (int col = 0; col < GRID_COLS; ++col) {
                        auto &cell = grid[row][col];
                        if (cell.empty()) {
                            continue;
                        }
                        
                        for (int a = 0; a < cell.size(); ++a) {
                            for (int b = a + 1; b < cell.size(); ++b) {
                                resolveCollision(particles[cell[a]], particles[cell[b]]);
                            }
                        }

                        for (auto& [dr, dc] : ds) {
                            int nr = dr + row;
                            int nc = dc + col;

                            if (nc < 0 || nc >= GRID_COLS || nr < 0 || nr >= GRID_ROWS) {
                                continue;
                            }

                            auto &neighbour = grid[nr][nc];

                            if (neighbour.empty()) {
                                continue;
                            }

                            for (int a : cell) {
                                for (int b : neighbour) {
                                    resolveCollision(particles[a], particles[b]);
                                }
                            }
                        }
                    }
                }
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

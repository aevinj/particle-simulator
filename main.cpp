#include <SFML/Graphics.hpp>
#include "Particle.hpp"
#include <vector>
#include <cmath>
#include <iostream>

const int SCREEN_HEIGHT = 600;
const int SCREEN_WIDTH = 800;
const float SPAWN_DELAY = 0.005f;
const int PARTICLE_COUNT = 1000;
const size_t CELL_SIZE = 16;
constexpr size_t GRID_COLS = (SCREEN_WIDTH + CELL_SIZE - 1) / CELL_SIZE;
constexpr size_t GRID_ROWS = (SCREEN_HEIGHT + CELL_SIZE - 1) / CELL_SIZE;
const int ITERATIONS = 4;
std::vector<int> grid[GRID_ROWS][GRID_COLS];
const std::pair<int, int> ds[4] = {
    {1,0},
    {0,1},
    {1,1},
    {-1,1}
};

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

    const float stiffness = 0.8f;
    float correction = overlap * 0.5f * stiffness;

    a.position += normal * correction;
    b.position -= normal * correction;
}

void updateStartingVel(sf::Vector2f &sv, bool &goingUp) {
    if (goingUp) {
        if (sv.x + 15.f < 500.f) {
            sv.x += 15.f;
        } else {
            sv.x = 500.f;
            goingUp = false;
        }
    } else {
        if (sv.x - 15.f > -500.f) {
            sv.x -= 15.f;
        } else {
            sv.x = -500.f;
            goingUp = true;
        }
    }
}

int main() {
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Particle Simulator");
    window.setFramerateLimit(60);

    sf::Clock clock, spawner;
    float dt = 1.f / 60.f;
    std::vector<Particle> particles;
    sf::Vector2f starting_vel(0.f, 500.f);
    bool goingUp = false;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
                window.close();
            }
        }
        // dt = clock.restart().asSeconds();

        if (particles.size() < PARTICLE_COUNT && spawner.getElapsedTime().asSeconds() >= SPAWN_DELAY) {
            particles.emplace_back(sf::Vector2f(static_cast<float>(SCREEN_WIDTH) / 2.f, 10.f), 5.f);
            auto& latest = particles.back();
            latest.prev_position = latest.position - starting_vel * dt;
            updateStartingVel(starting_vel, goingUp);
        }

        window.clear(sf::Color::White);

        for (int s = 0; s < 4; ++s) {
            for (auto& particle : particles) {
                particle.applyGravity();
                particle.integrate(static_cast<float>(dt / 4.f));
                particle.applyBounds(SCREEN_HEIGHT, SCREEN_WIDTH);
            }

            for (int row = 0; row < GRID_ROWS; ++row) {
                for (int col = 0; col < GRID_COLS; ++col) {
                    grid[row][col].clear();
                }
            }

            for (int i = 0; i < particles.size(); ++i) {
                const auto& p = particles[i];
                int cx = static_cast<int>(p.position.x / CELL_SIZE);
                int cy = static_cast<int>(p.position.y / CELL_SIZE);

                // clamp
                if (cx < 0) cx = 0;
                if (cy < 0) cy = 0;
                if (cx >= GRID_COLS) cx = GRID_COLS - 1;
                if (cy >= GRID_ROWS) cy = GRID_ROWS - 1;

                grid[cy][cx].push_back(i); // store index of this particle
            }

            for (int k = 0; k < ITERATIONS; ++k) {
                for (int row = 0; row < GRID_ROWS; ++row) {
                    for (int col = 0; col < GRID_COLS; ++col) {
                        auto& cell = grid[row][col];
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

                            auto& neighbour = grid[nr][nc];

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

        for (auto& particle : particles) {
            auto p = particle.getParticle();
            window.draw(p);
        }    

        window.display();
    }
}

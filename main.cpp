#include <SFML/Graphics.hpp>
#include "Particle.hpp"
#include <vector>
#include <cmath>
#include <iostream>
#include <string>

const int SCREEN_HEIGHT = 800;
const int SCREEN_WIDTH = 800;
const float SPAWN_DELAY = 0.0005f;
const int PARTICLE_COUNT = 5000;
const size_t CELL_SIZE = 8;
constexpr int GRID_COLS = (SCREEN_WIDTH + CELL_SIZE - 1) / CELL_SIZE;
constexpr int GRID_ROWS = (SCREEN_HEIGHT + CELL_SIZE - 1) / CELL_SIZE;
const int ITERATIONS = 4;
std::vector<int> grid[GRID_ROWS][GRID_COLS];
const std::pair<int, int> ds[4] = {
    {1,0},
    {0,1},
    {1,1},
    {1,-1} 
};
const float MOUSE_STRENGTH = 5000.f; 
const float mouseRadius = 200.f;

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

void updateStartingVel(sf::Vector2f &sv, bool &goingUp) {
    if (goingUp) {
        if (sv.x + 25.f < 500.f) {
            sv.x += 25.f;
        } else {
            sv.x = 500.f;
            goingUp = false;
        }
    } else {
        if (sv.x - 25.f > -500.f) {
            sv.x -= 25.f;
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
    bool mouseHeld = false;
    sf::Vector2f mousePos(0.f, 0.f);
    bool downPressed = false;
    bool leftPressed = false;
    bool rightPressed = false;
    bool upPressed = false;
    Particle::GRAVITY = {0.f, 2000.f};
    sf::Font font;
    font.loadFromFile("../assets/arial.ttf");
    sf::Text particleCountText, timeBetweenFramesText;

    particleCountText.setFont(font);
    particleCountText.setString("--");
    particleCountText.setCharacterSize(24);
    particleCountText.setFillColor(sf::Color::Black);

    timeBetweenFramesText.setFont(font);
    timeBetweenFramesText.setString("--");
    timeBetweenFramesText.setCharacterSize(24);
    timeBetweenFramesText.setFillColor(sf::Color::Black);
    timeBetweenFramesText.setPosition(0.f, 30.f);

    particles.reserve(PARTICLE_COUNT);

    while (window.isOpen()) {
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
                window.close();
            }
        }

        if (particles.size() < PARTICLE_COUNT && spawner.getElapsedTime().asSeconds() >= SPAWN_DELAY) {
            particles.emplace_back(sf::Vector2f(static_cast<float>(SCREEN_WIDTH) / 2.f, 10.f), 3.f);
            auto& latest = particles.back();
            latest.prev_position = latest.position - starting_vel * dt;
            updateStartingVel(starting_vel, goingUp);
            spawner.restart();
        }

        particleCountText.setString(std::to_string(particles.size()));
        timeBetweenFramesText.setString(std::to_string(clock.restart().asMilliseconds()) + "ms");

        window.clear(sf::Color::White);

        mouseHeld = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
        downPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Down); 
        leftPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Left); 
        rightPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Right); 
        upPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Up); 

        if (leftPressed) {
            Particle::GRAVITY = {-2000.f, 0.f};
        } else if (downPressed) {
            Particle::GRAVITY = {0.f, 2000.f};
        } else if (rightPressed) {
            Particle::GRAVITY =  {2000.f, 0.f};
        } else if (upPressed) {
            Particle::GRAVITY = {0.f, -2000.f};
        }

        for (int s = 0; s < 4; ++s) {
            for (auto& particle : particles) {

                if (mouseHeld) {
                    sf::Vector2f dir = mousePos - particle.position;
                    float distSq = dir.x * dir.x + dir.y * dir.y;

                    if (distSq < mouseRadius * mouseRadius) {
                        float dist = std::sqrt(distSq);
                        if (dist > 1.f) { 
                            sf::Vector2f normalized = dir / dist;

                            particle.acceleration += normalized * MOUSE_STRENGTH;
                        }
                    }
                }
                
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
            window.draw(particle.getParticle());
        }    

        window.draw(particleCountText);
        window.draw(timeBetweenFramesText);
        window.display();
    }
}

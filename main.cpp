#include <SFML/Graphics.hpp>
#include "particle.hpp"
#include <vector>
#include <cmath>

const int SCREEN_HEIGHT = 600;
const int SCREEN_WIDTH = 800;
const float SPAWN_DELAY = 0.005f;
const int PARTICLE_COUNT = 200;

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

int main() {
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Particle Simulator");
    window.setFramerateLimit(60);

    sf::Clock clock, spawner;
    float dt;
    std::vector<Particle> particles;
    const sf::Vector2f starting_vel(1000.f, 1000.f);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
                window.close();
            }
        }
        dt = clock.restart().asSeconds();

        if (particles.size() < PARTICLE_COUNT && spawner.getElapsedTime().asSeconds() >= SPAWN_DELAY) {
            particles.emplace_back(sf::Vector2f(100.f,100.f), 5.f);
            auto& latest = particles.back();
            latest.prev_position = latest.position - starting_vel * dt;
        }

        window.clear(sf::Color::White);

        for (auto& particle : particles) {
            particle.applyGravity();
            particle.integrate(dt);
            particle.applyBounds(SCREEN_HEIGHT, SCREEN_WIDTH);
        }

        const int iterations = 4;
        for (int k = 0; k < iterations; ++k) {
            for (int i = 0; i < particles.size(); ++i) {
                for (int j = i + 1; j < particles.size(); ++j) {
                    resolveCollision(particles[i], particles[j]);
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

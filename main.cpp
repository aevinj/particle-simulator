#include <SFML/Graphics.hpp>
#include "particle.hpp"
#include <vector>

const int SCREEN_HEIGHT = 600;
const int SCREEN_WIDTH = 800;

int main() {
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Particle Simulator");
    window.setFramerateLimit(60);

    sf::Clock clock;
    float dt;
    std::vector<Particle> particles;
    particles.emplace_back(sf::Vector2f(300.f,300.f), 9.f);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
                window.close();
            }
        }
        dt = clock.restart().asSeconds();

        window.clear(sf::Color::White);

        for (auto& particle : particles) {
            particle.applyGravity();
            particle.integrate(dt);
            particle.applyBounds(SCREEN_HEIGHT, SCREEN_WIDTH);
            auto p = particle.getParticle();
            window.draw(p);
        }
        
        window.display();
    }
}

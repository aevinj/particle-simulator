#include <SFML/Graphics.hpp>
#include <string>

#include "Config.hpp"
#include "World.hpp"
#include "Particle.hpp"


int main() {
    srand(1);
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Particle Sim");
    window.setFramerateLimit(60);

    sf::Clock clock, spawner;

    VisualText visualText;
    InputState inpState;

    // Particle count, substeps, iterations
    World world(5000, 6, 1);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
                window.close();
            }
        }

        world.spawnIfPossible(spawner.restart().asSeconds(), spawner);
        visualText.setParticle(std::to_string(world.particles.size()));
        visualText.setFrames(std::to_string(clock.restart().asMilliseconds()));

        window.clear(sf::Color::White);

        inpState.update(window);

        world.update(inpState);

        world.draw(window);

        visualText.draw(window);
        window.display();
    }
}

#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>

#include "Config.hpp"
#include "World.hpp"

int main() {
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Particle Simulator");
    window.setFramerateLimit(60);

    World world;

    sf::Clock frameClock;

    sf::Font font;
    if (!font.loadFromFile("../assets/arial.ttf")) {
        std::cerr << "Error loading font - ensure it is available." << std::endl;
        throw 1;
    }

    sf::Text particleCountText;
    particleCountText.setFont(font);
    particleCountText.setCharacterSize(24);
    particleCountText.setFillColor(sf::Color::Black);

    sf::Text timeBetweenFramesText;
    timeBetweenFramesText.setFont(font);
    timeBetweenFramesText.setCharacterSize(24);
    timeBetweenFramesText.setFillColor(sf::Color::Black);
    timeBetweenFramesText.setPosition(0.f, 30.f);

    const float fixedDt = 1.f / 60.f;
    InputState input;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
                window.close();
            }
        }

        input.mouseHeld    = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        input.mousePos     = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
        input.downPressed  = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        input.leftPressed  = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        input.rightPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
        input.upPressed    = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);

        world.spawnIfNeeded(fixedDt);
        world.update(fixedDt, input);

        particleCountText.setString(std::to_string(world.particleCount()));
        timeBetweenFramesText.setString(std::to_string(frameClock.restart().asMilliseconds()) + "ms");

        window.clear(sf::Color::White);

        for (auto& particle : world.particles()) {
            window.draw(particle.getParticle());
        }

        window.draw(particleCountText);
        window.draw(timeBetweenFramesText);
        window.display();
    }

    return 0;
}


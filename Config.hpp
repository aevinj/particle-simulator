#pragma once

#include "Particle.hpp"

inline const int SCREEN_WIDTH = 800;
inline const int SCREEN_HEIGHT = 800;

struct InputState {
    bool mouseHeld = false;
    sf::Vector2f mousePos;

    bool downPressed = false;    
    bool upPressed = false;
    bool leftPressed = false;
    bool rightPressed = false;

    void update(sf::RenderWindow &window) {
        mouseHeld = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
        downPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        leftPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        rightPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
        upPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
        updateGravityIfNeeded();
    }

    void updateGravityIfNeeded() {
        if (leftPressed) {
            Particle::GRAVITY = {-2000.f, 0.f};
        } else if (downPressed) {
            Particle::GRAVITY = {0.f, 2000.f};
        } else if (rightPressed) {
            Particle::GRAVITY =  {2000.f, 0.f};
        } else if (upPressed) {
            Particle::GRAVITY = {0.f, -2000.f};
        }
    }
};

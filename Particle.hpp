#pragma once
#include <SFML/Graphics.hpp>

struct Particle {
    sf::Vector2f position;
    sf::Vector2f prev_position;
    sf::Vector2f acceleration;
    float radius;
    sf::Color color;

    inline static sf::Vector2f GRAVITY = {0.f, 0.f}; 

    Particle(sf::Vector2f start_pos, float r, sf::Color c)
        : position(start_pos)
        , prev_position(start_pos)
        , acceleration(0.f, 0.f)
        , radius(r)
        , color(c)
    {}

    void accelerate(sf::Vector2f a) { acceleration += a; }

    void integrate(float dt) {
        sf::Vector2f displacement = position - prev_position;
        prev_position = position;
        position = position + displacement + acceleration * (dt * dt);
        acceleration = {0.f, 0.f};
    }

    sf::Vector2f getDisplacement() const {
        return position - prev_position;
    }

    void setVelocity(sf::Vector2f v, float dt) {
        prev_position = position - v * dt;
    }

    void applyBorderBounce(float width, float height, float padding, float dampening) {
        const sf::Vector2f pos = position;
        const sf::Vector2f disp = getDisplacement();

        const sf::Vector2f dx = {-disp.x,  disp.y};
        const sf::Vector2f dy = { disp.x, -disp.y};

        // Left/right
        if (pos.x < padding || pos.x > width - padding) {
            position.x = (pos.x < padding) ? padding : (width - padding);
            prev_position = position - (dx * dampening);
        }

        // Top/bottom
        if (pos.y < padding || pos.y > height - padding) {
            position.y = (pos.y < padding) ? padding : (height - padding);
            prev_position = position - (dy * dampening);
        }
    }
};


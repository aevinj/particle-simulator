#pragma once

#include <SFML/Graphics.hpp>

inline constexpr float DAMPING     = 0.98f;
inline constexpr float RESTITUTION = 0.8f;   // how bouncy surfaces are
inline constexpr float FRICTION    = 0.99f;

struct Particle {
    sf::Vector2f position;
    sf::Vector2f prev_position;
    sf::Vector2f acceleration;
    float radius;
    sf::Color color;
    inline static sf::Vector2f GRAVITY = {0.f, 2000.f}; 

    Particle(sf::Vector2f start_pos, float r, sf::Color color)
        : position(start_pos)
        , prev_position(start_pos)
        , acceleration(0.f, 0.f)
        , radius(r)
        , color(color)
    {}

    void integrate(float dt) {
        // Verlet: p_new = p + (p - p_prev) * damping + a * dt^2
        sf::Vector2f displacement = position - prev_position;
        sf::Vector2f new_position = position + displacement * DAMPING + acceleration * (dt * dt);

        prev_position = position;
        position      = new_position;

        acceleration = {0.f, 0.f};
    }

    void applyGravity() {
        acceleration += GRAVITY;
    }

    void applyBounds(int height, int width) {
        sf::Vector2f velocity = position - prev_position;

        // floor
        if (position.y + radius > height) {
            position.y = height - radius;

            if (velocity.y > 0.f) {
                velocity.y = -velocity.y * RESTITUTION;
                velocity.x *= FRICTION;
            }

            prev_position = position - velocity;
        }

        // ceil
        if (position.y - radius < 0.f) {
            position.y = radius;

            if (velocity.y < 0.f) {
                velocity.y = -velocity.y * RESTITUTION;
                velocity.x *= FRICTION;
            }

            prev_position = position - velocity;
        }

        // left wall
        if (position.x - radius < 0.f) {
            position.x = radius;

            if (velocity.x < 0.f) {
                velocity.x = -velocity.x * RESTITUTION;
                velocity.y *= FRICTION;
            }

            prev_position = position - velocity;
        }

        // right wall
        if (position.x + radius > width) {
            position.x = width - radius;

            if (velocity.x > 0.f) {
                velocity.x = -velocity.x * RESTITUTION;
                velocity.y *= FRICTION;
            }

            prev_position = position - velocity;
        }
    }
};


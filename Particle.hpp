#pragma once

#include <SFML/Graphics.hpp>

const float DAMPING = 0.98f;
const float RESTITUTION = 0.8f;     // how bouncy surfaces are
const float FRICTION = 0.99f; 

struct Particle {
    sf::Vector2f position;
    sf::Vector2f prev_position;
    sf::Vector2f acceleration;
    float radius;
    sf::Color color;
    sf::CircleShape p;
    inline static sf::Vector2f GRAVITY;

    Particle(sf::Vector2f start_pos, float r) : 
        position(start_pos),
        prev_position(start_pos),
        acceleration(0.f, 0.f),
        radius(r),
        color(sf::Color::Green),
        p(radius) {
            p.setOrigin(radius, radius);
            p.setFillColor(color);
        }

    void integrate(float dt) {
        //  p_new = p + (p - p_prev) * damping + a * dt²
        sf::Vector2f displacement = position - prev_position;
        auto new_position = position + displacement * DAMPING + acceleration * (dt*dt);

        prev_position = position;
        position = new_position;

        acceleration = {0.f, 0.f};
    }

    void applyGravity() {
        acceleration += GRAVITY;
    }

    sf::CircleShape& getParticle() {
        p.setPosition(position);
        return p;
    }

    void applyBounds(const int height, const int width) {
        auto velocity = position - prev_position;
        
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

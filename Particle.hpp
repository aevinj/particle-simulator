#pragma once

#include <SFML/Graphics.hpp>
#include <cstdlib>
#include <cmath>

const int SCREEN_HEIGHT = 600;
const int SCREEN_WIDTH = 800;
const float GRAVITY = 800.f;

const float DAMPENING = 0.8f;
const float DRAG = 0.97f;
const float FRICTION = 0.88f;
const float MIN_VEL_THRESHOLD = 5.f;
const float FLOOR_BOUNCE_CUTOFF = 60.f;

struct Particle {
    float radius = 7.f;
    sf::Vector2f velocity;
    sf::CircleShape particle;

    Particle(sf::Vector2f velocity, sf::Vector2f position) : velocity(velocity) , particle(radius) {
        particle.setFillColor(sf::Color::Green);
        particle.setOrigin(radius, radius);

        // int spawnWidth  = SCREEN_WIDTH  - 2 * static_cast<int>(radius);
        // int spawnHeight = SCREEN_HEIGHT / 2 - 2 * static_cast<int>(radius);

        // float x = static_cast<float>((rand() % spawnWidth)  + static_cast<int>(radius));
        // float y = static_cast<float>((rand() % spawnHeight) + static_cast<int>(radius));

        particle.setPosition(position);
    }

    void update(float dt) {
        // add gravity
        applyGravity(dt);

        // read current pos
        sf::Vector2f curr_pos = particle.getPosition();

        // move pos using v * dt
        curr_pos.x += velocity.x * dt;
        curr_pos.y += velocity.y * dt;

        // call dedicated func for clamping, bouncing/stopping , floor friction
        handleBounds(curr_pos);

        // apply drag
        applyDrag();

        // clamp tiny velocities
        if (std::fabs(velocity.x) <= MIN_VEL_THRESHOLD) {
            velocity.x = 0.f;
        }
        if (std::fabs(velocity.y) <= MIN_VEL_THRESHOLD) {
            velocity.y = 0.f;
        }

        // write final pos back to shape
        particle.setPosition(curr_pos);
    }

    void handleBounds(sf::Vector2f &pos) {
        // floor
        if (pos.y + radius > SCREEN_HEIGHT) {
            pos.y = SCREEN_HEIGHT - radius;

            if (velocity.y > 0.f) {
                if (std::fabs(velocity.y) < FLOOR_BOUNCE_CUTOFF) {
                    velocity.y = 0.f;
                } else {
                    velocity.y = -velocity.y * DAMPENING;
                }
            }

            velocity.x *= FRICTION;
        }

        // ceiling
        if (pos.y - radius < 0.f) {
            pos.y = radius;

            if (velocity.y < 0.f) {
                velocity.y = -velocity.y * DAMPENING;
            }
        }

        // right wall
        if (pos.x + radius > SCREEN_WIDTH) {
            pos.x = SCREEN_WIDTH - radius;
            if (velocity.x > 0.f) {
                velocity.x = -velocity.x * DAMPENING;
            }
        }

        // left wall
        if (pos.x - radius < 0.f) {
            pos.x = radius;
            if (velocity.x < 0.f) {
                velocity.x = -velocity.x * DAMPENING;
            }
        }
    }

    void applyGravity(float dt) {
        velocity.y += GRAVITY * dt;
    }

    void applyDrag() {
        velocity.x *= DRAG;
        velocity.y *= DRAG;
    }

    sf::CircleShape& getParticle() {
        return particle;
        }
};

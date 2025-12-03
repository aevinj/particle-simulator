#include <SFML/Graphics.hpp>

const int SCREEN_HEIGHT = 600;
const int SCREEN_WIDTH = 800;
const float GRAVITY = 800.f;
const float DAMPENING = 0.8f;

struct Particle {
        std::pair<float, float> velocity;
        sf::CircleShape particle;

        Particle(std::pair<float, float> velocity) : velocity(velocity) , particle(5.f) {
                particle.setFillColor(sf::Color::Green);
                particle.setOrigin(particle.getRadius(), particle.getRadius());
                particle.setPosition(static_cast<float>(SCREEN_WIDTH / 2), static_cast<float>(SCREEN_HEIGHT / 2));
        }

        void updatePos(float dt) {
                auto [x,y] = particle.getPosition();
        updateVelocities(dt);
                x += velocity.first * dt;
                y += velocity.second * dt;

        if (y + particle.getRadius() >= SCREEN_HEIGHT) {
            y = SCREEN_HEIGHT - particle.getRadius();
            velocity.second = -velocity.second * DAMPENING;
        }

                particle.setPosition(x, y);
        }

    void updateVelocities(float dt) {
        velocity.second += GRAVITY * dt;
    }

        sf::CircleShape& getParticle() {return particle;}
};

int main() {
        sf::RenderWindow window(sf::VideoMode(800,600), "Particle test");
        window.setFramerateLimit(120);

    sf::Clock clock;

        Particle particle({0.f,0.f});

        float dt;
        while (window.isOpen()) {
                sf::Event event;
                while (window.pollEvent(event)) {
                        if (event.type == sf::Event::Closed) window.close();
                }
        dt = clock.restart().asSeconds();
                window.clear(sf::Color::Black);
                window.draw(particle.getParticle());
                window.display();

                particle.updatePos(dt);
        }
        return 0;
}

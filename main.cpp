#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdlib>  // for rand
#include <ctime>    // for rand

const int SCREEN_HEIGHT = 600;
const int SCREEN_WIDTH = 800;
const float GRAVITY = 800.f;
const float DAMPENING = 0.8f;

struct Particle {
        float radius = 5.f;
        sf::Vector2f velocity;
        sf::CircleShape particle;

        Particle(sf::Vector2f velocity) : velocity(velocity) , particle(radius) {
                particle.setFillColor(sf::Color::Green);
                particle.setOrigin(radius, radius);
                float x = static_cast<float>((rand() % SCREEN_WIDTH - radius) + radius);
                float y = static_cast<float>((rand() % (SCREEN_HEIGHT / 2) - radius) + radius);
                particle.setPosition(x, y);
        }

        void updatePos(float dt) {
            auto [x,y] = particle.getPosition();
            updateVelocities(dt);
            x += velocity.x * dt;
            y += velocity.y * dt;

            if (y + radius >= SCREEN_HEIGHT) {
                y = SCREEN_HEIGHT - radius; 
                velocity.y = -velocity.y * DAMPENING;
            }

            particle.setPosition(x, y);
        }

        void updateVelocities(float dt) {
            velocity.y += GRAVITY * dt;
        }

        sf::CircleShape& getParticle() {return particle;}
};

int main() {
        srand(time(NULL));  // for rand
        sf::RenderWindow window(sf::VideoMode(800,600), "Particle test");
        window.setFramerateLimit(120);

        sf::Clock clock;

        std::vector<Particle> particles;
        particles.reserve(3);
        for (int i = 0; i < 3; ++i) {
            particles.emplace_back(sf::Vector2f(0.f,0.f));
        }
        
        float dt;
        while (window.isOpen()) {
                sf::Event event;
                while (window.pollEvent(event)) {
                        if (event.type == sf::Event::Closed) window.close();
                }
                dt = clock.restart().asSeconds();

                window.clear(sf::Color::Black);

                for (auto& particle : particles) {
                    particle.updatePos(dt);
                    window.draw(particle.getParticle());
                }
               
                window.display();
        }
        return 0;
}

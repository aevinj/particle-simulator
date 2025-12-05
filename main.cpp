#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdlib>  // for rand
#include <ctime>    // for rand
#include <cmath>

const int SCREEN_HEIGHT = 600;
const int SCREEN_WIDTH = 800;
const float GRAVITY = 800.f;
const float DAMPENING = 0.8f;
const float DRAG = 0.97f;
const float FRICTION = 0.88f;

struct Particle {
        float radius = 7.f;
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

            applyGravity(dt);

            x += velocity.x * dt;
            y += velocity.y * dt;

            bounce(x, y);

            applyDrag();

            if (y + radius >= SCREEN_HEIGHT) {
                velocity.x *= FRICTION;
            }

            particle.setPosition(x, y);
        }

        void bounce(float& x, float& y) {
            if (y + radius >= SCREEN_HEIGHT) {
                y = SCREEN_HEIGHT - radius; 
                velocity.y = -velocity.y * DAMPENING;
            } else if (y - radius <= 0) {
                y = 0 + radius;
                velocity.y = -velocity.y;
            }
            
            if (x + radius >= SCREEN_WIDTH) {
                x = SCREEN_WIDTH - radius;
                velocity.x = -velocity.x;
            } else if (x - radius <= 0) {
                x = 0 + radius;
                velocity.x = -velocity.x;
            }
        }

        void applyGravity(float dt) {
            velocity.y += GRAVITY * dt;
        }

        void applyDrag() {
            velocity.x *= DRAG;
            velocity.y *= DRAG;
        }

        sf::CircleShape& getParticle() {return particle;}
};

float dot(const sf::Vector2f& a, const sf::Vector2f& b) {
    return a.x*b.x + a.y*b.y;
}

bool checkCollision(Particle &a, Particle &b) {
    float dx = b.getParticle().getPosition().x - a.getParticle().getPosition().x;
    float dy = b.getParticle().getPosition().y - a.getParticle().getPosition().y;
    
    return dx*dx + dy*dy < (a.radius*b.radius*4);
}

void handleCollision(Particle &a, Particle &b) {
    sf::Vector2f posA = a.getParticle().getPosition();
    sf::Vector2f posB = b.getParticle().getPosition();

    float dx = posB.x - posA.x;
    float dy = posB.y - posA.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    if (dist == 0) return; // identical positions -> avoid NaN

    sf::Vector2f normal(dx / dist, dy / dist);
    float overlap = (a.radius + b.radius) - dist;

    a.particle.setPosition(posA - normal * (overlap * 0.5f));
    b.particle.setPosition(posB + normal * (overlap * 0.5f));
}

void swapVelocities(Particle &a, Particle &b) {
    sf::Vector2f temp = a.velocity;
    a.velocity = b.velocity;
    b.velocity = temp;
}

int main() {
        srand(time(NULL));  // for rand
        sf::RenderWindow window(sf::VideoMode(800,600), "Particle test");
        window.setFramerateLimit(120);

        sf::Clock clock;

        std::vector<Particle> particles;
        particles.reserve(300);
        for (int i = 0; i < 300; ++i) {
            particles.emplace_back(sf::Vector2f(100.f,0.f));
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

                for (int i = 0; i < particles.size(); ++i) {
                    for (int j = i + 1; j < particles.size(); ++j) {
                        if (checkCollision(particles[i], particles[j])) {
                            swapVelocities(particles[i], particles[j]);
                            handleCollision(particles[i], particles[j]);
                        }
                    }
                }
                
                // positional relaxations only
                for (int k = 0; k < 10; ++k) {
                    for (int i = 0; i < particles.size(); ++i) {
                        for (int j = i + 1; j < particles.size(); ++j) {
                            if (checkCollision(particles[i], particles[j])) {
                                handleCollision(particles[i], particles[j]);
                            }
                        }
                    }
                }   

                window.display();
        }
        return 0;
}

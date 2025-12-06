#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdlib>  // for rand
#include <ctime>    // for rand
#include <cmath>
#include "Particle.hpp"

float dot(const sf::Vector2f& a, const sf::Vector2f& b) {
    return a.x*b.x + a.y*b.y;
}

bool checkCollision(Particle &a, Particle &b) {
    float dx = b.getParticle().getPosition().x - a.getParticle().getPosition().x;
    float dy = b.getParticle().getPosition().y - a.getParticle().getPosition().y;
    float min_dist = a.radius + b.radius;
    
    return dx*dx + dy*dy < min_dist*min_dist;
}

void handleCollision(Particle &a, Particle &b) {
    sf::Vector2f posA = a.getParticle().getPosition();
    sf::Vector2f posB = b.getParticle().getPosition();

    float dx = posB.x - posA.x;
    float dy = posB.y - posA.y;
    float squared_dist = dx*dx + dy*dy;

    if (squared_dist == 0.f) return;

    float dist = std::sqrt(squared_dist);
    float r = a.radius + b.radius;

    if (dist >= r) return;

    sf::Vector2f normal(dx / dist, dy / dist);

    float overlap = r - dist;
    const float percent = 0.8f;
    sf::Vector2f correction = normal * (overlap * 0.5f * percent);

    posA -= correction;
    posB += correction;

    a.getParticle().setPosition(posA);
    b.getParticle().setPosition(posB);

    sf::Vector2f va = a.velocity;
    sf::Vector2f vb = b.velocity;
    float vaN = dot(va, normal); // normal component of a 
    float vbN = dot(vb, normal); // normal component of b 

    // If they are separating already, don't bounce 
    if (vaN - vbN < 0.f) return; 

    sf::Vector2f vaT = va - normal * vaN; 
    sf::Vector2f vbT = vb - normal * vbN;

    std::swap(vaN, vbN);

    a.velocity = vaT + normal * vaN;
    b.velocity = vbT + normal * vbN;
}

int main() {
        srand(time(NULL));  // for rand
        sf::RenderWindow window(sf::VideoMode(800,600), "Particle test");
        window.setFramerateLimit(120);

        sf::Clock clock;

        std::vector<Particle> particles;
        particles.reserve(500);
        for (int i = 0; i < 500; ++i) {
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
                    particle.update(dt);
                }

                for (int i = 0; i < particles.size(); ++i) {
                    for (int j = i + 1; j < particles.size(); ++j) {
                        if (checkCollision(particles[i], particles[j])) {
                            handleCollision(particles[i], particles[j]);
                        }
                    }
                }

                for (auto& particle : particles) {
                    window.draw(particle.getParticle());
                }
                
                window.display();
        }
        return 0;
}

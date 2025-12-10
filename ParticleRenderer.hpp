#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include "Particle.hpp"

class ParticleRenderer {
    private:
        sf::Texture texture;

    public:
        sf::VertexArray vertices;

        ParticleRenderer(const int PARTICLE_COUNT) : vertices(sf::Quads, PARTICLE_COUNT * 4) {
            texture.loadFromFile("../assets/circle.png");
            texture.setSmooth(true);
        }

        void build(const std::vector<Particle> &particles) {
            const float textureSize = static_cast<float>(texture.getSize().x);

            for (int i = 0; i < particles.size(); ++i) {
                const auto &p = particles[i];
                const float r = p.radius;
                const std::size_t base = i*4;

                // positions: a quad around particle centre
                vertices[base + 0].position = p.position + sf::Vector2f(-r, -r);
                vertices[base + 1].position = p.position + sf::Vector2f( r, -r);
                vertices[base + 2].position = p.position + sf::Vector2f( r,  r);
                vertices[base + 3].position = p.position + sf::Vector2f(-r,  r);

                // full texture
                vertices[base + 0].texCoords = {0.f,       0.f};
                vertices[base + 1].texCoords = {textureSize,   0.f};
                vertices[base + 2].texCoords = {textureSize, textureSize};
                vertices[base + 3].texCoords = {0.f,       textureSize};

                // color per vertex
                for (int k = 0; k < 4; ++k) {
                    vertices[base + k].color = p.color;
                }
            }
        }

        void draw(sf::RenderTarget &target) {
            sf::RenderStates states;
            states.texture = &texture;
            target.draw(vertices, states);
        }
};

#pragma once

#include <SFML/Graphics.hpp>

class VisualText {
private:
    sf::Font font;
    sf::Text particleCount, timeBetweenFrames;

public:
    VisualText() {
        font.loadFromFile("../assets/arial.ttf");

        particleCount.setFont(font);
        particleCount.setString("--");
        particleCount.setCharacterSize(24);
        particleCount.setFillColor(sf::Color::Black);

        timeBetweenFrames.setFont(font);
        timeBetweenFrames.setString("--");
        timeBetweenFrames.setCharacterSize(24);
        timeBetweenFrames.setFillColor(sf::Color::Black);
        timeBetweenFrames.setPosition(0.f, 30.f);
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(particleCount);
        window.draw(timeBetweenFrames);
    }

    void setParticle(std::string cnt) { particleCount.setString(cnt); }
    void setFrames(std::string cnt)   { timeBetweenFrames.setString(cnt + "ms"); }
};

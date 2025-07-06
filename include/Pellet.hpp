#pragma once
#include <SFML/Graphics.hpp>

class Pellet : public sf::Drawable, public sf::Transformable {
public:
    Pellet(const sf::Vector2f& pos, float radius = 5.f);
    bool eaten(const sf::Vector2f& playerPos) const;

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    sf::CircleShape m_shape;
};

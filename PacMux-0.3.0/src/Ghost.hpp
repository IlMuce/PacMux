#pragma once
#include <SFML/Graphics.hpp>

class Ghost : public sf::Drawable, public sf::Transformable {
public:
    Ghost(const sf::Vector2f& pos, sf::Color color = sf::Color::Red, float radius = 12.f);
    sf::Vector2f getPosition() const { return m_shape.getPosition(); }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    sf::CircleShape m_shape;
};

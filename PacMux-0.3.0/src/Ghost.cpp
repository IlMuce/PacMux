#include "Ghost.hpp"

Ghost::Ghost(const sf::Vector2f& pos, sf::Color color, float radius)
    : m_shape(radius)
{
    m_shape.setFillColor(color);
    m_shape.setOrigin({radius, radius}); // Corretto per SFML 3.0
    m_shape.setPosition(pos);
}

void Ghost::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    target.draw(m_shape, states);
}

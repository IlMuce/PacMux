#include "Pellet.hpp"

Pellet::Pellet(const sf::Vector2f& pos, float radius)
: m_shape(radius)
{
    m_shape.setFillColor(sf::Color::White);
    m_shape.setOrigin(sf::Vector2f(radius, radius));
    m_shape.setPosition(pos);
}

bool Pellet::eaten(const sf::Vector2f& playerPos) const {
    return m_shape.getGlobalBounds().contains(playerPos);
}

void Pellet::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    target.draw(m_shape, states);
}

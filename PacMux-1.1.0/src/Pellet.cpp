#include "Pellet.hpp"

// Costruttore: inizializza la posizione e l'aspetto del pellet
Pellet::Pellet(const sf::Vector2f& pos, float radius)
    : m_shape(radius)
{
    m_shape.setFillColor(sf::Color(255, 209, 128)); // Peach
    m_shape.setOrigin(sf::Vector2f(radius, radius));
    m_shape.setPosition(pos);
}

// Ritorna true se il player ha "mangiato" il pellet (collisione)
bool Pellet::eaten(const sf::Vector2f& playerPos) const {
    return m_shape.getGlobalBounds().contains(playerPos);
}

// Disegna il pellet sulla finestra
void Pellet::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    target.draw(m_shape, states);
}

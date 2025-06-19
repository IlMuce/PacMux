#include "Player.hpp"
#include <algorithm> // std::clamp

Player::Player(float speed, const sf::Vector2f& startPos, const sf::Vector2u& tileSize)
: m_shape(tileSize.x * 0.4f)
, m_speed(speed)
{
    m_shape.setFillColor(sf::Color::Yellow);
    m_shape.setOrigin(sf::Vector2f(m_shape.getRadius(), m_shape.getRadius()));
    m_shape.setPosition(startPos);
}

void Player::update(float dt, const TileMap& map, const sf::Vector2u& tileSize) {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  m_direction = {-1.f,  0.f};
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) m_direction = { 1.f,  0.f};
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))    m_direction = { 0.f, -1.f};
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  m_direction = { 0.f,  1.f};

    sf::Vector2f newPos = m_shape.getPosition() + m_direction * m_speed * dt;

    // collisione con muri
    unsigned tx = static_cast<unsigned>(newPos.x / tileSize.x);
    unsigned ty = static_cast<unsigned>(newPos.y / tileSize.y);
    if (!map.isWall(tx, ty)) {
        // clamp dentro i bordi
        newPos.x = std::clamp(newPos.x, 0.f, map.getSize().x * float(tileSize.x));
        newPos.y = std::clamp(newPos.y, 0.f, map.getSize().y * float(tileSize.y));
        m_shape.setPosition(newPos);
    }
}

void Player::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    target.draw(m_shape, states);
}

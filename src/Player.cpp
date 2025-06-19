// src/Player.cpp
#include "Player.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>

Player::Player(float speed, const sf::Vector2f& startPos, const sf::Vector2u& tileSize)
: m_shape(tileSize.x * 0.4f)
, m_speed(speed)
, m_direction{0.f, 0.f}
, m_nextDirection{0.f, 0.f}
, m_tileSize(tileSize)
{
    m_shape.setFillColor(sf::Color::Yellow);
    m_shape.setOrigin(sf::Vector2f(m_shape.getRadius(), m_shape.getRadius()));
    m_shape.setPosition(startPos);
}

void Player::update(float dt, const TileMap& map, const sf::Vector2u& tileSize) {
    using Key = sf::Keyboard::Key;

    // 1) Leggi input
    if      (sf::Keyboard::isKeyPressed(Key::Left))  m_nextDirection = {-1.f,  0.f};
    else if (sf::Keyboard::isKeyPressed(Key::Right)) m_nextDirection = { 1.f,  0.f};
    else if (sf::Keyboard::isKeyPressed(Key::Up))    m_nextDirection = { 0.f, -1.f};
    else if (sf::Keyboard::isKeyPressed(Key::Down))  m_nextDirection = { 0.f,  1.f};

    sf::Vector2f pos = m_shape.getPosition();
    float r = m_shape.getRadius();

    // 2) Cella corrente e centro
    unsigned cellX = unsigned(pos.x / tileSize.x);
    unsigned cellY = unsigned(pos.y / tileSize.y);
    sf::Vector2f center{
        cellX * float(tileSize.x) + tileSize.x/2.f,
        cellY * float(tileSize.y) + tileSize.y/2.f
    };

    // tolleranza per allineamento (1/4 di cella)
    float tolX = tileSize.x / 4.f;
    float tolY = tileSize.y / 4.f;

    // 3) Se retromarcia, accetta subito
    if (m_nextDirection == -m_direction && m_direction != sf::Vector2f{0,0}) {
        m_direction = m_nextDirection;
    }
    // 4) Altrimenti, prova a svoltare quando:
    //    - sei al centro (lato corrente) oppure
    //    - sei allineato entro tolleranza sull’asse perpendicolare
    else if (m_nextDirection != m_direction) {
        bool canTurn = false;
        // se stai andando orizzontale e vuoi svoltare verticale:
        if (m_direction.x != 0.f && m_nextDirection.y != 0.f
            && std::abs(pos.x - center.x) < tolX)
            canTurn = true;
        // se stai andando verticale e vuoi svoltare orizzontale:
        if (m_direction.y != 0.f && m_nextDirection.x != 0.f
            && std::abs(pos.y - center.y) < tolY)
            canTurn = true;
        // se sei fermo, considera sempre la svolta al centro
        if (m_direction == sf::Vector2f{0,0})
            canTurn = true;
        // oppure se sei esattamente al centro
        if (std::abs(pos.x - center.x) < 1.f && std::abs(pos.y - center.y) < 1.f)
            canTurn = true;

        if (canTurn) {
            int nx = int(cellX) + int(m_nextDirection.x);
            int ny = int(cellY) + int(m_nextDirection.y);
            bool inBounds = nx>=0 && ny>=0
                         && nx<int(map.getSize().x) && ny<int(map.getSize().y);
            if (inBounds && !map.isWall(nx,ny)) {
                m_direction = m_nextDirection;
                // riallinea al centro per evitare piccole imprecisioni
                m_shape.setPosition(center);
                pos = center;
            }
        }
    }

    // 5) Muoviti
    sf::Vector2f delta = m_direction * m_speed * dt;
    sf::Vector2f newPos = pos + delta;
    int tx = int(newPos.x / tileSize.x);
    int ty = int(newPos.y / tileSize.y);

    // 6) Se libero, muoviti
    if (tx>=0 && ty>=0 && tx<int(map.getSize().x) && ty<int(map.getSize().y)
        && !map.isWall(tx, ty))
    {
        m_shape.setPosition(newPos);
    }
    // 7) Altrimenti clamp e ferma direzione
    else {
        if (m_direction.x != 0.f) {
            float limitX = center.x + (m_direction.x>0
                ? (tileSize.x/2.f - r)
                : -(tileSize.x/2.f - r));
            m_shape.setPosition(sf::Vector2f(limitX, pos.y));
        } else if (m_direction.y != 0.f) {
            float limitY = center.y + (m_direction.y>0
                ? (tileSize.y/2.f - r)
                : -(tileSize.y/2.f - r));
            m_shape.setPosition(sf::Vector2f(pos.x, limitY));
        }
        m_direction = {0.f,0.f};
    }
}

void Player::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    target.draw(m_shape, states);
}

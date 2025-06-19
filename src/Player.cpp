// src/Player.cpp
#include "Player.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>

Player::Player(float speed, const sf::Vector2f& startPos, const sf::Vector2u& tileSize)
: m_shape(tileSize.x * 0.4f)
, m_speed(speed)
, m_direction{0.f,0.f}
, m_nextDirection{0.f,0.f}
, m_tileSize(tileSize)
{
    m_shape.setFillColor(sf::Color::Yellow);
    m_shape.setOrigin({m_shape.getRadius(), m_shape.getRadius()});
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
    float radius = m_shape.getRadius();

    // 2) Cella corrente e centro
    unsigned cellX = static_cast<unsigned>(pos.x / tileSize.x);
    unsigned cellY = static_cast<unsigned>(pos.y / tileSize.y);
    sf::Vector2f cellCenter{
        cellX * float(tileSize.x) + tileSize.x/2.f,
        cellY * float(tileSize.y) + tileSize.y/2.f
    };
    bool atCenter = (std::abs(pos.x - cellCenter.x) < 1.f &&
                     std::abs(pos.y - cellCenter.y) < 1.f);

    // 3) Se la cella in m_nextDirection è libera ed è retromarcia o siamo al centro, accetta la svolta
    if (m_nextDirection != sf::Vector2f{0,0}) {
        int nx = int(cellX) + int(m_nextDirection.x);
        int ny = int(cellY) + int(m_nextDirection.y);
        bool inBounds = nx>=0 && ny>=0 && nx<int(map.getSize().x) && ny<int(map.getSize().y);
        if (inBounds && !map.isWall(nx, ny)) {
            bool isReverse = (m_nextDirection == -m_direction);
            if (isReverse || atCenter) {
                m_direction = m_nextDirection;
                // Riallinea al centro se siamo al centro
                if (atCenter) m_shape.setPosition(cellCenter);
                pos = m_shape.getPosition();
            }
        }
    }

    // 4) Calcola la nuova posizione
    sf::Vector2f delta = m_direction * m_speed * dt;
    sf::Vector2f newPos = pos + delta;
    int tx = int(newPos.x / tileSize.x);
    int ty = int(newPos.y / tileSize.y);

    // 5) Se cella libera → muoviti
    if (tx>=0 && ty>=0 && tx<int(map.getSize().x) && ty<int(map.getSize().y)
        && !map.isWall(tx, ty))
    {
        m_shape.setPosition(newPos);
    }
    // 6) Se muro → clamp sicuro, ma NON azzerare la direzione
    else {
        // Orizzontale
        if (m_direction.x != 0.f) {
            float limitX = cellCenter.x + (m_direction.x>0
                ? (tileSize.x/2.f - radius)
                : -(tileSize.x/2.f - radius));
            m_shape.setPosition(sf::Vector2f(limitX, pos.y));
        }
        // Verticale
        else if (m_direction.y != 0.f) {
            float limitY = cellCenter.y + (m_direction.y>0
                ? (tileSize.y/2.f - radius)
                : -(tileSize.y/2.f - radius));
            m_shape.setPosition(sf::Vector2f(pos.x, limitY));
        }
        // Nota: m_direction e m_nextDirection rimangono invariate
        // così al prossimo frame, se premi indietro o svoltura valida,
        // la logica (punto 3) li ricollegherà correttamente.
    }
}

void Player::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    target.draw(m_shape, states);
}

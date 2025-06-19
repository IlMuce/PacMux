// src/Player.cpp
#include "Player.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>

// Costruttore: inizializza Pac-Man con velocità, posizione e dimensione cella
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

// Aggiorna la posizione e la direzione di Pac-Man in base all'input e alle collisioni
void Player::update(float dt, const TileMap& map, const sf::Vector2u& tileSize) {
    using Key = sf::Keyboard::Key;

    // Leggi input direzionale
    if      (sf::Keyboard::isKeyPressed(Key::Left))  m_nextDirection = {-1.f,  0.f};
    else if (sf::Keyboard::isKeyPressed(Key::Right)) m_nextDirection = { 1.f,  0.f};
    else if (sf::Keyboard::isKeyPressed(Key::Up))    m_nextDirection = { 0.f, -1.f};
    else if (sf::Keyboard::isKeyPressed(Key::Down))  m_nextDirection = { 0.f,  1.f};

    sf::Vector2f pos = m_shape.getPosition();
    float r = m_shape.getRadius();

    // Calcola cella corrente e centro della cella
    unsigned cellX = unsigned(pos.x / tileSize.x);
    unsigned cellY = unsigned(pos.y / tileSize.y);
    sf::Vector2f center{
        cellX * float(tileSize.x) + tileSize.x/2.f,
        cellY * float(tileSize.y) + tileSize.y/2.f
    };

    // Tolleranza per l'allineamento al centro cella
    float tolX = tileSize.x / 4.f;
    float tolY = tileSize.y / 4.f;

    // Se retromarcia, accetta subito la nuova direzione
    if (m_nextDirection == -m_direction && m_direction != sf::Vector2f{0,0}) {
        m_direction = m_nextDirection;
    }
    // Altrimenti, prova a svoltare se sei allineato o fermo
    else if (m_nextDirection != m_direction) {
        bool canTurn = false;
        // Svolta verticale da orizzontale
        if (m_direction.x != 0.f && m_nextDirection.y != 0.f
            && std::abs(pos.x - center.x) < tolX)
            canTurn = true;
        // Svolta orizzontale da verticale
        if (m_direction.y != 0.f && m_nextDirection.x != 0.f
            && std::abs(pos.y - center.y) < tolY)
            canTurn = true;
        // Se fermo, svolta sempre
        if (m_direction == sf::Vector2f{0,0})
            canTurn = true;
        // Se esattamente al centro
        if (std::abs(pos.x - center.x) < 1.f && std::abs(pos.y - center.y) < 1.f)
            canTurn = true;

        if (canTurn) {
            int nx = int(cellX) + int(m_nextDirection.x);
            int ny = int(cellY) + int(m_nextDirection.y);
            bool inBounds = nx>=0 && ny>=0
                         && nx<int(map.getSize().x) && ny<int(map.getSize().y);
            if (inBounds && !map.isWall(nx,ny)) {
                m_direction = m_nextDirection;
                m_shape.setPosition(center); // riallinea
                pos = center;
            }
        }
    }

    // Calcola nuova posizione
    sf::Vector2f delta = m_direction * m_speed * dt;
    sf::Vector2f newPos = pos + delta;
    int tx = int(newPos.x / tileSize.x);
    int ty = int(newPos.y / tileSize.y);

    // Se la cella è libera, muovi Pac-Man
    if (tx>=0 && ty>=0 && tx<int(map.getSize().x) && ty<int(map.getSize().y)
        && !map.isWall(tx, ty))
    {
        m_shape.setPosition(newPos);
    }
    // Altrimenti, blocca il movimento e riallinea
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

// Disegna Pac-Man sulla finestra
void Player::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    target.draw(m_shape, states);
}

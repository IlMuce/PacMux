#include "Pinky.hpp"
#include "Ghost.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

// Pinky: il fantasma rosa, mira 4 caselle avanti a Pac-Man
Pinky::Pinky(const sf::Vector2f& pos) : Ghost(pos, sf::Color::Magenta, 12.0f, Type::Pinky) {
    m_texture = std::make_unique<sf::Texture>();
    if (m_texture->loadFromFile("assets/pacman.png")) {
        m_sprite = std::make_unique<sf::Sprite>(*m_texture);
        m_sprite->setTextureRect(PINKY_FRAMES[2][0]); // frame iniziale: destra, anim 0
        m_sprite->setOrigin(sf::Vector2f{8.f, 8.f});
        float scale = 12.f / 8.f;
        m_sprite->setScale(sf::Vector2f{scale, scale});
        m_hasTexture = true;
    } else {
        m_hasTexture = false;
    }
    m_animTime = 0.f;
    m_animFrame = 0;
}

// Target = 4 caselle avanti nella direzione di Pac-Man
sf::Vector2f Pinky::calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                                  const TileMap& map, const sf::Vector2u& tileSize) {
    // Targeting classico: 4 celle avanti
    sf::Vector2f target = pacmanPos;
    if (std::hypot(pacmanDirection.x, pacmanDirection.y) > 0.1f) {
        sf::Vector2f dir = pacmanDirection / std::hypot(pacmanDirection.x, pacmanDirection.y);
        target.x += 4.0f * dir.x * float(tileSize.x);
        target.y += 4.0f * dir.y * float(tileSize.y);
        
        // Clamp migliorato: assicura che il target sia sempre dentro i confini validi
        int w = map.getSize().x, h = map.getSize().y;
        target.x = std::max(float(tileSize.x/2), std::min(target.x, (w-1) * float(tileSize.x) + float(tileSize.x/2)));
        target.y = std::max(float(tileSize.y/2), std::min(target.y, (h-1) * float(tileSize.y) + float(tileSize.y/2)));
        
        // Verifica che il target sia su una cella accessibile (non muro)
        int targetTileX = static_cast<int>(target.x / tileSize.x);
        int targetTileY = static_cast<int>(target.y / tileSize.y);
        
        // Se il target è su un muro o fuori dai confini, usa la posizione di Pac-Man
        if (targetTileX < 0 || targetTileX >= w || targetTileY < 0 || targetTileY >= h ||
            map.isWall(targetTileX, targetTileY)) {
            target = pacmanPos;
        }
    }
    return target;
}

void Pinky::update(float dt, const TileMap& map, const sf::Vector2u& tileSize,
                  const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode, bool gameStarted, bool released) {
    if (m_hasTexture && m_sprite) {
        m_sprite->setPosition(m_shape.getPosition());
        m_animTime += dt;
        if (m_direction != sf::Vector2f{0,0}) {
            if (m_animTime >= GHOST_ANIMATION_INTERVAL) {
                m_animFrame = 1 - m_animFrame;
                m_animTime = 0.f;
            }
        } else {
            m_animFrame = 0;
        }
        int dir = 2;
        if (m_direction.x < 0) dir = 0;
        else if (m_direction.x > 0) dir = 2;
        else if (m_direction.y < 0) dir = 1;
        else if (m_direction.y > 0) dir = 3;
        if (m_isFrightened) {
            m_sprite->setTextureRect(FRIGHTENED_FRAMES[m_animFrame]);
        } else {
            m_sprite->setTextureRect(PINKY_FRAMES[dir][m_animFrame]);
        }
    }
    Ghost::update(dt, map, tileSize, pacmanPos, pacmanDirection, mode, gameStarted);
}

void Pinky::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    if (m_hasTexture && m_sprite) {
        m_sprite->setPosition(m_drawPos);
        if (m_eaten || m_isReturningToHouse) {
            int dir = 2;
            if (m_direction.x < 0) dir = 0;
            else if (m_direction.x > 0) dir = 2;
            else if (m_direction.y < 0) dir = 1;
            else if (m_direction.y > 0) dir = 3;
            m_sprite->setTextureRect(EYES_FRAMES[dir]);
        } else if (m_isFrightened) {
            m_sprite->setTextureRect(FRIGHTENED_FRAMES[m_animFrame % 2]);
        } else {
            int dir = 2;
            if (m_direction.x < 0) dir = 0;
            else if (m_direction.x > 0) dir = 2;
            else if (m_direction.y < 0) dir = 1;
            else if (m_direction.y > 0) dir = 3;
            m_sprite->setTextureRect(PINKY_FRAMES[dir][m_animFrame]);
        }
        target.draw(*m_sprite, states);
    } else {
        sf::CircleShape shape = m_shape;
        shape.setPosition(m_drawPos);
        target.draw(shape, states);
    }
}

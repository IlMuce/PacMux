#include "Pinky.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

// Pinky: il fantasma rosa, mira 4 caselle avanti a Pac-Man
Pinky::Pinky(const sf::Vector2f& pos) : Ghost(pos, sf::Color::Magenta, 12.0f, Type::Pinky) {}

// Target = 4 caselle avanti nella direzione di Pac-Man
sf::Vector2f Pinky::calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                                  const TileMap& map, const sf::Vector2u& tileSize) {
    // Targeting classico: 4 celle avanti
    sf::Vector2f target = pacmanPos;
    if (std::hypot(pacmanDirection.x, pacmanDirection.y) > 0.1f) {
        sf::Vector2f dir = pacmanDirection / std::hypot(pacmanDirection.x, pacmanDirection.y);
        target.x += 4.0f * dir.x * float(tileSize.x);
        target.y += 4.0f * dir.y * float(tileSize.y);
        // Clamp ai bordi
        int w = map.getSize().x, h = map.getSize().y;
        target.x = std::max(float(tileSize.x/2), std::min(target.x, (w-1) * float(tileSize.x) + float(tileSize.x/2)));
        target.y = std::max(float(tileSize.y/2), std::min(target.y, (h-1) * float(tileSize.y) + float(tileSize.y/2)));
    }
    return target;
}

void Pinky::update(float dt, const TileMap& map, const sf::Vector2u& tileSize,
                  const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode) {
    static float debugTimer = 0.f;
    m_mode = mode;
    sf::Vector2f pos = m_shape.getPosition();
    debugTimer += dt;
    std::string modeStr = (m_mode == Mode::Chase) ? "Chase" : (m_mode == Mode::Scatter) ? "Scatter" : "Other";
    sf::Vector2f target = calculateTarget(pacmanPos, pacmanDirection, map, tileSize);
    if (debugTimer >= 1.0f) {
        std::cout << "[PINKY] Pos: (" << pos.x << "," << pos.y << ") Dir: (" << m_direction.x << "," << m_direction.y << ") Pacman: (" << pacmanPos.x << "," << pacmanPos.y << ") Target: (" << target.x << "," << target.y << ") Mode: " << modeStr << std::endl;
        debugTimer = 0.f;
    }
    Ghost::update(dt, map, tileSize, pacmanPos, pacmanDirection, mode);
}

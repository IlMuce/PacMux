#include "Clyde.hpp"
#include <cmath>
#include <iostream>

Clyde::Clyde(const sf::Vector2f& pos) : Ghost(pos, sf::Color(255, 165, 0), 12.0f, Type::Clyde) {
}

void Clyde::update(float dt, const TileMap& map, const sf::Vector2u& tileSize,
                  const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode) {
    static float debugTimer = 0.f;
    m_mode = mode;
    sf::Vector2f pos = m_shape.getPosition();
    float cx = std::round((pos.x - tileSize.x/2.f) / tileSize.x);
    float cy = std::round((pos.y - tileSize.y/2.f) / tileSize.y);
    int sx = int(std::round(cx));
    int sy = int(std::round(cy));
    debugTimer += dt;
    if (debugTimer >= 1.0f) {
        std::cout << "[CLYDE] Pos: (" << pos.x << ", " << pos.y << ") Dir: (" << m_direction.x << ", " << m_direction.y << ") ";
        std::cout << "Pac-Man: (" << pacmanPos.x << ", " << pacmanPos.y << ") ";
        std::cout << "Mode: ";
        if (map.isGhostHouse(sx, sy)) std::cout << "GhostHouse\n";
        else if (m_mode == Mode::Chase) std::cout << "Chase\n";
        else if (m_mode == Mode::Scatter) std::cout << "Scatter\n";
        else std::cout << "Other\n";
        debugTimer = 0.f;
    }
    if (map.isGhostHouse(sx, sy)) {
        // Prova su, destra, sinistra per uscire
        static const sf::Vector2f dirs[] = {{0,-1}, {1,0}, {-1,0}};
        for (const auto& dir : dirs) {
            if (canMove(dir, map, tileSize)) {
                m_direction = dir;
                break;
            }
        }
    } else {
        // Comportamento normale: delega alla base
        Ghost::update(dt, map, tileSize, pacmanPos, pacmanDirection, mode);
        return;
    }
    // Movimento semplice (come base)
    float nextX = cx + m_direction.x;
    float nextY = cy + m_direction.y;
    int w = map.getSize().x, h = map.getSize().y;
    if (nextY == 10) {
        if (nextX < 0) nextX = w - 1;
        else if (nextX >= w) nextX = 0;
    }
    if (nextX >= 0 && nextX < w && nextY >= 0 && nextY < h && canMove(m_direction, map, tileSize)) {
        sf::Vector2f dest{nextX * float(tileSize.x) + tileSize.x/2.f, nextY * float(tileSize.y) + tileSize.y/2.f};
        sf::Vector2f delta = dest - m_shape.getPosition();
        float step = m_speed * dt;
        if (std::hypot(delta.x, delta.y) <= step) {
            m_shape.setPosition(dest);
        } else {
            float deltaLen = std::hypot(delta.x, delta.y);
            if (deltaLen > 0) {
                sf::Vector2f normalizedDelta = delta / deltaLen;
                m_shape.move(normalizedDelta * step);
            }
        }
    }
    m_drawPos = m_shape.getPosition();
}

sf::Vector2f Clyde::calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection,
                                   const TileMap& map, const sf::Vector2u& tileSize) {
    // Targeting classico Clyde
    sf::Vector2f pos = m_shape.getPosition();
    float dist = std::hypot(pacmanPos.x - pos.x, pacmanPos.y - pos.y);
    float cellDist = dist / float(tileSize.x); // Supponiamo tile quadrati
    int w = map.getSize().x, h = map.getSize().y;
    if (cellDist > 8.0f) {
        return pacmanPos;
    } else {
        return {0, (h-1) * float(tileSize.y)}; // Angolo scatter in basso a sinistra
    }
}

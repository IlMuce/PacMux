#include "Inky.hpp"
#include <cmath>
#include <iostream>

// Inky: targeting collaborativo (Blinky + Pac-Man)
Inky::Inky(const sf::Vector2f& pos) : Ghost(pos, sf::Color::Cyan, 12.0f, Type::Inky) {}

// Target = punto ottenuto proiettando il vettore da Blinky a 2 celle davanti a Pac-Man, raddoppiato
sf::Vector2f Inky::calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection,
                                  const TileMap& map, const sf::Vector2u& tileSize, const sf::Vector2f& blinkyPos) {
    // Targeting classico Inky
    sf::Vector2f ahead = pacmanPos;
    if (std::hypot(pacmanDirection.x, pacmanDirection.y) > 0.1f) {
        sf::Vector2f dir = pacmanDirection / std::hypot(pacmanDirection.x, pacmanDirection.y);
        ahead.x += 2.0f * dir.x * float(tileSize.x);
        ahead.y += 2.0f * dir.y * float(tileSize.y);
    }
    sf::Vector2f vec = ahead - blinkyPos;
    sf::Vector2f target = blinkyPos + 2.0f * vec;
    // Clamp ai bordi
    int w = map.getSize().x, h = map.getSize().y;
    target.x = std::max(float(tileSize.x/2), std::min(target.x, (w-1) * float(tileSize.x) + float(tileSize.x/2)));
    target.y = std::max(float(tileSize.y/2), std::min(target.y, (h-1) * float(tileSize.y) + float(tileSize.y/2)));
    return target;
}

// Implementazione richiesta dal base class Ghost (fallback: usa origine mappa come Blinky se non fornito)
sf::Vector2f Inky::calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection,
                                   const TileMap& map, const sf::Vector2u& tileSize) {
    // Fallback: usa (0,0) come posizione di Blinky se non specificata
    return calculateTarget(pacmanPos, pacmanDirection, map, tileSize, sf::Vector2f(0,0));
}

// Overloaded update: uses Blinky's position for targeting
void Inky::update(float dt, const TileMap& map, const sf::Vector2u& tileSize,
                  const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode,
                  const sf::Vector2f& blinkyPos) {
    // Se è in stato eaten/returning, lascia che la base gestisca tutto!
    if (m_eaten || m_isReturningToHouse) {
        Ghost::update(dt, map, tileSize, pacmanPos, pacmanDirection, mode);
        return;
    }
    static float debugTimer = 0.f;
    m_mode = mode;
    sf::Vector2f pos = m_shape.getPosition();
    float cx = std::round((pos.x - tileSize.x/2.f) / tileSize.x);
    float cy = std::round((pos.y - tileSize.y/2.f) / tileSize.y);
    int sx = int(std::round(cx));
    int sy = int(std::round(cy));
    debugTimer += dt;
    std::string modeStr = (m_mode == Mode::Chase) ? "Chase" : (m_mode == Mode::Scatter) ? "Scatter" : "Other";
    sf::Vector2f target = calculateTarget(pacmanPos, pacmanDirection, map, tileSize, blinkyPos);
    if (debugTimer >= 1.0f) {
        if (map.isGhostHouse(sx, sy)) {
            std::cout << "[INKY] GHOUSE Pos: (" << pos.x << "," << pos.y << ") Dir: (" << m_direction.x << "," << m_direction.y << ") Pacman: (" << pacmanPos.x << "," << pacmanPos.y << ") Target: (" << target.x << "," << target.y << ") Mode: " << modeStr << std::endl;
        } else {
            std::cout << "[INKY] OUT Pos: (" << pos.x << "," << pos.y << ") Dir: (" << m_direction.x << "," << m_direction.y << ") Pacman: (" << pacmanPos.x << "," << pacmanPos.y << ") Target: (" << target.x << "," << target.y << ") Mode: " << modeStr << std::endl;
        }
        debugTimer = 0.f;
    }
    if (map.isGhostHouse(sx, sy)) {
        // Usa il pathfinding verso la porta di uscita
        sf::Vector2f exit = getGhostHouseExit(map, tileSize);
        m_direction = findPath(exit, map, tileSize);
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
    } else {
        // Comportamento normale: delega alla base
        Ghost::update(dt, map, tileSize, pacmanPos, pacmanDirection, mode);
    }
}

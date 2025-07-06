#include "Ghost.hpp"
#include <cmath>
#include <iostream>

// =========================
// Classe base Ghost
// =========================
// Contiene tutta la logica comune di movimento, pathfinding greedy, tunnel, ghost house, scatter/chase.
// Ogni fantasma ridefinisce solo il calcolo del target (e opzionalmente il pathfinding).
// =========================

Ghost::Ghost(const sf::Vector2f& pos, sf::Color color, float radius, Type type)
    : m_shape(radius), m_direction(0, -1), m_speed(90.f), m_type(type), m_mode(Mode::Chase), 
      m_drawPos(pos), m_hasLeftGhostHouse(false)
{
    m_shape.setFillColor(color);
    m_shape.setOrigin({radius, radius});
    m_shape.setPosition(pos);
    m_target = pos;
}

void Ghost::update(float dt, const TileMap& map, const sf::Vector2u& tileSize, 
                  const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode) {
    // Solo Blinky e Pinky si muovono per ora
    if (m_type == Type::Inky || m_type == Type::Clyde) {
        m_drawPos = m_shape.getPosition();
        return;
    }
    
    m_mode = mode;
    sf::Vector2f pos = m_shape.getPosition();
    float cx = std::round((pos.x - tileSize.x/2.f) / tileSize.x);
    float cy = std::round((pos.y - tileSize.y/2.f) / tileSize.y);
    sf::Vector2f center{
        cx * tileSize.x + tileSize.x/2.f,
        cy * tileSize.y + tileSize.y/2.f
    };
    bool centered = (std::abs(pos.x - center.x) < 2.0f && std::abs(pos.y - center.y) < 2.0f);
    int w = map.getSize().x, h = map.getSize().y;
    
    // Aggiorna direzione solo se centrato sulla cella o bloccato
    bool shouldUpdateDirection = centered && 
                               ((m_direction.x == 0 && m_direction.y == -1) ||
                               !canMove(m_direction, map, tileSize));
                                
    if (shouldUpdateDirection) {
        int sx = int(std::round(cx));
        int sy = int(std::round(cy));
        // Gestione ghost house
        if (!m_hasLeftGhostHouse && !map.isGhostHouse(sx, sy)) m_hasLeftGhostHouse = true;
        if (m_hasLeftGhostHouse && map.isGhostHouse(sx, sy)) m_hasLeftGhostHouse = false;
        // Calcola il target (scatter/chase)
        sf::Vector2f target;
        if (m_mode == Mode::Scatter) {
            switch (m_type) {
                case Type::Blinky: target = {(w-1) * float(tileSize.x), 0}; break;
                case Type::Pinky:  target = {0, 0}; break;
                case Type::Inky:   target = {(w-1) * float(tileSize.x), (h-1) * float(tileSize.y)}; break;
                case Type::Clyde:  target = {0, (h-1) * float(tileSize.y)}; break;
            }
        } else {
            target = calculateTarget(pacmanPos, pacmanDirection, map, tileSize);
        }
        m_direction = findPath(target, map, tileSize);
    }
    // Movimento e gestione tunnel
    int nextX = int(std::round(cx + m_direction.x));
    int nextY = int(std::round(cy + m_direction.y));
    bool validMove = true;
    if (nextX < 0 && int(std::round(cy)) == 10) nextX = w - 1;
    else if (nextX >= w && int(std::round(cy)) == 10) nextX = 0;
    else if (nextX < 0 || nextX >= w) validMove = false;
    if (nextY < 0 || nextY >= h) validMove = false;
    if (validMove && canMove(m_direction, map, tileSize)) {
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
        // Teletrasporto tunnel
        float tunnelY = 10 * tileSize.y + tileSize.y/2.f;
        float mapWidth = map.getSize().x * tileSize.x;
        if (std::abs(m_shape.getPosition().y - tunnelY) < tileSize.y/4.f) {
            if (m_shape.getPosition().x < 0) {
                m_shape.setPosition({mapWidth - tileSize.x/2.f, m_shape.getPosition().y});
            } else if (m_shape.getPosition().x >= mapWidth) {
                m_shape.setPosition({tileSize.x/2.f, m_shape.getPosition().y});
            }
        }
    } else {
        // Se bloccato, forza aggiornamento direzione
        int sx = int(std::round(cx));
        int sy = int(std::round(cy));
        sf::Vector2f target;
        if (m_mode == Mode::Scatter) {
            switch (m_type) {
                case Type::Blinky: target = {(w-1) * float(tileSize.x), 0}; break;
                case Type::Pinky:  target = {0, 0}; break;
                case Type::Inky:   target = {(w-1) * float(tileSize.x), (h-1) * float(tileSize.y)}; break;
                case Type::Clyde:  target = {0, (h-1) * float(tileSize.y)}; break;
            }
        } else {
            target = calculateTarget(pacmanPos, pacmanDirection, map, tileSize);
        }
        m_direction = findPath(target, map, tileSize);
        if (m_direction == sf::Vector2f(0,0)) m_direction = {0, -1};
    }
    m_drawPos = m_shape.getPosition();
}

void Ghost::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    sf::CircleShape shape = m_shape;
    shape.setPosition(m_drawPos);
    target.draw(shape, states);
}

void Ghost::setPosition(const sf::Vector2f& pos) {
    m_shape.setPosition(pos);
    m_direction = {0, -1};
    m_drawPos = pos;
    m_hasLeftGhostHouse = false;
}

// Pathfinding greedy base: cerca la direzione che avvicina di più al target, evitando reverse se possibile
sf::Vector2f Ghost::findPath(const sf::Vector2f& target, const TileMap& map, const sf::Vector2u& tileSize) {
    sf::Vector2f pos = m_shape.getPosition();
    int startX = int(std::round((pos.x - tileSize.x/2.f) / tileSize.x));
    int startY = int(std::round((pos.y - tileSize.y/2.f) / tileSize.y));
    std::vector<sf::Vector2f> directions = {{0,-1}, {-1,0}, {0,1}, {1,0}};
    float minDist = 1e9f;
    sf::Vector2f bestDir = {0, -1};
    bool foundValidMove = false;
    for (const auto& dir : directions) {
        bool isReverse = (dir + m_direction == sf::Vector2f(0,0) && m_direction != sf::Vector2f(0,0));
        if (canMove(dir, map, tileSize)) {
            int nextX = startX + int(dir.x);
            int nextY = startY + int(dir.y);
            sf::Vector2f nextPos{nextX * float(tileSize.x) + tileSize.x/2.f, 
                               nextY * float(tileSize.y) + tileSize.y/2.f};
            float dist = std::hypot(target.x - nextPos.x, target.y - nextPos.y);
            if (!isReverse && (!foundValidMove || dist < minDist)) {
                minDist = dist;
                bestDir = dir;
                foundValidMove = true;
            }
        }
    }
    if (!foundValidMove) {
        for (const auto& dir : directions) {
            if (canMove(dir, map, tileSize)) {
                int nextX = startX + int(dir.x);
                int nextY = startY + int(dir.y);
                sf::Vector2f nextPos{nextX * float(tileSize.x) + tileSize.x/2.f, 
                                   nextY * float(tileSize.y) + tileSize.y/2.f};
                float dist = std::hypot(target.x - nextPos.x, target.y - nextPos.y);
                if (!foundValidMove || dist < minDist) {
                    minDist = dist;
                    bestDir = dir;
                    foundValidMove = true;
                }
            }
        }
    }
    return bestDir;
}

// Controlla se la direzione è valida (no muri, no ghost house dopo essere usciti, gestisce tunnel)
bool Ghost::canMove(const sf::Vector2f& direction, const TileMap& map, const sf::Vector2u& tileSize) {
    sf::Vector2f pos = m_shape.getPosition();
    float cx = std::round((pos.x - tileSize.x/2.f) / tileSize.x);
    float cy = std::round((pos.y - tileSize.y/2.f) / tileSize.y);
    int nextX = int(std::round(cx + direction.x));
    int nextY = int(std::round(cy + direction.y));
    int w = map.getSize().x, h = map.getSize().y;
    // Tunnel orizzontali (riga 10)
    if (nextX < 0 && int(std::round(cy)) == 10) nextX = w - 1;
    else if (nextX >= w && int(std::round(cy)) == 10) nextX = 0;
    else if (nextX < 0 || nextX >= w) return false;
    if (nextY < 0 || nextY >= h) return false;
    if (nextX < 0 || nextX >= w || nextY < 0 || nextY >= h) return false;
    if (map.isWall(nextX, nextY)) return false;
    if (m_hasLeftGhostHouse && map.isGhostHouse(nextX, nextY)) return false;
    return true;
}

#include "Inky.hpp"
#include "Ghost.hpp"
#include <cmath>
#include <iostream>

// Inky: targeting collaborativo (Blinky + Pac-Man)
Inky::Inky(const sf::Vector2f& pos) : Ghost(pos, sf::Color::Cyan, 12.0f, Type::Inky) {
    m_texture = std::make_unique<sf::Texture>();
    if (m_texture->loadFromFile("assets/pacman.png")) {
        m_sprite = std::make_unique<sf::Sprite>(*m_texture);
        m_sprite->setTextureRect(INKY_FRAMES[2][0]); // frame iniziale: destra, anim 0
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
    
    return target;
}

// Implementazione richiesta dal base class Ghost (fallback: usa origine mappa come Blinky se non fornito)
sf::Vector2f Inky::calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection,
                                   const TileMap& map, const sf::Vector2u& tileSize) {
    // Fallback: usa (0,0) come posizione di Blinky se non specificata
    return calculateTarget(pacmanPos, pacmanDirection, map, tileSize, sf::Vector2f(float(tileSize.x/2), float(tileSize.y/2)));
}

// Overloaded update: uses Blinky's position for targeting and release logic
void Inky::update(float dt, const TileMap& map, const sf::Vector2u& tileSize,
                  const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode,
                  const sf::Vector2f& blinkyPos, bool gameStarted, bool released) {
    if (!m_released) {
        m_drawPos = m_shape.getPosition();
        return;
    }
    // Se è in stato eaten/returning, lascia che la base gestisca tutto!
    if (m_eaten || m_isReturningToHouse) {
        Ghost::update(dt, map, tileSize, pacmanPos, pacmanDirection, mode, gameStarted);
        return;
    }
    m_mode = mode;
    sf::Vector2f pos = m_shape.getPosition();
    float cx = std::round((pos.x - tileSize.x/2.f) / tileSize.x);
    float cy = std::round((pos.y - tileSize.y/2.f) / tileSize.y);
    int sx = int(std::round(cx));
    int sy = int(std::round(cy));
    static float debugTimer = 0.f;
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
            sf::Vector2f dest{nextX * float(tileSize.x) + tileSize.x/2.f, nextY * float(tileSize.y) + float(tileSize.y/2.f)};
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
        Ghost::update(dt, map, tileSize, pacmanPos, pacmanDirection, mode, gameStarted);
    }
}

void Inky::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    if (m_hasTexture && m_sprite) {
        m_sprite->setPosition(m_drawPos);
        target.draw(*m_sprite, states);
    } else {
        sf::CircleShape shape = m_shape;
        shape.setPosition(m_drawPos);
        target.draw(shape, states);
    }
}

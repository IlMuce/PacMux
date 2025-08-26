#include "Ghost.hpp"
#include <cmath>
#include <iostream>
#include <algorithm> // for std::random_shuffle
#include <random>
#include <ctime>

// =========================
// Classe base Ghost
// =========================
// Contiene tutta la logica comune di movimento, pathfinding greedy, tunnel, ghost house, scatter/chase.
// Ogni fantasma ridefinisce solo il calcolo del target (e opzionalmente il pathfinding).
// =========================

Ghost::Ghost(const sf::Vector2f& pos, sf::Color color, float radius, Type type)
    : m_shape(radius), m_direction(0, -1), m_speed(90.f), m_type(type), m_mode(Mode::Chase), 
      m_drawPos(pos), m_hasLeftGhostHouse(false), m_eaten(false), m_isReturningToHouse(false)
{
    m_shape.setFillColor(color);
    m_shape.setOrigin({radius, radius});
    m_shape.setPosition(pos);
    m_target = pos;
    // Imposta il delay di uscita classico
    switch (m_type) {
        case Type::Blinky: m_releaseDelay = 0.f; break;
        case Type::Pinky:  m_releaseDelay = 2.f; break;
        case Type::Inky:   m_releaseDelay = 4.f; break;
        case Type::Clyde:  m_releaseDelay = 6.f; break;
    }
    m_canLeaveHouse = false;
}

void Ghost::update(float dt, const TileMap& map, const sf::Vector2u& tileSize, 
                  const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode, bool gameStarted) {
    // Blocca il fantasma finché non è rilasciato
    if (!m_released) {
        m_drawPos = m_shape.getPosition();
        return;
    }
    // --- Ghost eaten/respawn logic: gestisci PRIMA di tutto ---
    if (m_isReturningToHouse) {
        int houseX = 10, houseY = 10;
        sf::Vector2f houseCenter = {houseX * float(tileSize.x) + tileSize.x/2.f, houseY * float(tileSize.y) + tileSize.y/2.f};
        sf::Vector2f pos = m_shape.getPosition();
        float distToHouse = std::hypot(pos.x - houseCenter.x, pos.y - houseCenter.y);
        if (distToHouse < tileSize.x * 1.0f) {
            std::cout << "[GHOST] Arrived at ghost house, waiting to respawn.\n";
            m_isReturningToHouse = false;
            m_eaten = true;
            m_speed = 0.f;
            m_respawnTimer = 0.f;
            m_shape.setPosition(houseCenter);
            m_drawPos = houseCenter;
            return;
        }
        sf::Vector2f delta = houseCenter - pos;
        float step = m_speed * dt;
        float dist = std::hypot(delta.x, delta.y);
        if (dist > 0) {
            sf::Vector2f normalizedDelta = delta / dist;
            m_shape.move(normalizedDelta * step);
        }
        m_drawPos = m_shape.getPosition();
        return;
    }
    if (m_eaten) {
        m_respawnTimer += dt;
        if (m_respawnTimer >= m_respawnDuration) {
            std::cout << "[GHOST] Respawn timer done, ghost leaves house.\n";
            m_eaten = false;
            m_speed = 90.f;
            m_isFrightened = false;
            m_direction = {0, -1};
            m_hasLeftGhostHouse = false; // deve uscire di nuovo!
            m_shape.move(sf::Vector2f(0, -static_cast<float>(tileSize.y)));
            m_drawPos = m_shape.getPosition();
        } else {
            m_drawPos = m_shape.getPosition();
            return;
        }
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

    // Release logic is now handled in main.cpp cascade system
    // m_canLeaveHouse is set via setReleased() from main.cpp
    // Blocca il movimento dei fantasmi finché il gioco non è partito
    if (!gameStarted) {
        m_drawPos = m_shape.getPosition();
        return;
    }

    // Check ghost house status every frame (moved outside shouldUpdateDirection)
    int sx = int(std::round(cx));
    int sy = int(std::round(cy));
    if (!m_hasLeftGhostHouse && !map.isGhostHouse(sx, sy) && m_canLeaveHouse) {
        m_hasLeftGhostHouse = true;
        std::cout << "[GHOST] " << Ghost::getTypeName(m_type) << " has left ghost house at (" << sx << "," << sy << ")!\n";
    }
    // Resetta solo se il fantasma è nella ghost house centrale (tile 10,10)
    if (m_hasLeftGhostHouse && map.isGhostHouse(sx, sy) && !m_isReturningToHouse && sx == 10 && sy == 10) {
        std::cout << "[GHOST] " << Ghost::getTypeName(m_type) << " re-entered ghost house at (" << sx << "," << sy << ")!\n";
        m_hasLeftGhostHouse = false;
    }

    bool shouldUpdateDirection = centered && 
                               ((m_direction.x == 0 && m_direction.y == -1) ||
                               !canMove(m_direction, map, tileSize));
    if (shouldUpdateDirection) {
        // Uscita forzata dalla ghost house per tutti i fantasmi
        sf::Vector2f target;
        if (map.isGhostHouse(sx, sy)) {
            // Solo se può uscire
            if (m_canLeaveHouse) {
                target = getGhostHouseExit(map, tileSize);
            } else {
                target = {sx * float(tileSize.x) + tileSize.x/2.f, sy * float(tileSize.y) + tileSize.y/2.f}; // resta fermo
            }
        } else if (m_mode == Mode::Scatter) {
            switch (m_type) {
                case Type::Blinky: target = {(w-1) * float(tileSize.x), 0}; break;
                case Type::Pinky:  target = {0, 0}; break;
                case Type::Inky:   target = {(w-1) * float(tileSize.x), (h-1) * float(tileSize.y)}; break;
                case Type::Clyde:  target = {0, (h-1) * float(tileSize.y)}; break;
            }
            // sf::Vector2f pos = m_shape.getPosition();
            // std::cout << "[SCATTER] " << Ghost::getTypeName(m_type) << " Target: (" << target.x << ", " << target.y << ") Pos: (" << pos.x << ", " << pos.y << ")" << std::endl;
        } else {
            target = calculateTarget(pacmanPos, pacmanDirection, map, tileSize);
        }
        m_direction = findPath(target, map, tileSize);
    }
    // Movimento e gestione tunnel
    int nextX = int(std::round(cx + m_direction.x));
    int nextY = int(std::round(cy + m_direction.y));
    bool validMove = true;
    
    // Gestione tunnel automatica: rileva tunnel secondo la regola specifica
    if (nextY >= 0 && nextY < h) {
        if (nextX < 0) {
            // Controlla se è un tunnel valido a sinistra
            bool isTunnel = (map.getData()[nextY][0] == '2');
            if (nextY > 0) isTunnel = isTunnel && (map.getData()[nextY-1][0] == '1');
            if (nextY < h-1) isTunnel = isTunnel && (map.getData()[nextY+1][0] == '1');
            
            if (isTunnel) {
                nextX = w - 1; // Tunnel: entra dal lato destro
            }
        } else if (nextX >= w) {
            // Controlla se è un tunnel valido a destra
            bool isTunnel = (map.getData()[nextY][w-1] == '2');
            if (nextY > 0) isTunnel = isTunnel && (map.getData()[nextY-1][w-1] == '1');
            if (nextY < h-1) isTunnel = isTunnel && (map.getData()[nextY+1][w-1] == '1');
            
            if (isTunnel) {
                nextX = 0; // Tunnel: entra dal lato sinistro
            }
        }
    }
    
    // Dopo la gestione tunnel, controlla i limiti
    if (nextX < 0 || nextX >= w) validMove = false;
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
        // Tunnel teleport automatico: rileva tunnel secondo la regola specifica
        float mapWidth = map.getSize().x * tileSize.x;
        sf::Vector2f currentPos = m_shape.getPosition();
        int currentTileY = static_cast<int>(currentPos.y / tileSize.y);
        
        // Controlla se la riga corrente è un tunnel valido
        if (currentTileY >= 0 && currentTileY < h) {
            bool isTunnel = (map.getData()[currentTileY][0] == '2' && map.getData()[currentTileY][w-1] == '2');
            if (currentTileY > 0) {
                isTunnel = isTunnel && (map.getData()[currentTileY-1][0] == '1' && map.getData()[currentTileY-1][w-1] == '1');
            }
            if (currentTileY < h-1) {
                isTunnel = isTunnel && (map.getData()[currentTileY+1][0] == '1' && map.getData()[currentTileY+1][w-1] == '1');
            }
            
            if (isTunnel) {
                if (currentPos.x < 0) {
                    m_shape.setPosition({mapWidth - tileSize.x/2.f, currentPos.y});
                } else if (currentPos.x >= mapWidth) {
                    m_shape.setPosition({tileSize.x/2.f, currentPos.y});
                }
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

    // --- Frightened mode logic ---
    if (m_isFrightened) {
        m_frightenedTimer += dt;
        if (m_frightenedTimer >= m_frightenedDuration) {
            m_isFrightened = false;
            m_mode = Mode::Chase;
            m_speed = 90.f; // restore normal speed
        }
    }

    // If frightened, move randomly at intersections
    if (m_isFrightened && centered) {
        std::vector<sf::Vector2f> directions = {{0,-1}, {1,0}, {0,1}, {-1,0}};
        // Shuffle directions for randomness (C++17+)
        std::shuffle(directions.begin(), directions.end(), std::default_random_engine(static_cast<unsigned>(std::time(nullptr))));
        for (const auto& dir : directions) {
            bool isReverse = (dir + m_direction == sf::Vector2f(0,0) && m_direction != sf::Vector2f(0,0));
            if (canMove(dir, map, tileSize) && !isReverse) {
                m_direction = dir;
                break;
            }
        }
    }

    // --- Ghost eaten/respawn logic ---
    if (m_isReturningToHouse) {
        // Move towards ghost house center (e.g., tile 10,10)
        int houseX = 10, houseY = 10;
        sf::Vector2f houseCenter = {houseX * float(tileSize.x) + tileSize.x/2.f, houseY * float(tileSize.y) + tileSize.y/2.f};
        sf::Vector2f pos = m_shape.getPosition();
        float distToHouse = std::hypot(pos.x - houseCenter.x, pos.y - houseCenter.y);
        if (distToHouse < tileSize.x * 1.0f) {
            // Arrived at ghost house
            std::cout << "[GHOST] Arrived at ghost house, waiting to respawn.\n";
            m_isReturningToHouse = false;
            m_eaten = true;
            m_speed = 0.f;
            m_respawnTimer = 0.f;
            m_shape.setPosition(houseCenter);
            m_drawPos = houseCenter;
            return;
        }
        // Move towards house
        sf::Vector2f delta = houseCenter - pos;
        float step = m_speed * dt;
        float dist = std::hypot(delta.x, delta.y);
        if (dist > 0) {
            sf::Vector2f normalizedDelta = delta / dist;
            m_shape.move(normalizedDelta * step);
        }
        m_drawPos = m_shape.getPosition();
        return;
    }
    if (m_eaten) {
        // Wait in ghost house, then respawn
        m_respawnTimer += dt;
        if (m_respawnTimer >= m_respawnDuration) {
            std::cout << "[GHOST] Respawn timer done, ghost leaves house.\n";
            m_eaten = false;
            m_speed = 90.f;
            m_isFrightened = false;
            m_direction = {0, -1};
            m_hasLeftGhostHouse = false; // deve uscire di nuovo!
            // Place just outside ghost house
            m_shape.move(sf::Vector2f(0, -static_cast<float>(tileSize.y)));
            m_drawPos = m_shape.getPosition();
        } else {
            m_drawPos = m_shape.getPosition();
            return;
        }
    }
}

void Ghost::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    sf::CircleShape shape = m_shape;
    shape.setPosition(m_drawPos);
    if (m_isFrightened) {
        // Lampeggia tra blu e bianco negli ultimi 2 secondi
        if (m_frightenedDuration - m_frightenedTimer < 2.f) {
            int blink = int(m_frightenedTimer * 8) % 2;
            shape.setFillColor(blink ? sf::Color(255,255,255) : sf::Color(0,0,255));
        } else {
            shape.setFillColor(sf::Color(0, 0, 255));
        }
    } else if (m_eaten || m_isReturningToHouse) {
        shape.setFillColor(sf::Color(200,200,200)); // Gray/white for eyes
    }
    target.draw(shape, states);
}

void Ghost::setPosition(const sf::Vector2f& pos) {
    m_shape.setPosition(pos);
    m_direction = {0, -1};
    m_drawPos = pos;
    m_hasLeftGhostHouse = false;
}

void Ghost::setFrightened(float duration) {
    if (duration <= 0.f) {
        m_isFrightened = false;
        m_frightenedTimer = 0.f;
        m_frightenedDuration = 0.f;
        m_mode = Mode::Chase;
        m_speed = 90.f;
        return;
    }
    m_isFrightened = true;
    m_frightenedTimer = 0.f;
    m_frightenedDuration = duration;
    m_mode = Mode::Frightened;
    m_speed = 60.f; // slower when frightened
    // Reverse direction
    m_direction = {-m_direction.x, -m_direction.y};
}

void Ghost::setEaten(bool eaten) {
    if (eaten) {
        m_isFrightened = false;
        m_eaten = true;
        m_isReturningToHouse = true;
        m_speed = 180.f; // fast return to house
        m_respawnTimer = 0.f;
        std::cout << "[GHOST] EATEN! Returning to ghost house as eyes.\n";
    } else {
        m_eaten = false;
        m_isReturningToHouse = false;
        m_speed = 90.f;
        std::cout << "[GHOST] Respawned, back to normal.\n";
    }
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
    
    // Gestione tunnel automatica: rileva tunnel secondo la regola specifica
    if (nextY >= 0 && nextY < h) { // Assicurati che Y sia valido
        if (nextX < 0) {
            // Controlla se è un tunnel valido a sinistra:
            // - Prima cella della riga corrente = '2'
            // - Prima cella riga sopra = '1' (se esiste)
            // - Prima cella riga sotto = '1' (se esiste)
            bool isTunnel = (map.getData()[nextY][0] == '2');
            if (nextY > 0) isTunnel = isTunnel && (map.getData()[nextY-1][0] == '1');
            if (nextY < h-1) isTunnel = isTunnel && (map.getData()[nextY+1][0] == '1');
            
            if (isTunnel) {
                nextX = w - 1; // Tunnel: entra dal lato destro
            } else {
                return false; // No tunnel disponibile
            }
        } else if (nextX >= w) {
            // Controlla se è un tunnel valido a destra:
            // - Ultima cella della riga corrente = '2'
            // - Ultima cella riga sopra = '1' (se esiste)
            // - Ultima cella riga sotto = '1' (se esiste)
            bool isTunnel = (map.getData()[nextY][w-1] == '2');
            if (nextY > 0) isTunnel = isTunnel && (map.getData()[nextY-1][w-1] == '1');
            if (nextY < h-1) isTunnel = isTunnel && (map.getData()[nextY+1][w-1] == '1');
            
            if (isTunnel) {
                nextX = 0; // Tunnel: entra dal lato sinistro
            } else {
                return false; // No tunnel disponibile
            }
        }
    }
    
    // Dopo la gestione tunnel, controlla i limiti
    if (nextX < 0 || nextX >= w || nextY < 0 || nextY >= h) return false;
    if (map.isWall(nextX, nextY)) return false;
    if (m_hasLeftGhostHouse && map.isGhostHouse(nextX, nextY)) return false;
    return true;
}

// Trova la porta di uscita della ghost house: cerca verso l'alto, poi lateralmente se serve
sf::Vector2f Ghost::getGhostHouseExit(const TileMap& map, const sf::Vector2u& tileSize) const {
    sf::Vector2f pos = m_shape.getPosition();
    int x = int(std::round((pos.x - tileSize.x/2.f) / tileSize.x));
    int y = int(std::round((pos.y - tileSize.y/2.f) / tileSize.y));
    // Cerca verso l'alto
    for (int yy = y-1; yy >= 0; --yy) {
        if (!map.isGhostHouse(x, yy) && !map.isWall(x, yy)) {
            return {x * float(tileSize.x) + tileSize.x/2.f, yy * float(tileSize.y) + tileSize.y/2.f};
        }
    }
    // Se non trova sopra, cerca lateralmente sulla stessa riga
    for (int dx = 1; dx < 5; ++dx) {
        if (x-dx >= 0 && !map.isGhostHouse(x-dx, y) && !map.isWall(x-dx, y))
            return {(x-dx) * float(tileSize.x) + tileSize.x/2.f, y * float(tileSize.y) + tileSize.y/2.f};
        if (x+dx < int(map.getSize().x) && !map.isGhostHouse(x+dx, y) && !map.isWall(x+dx, y))
            return {(x+dx) * float(tileSize.x) + tileSize.x/2.f, y * float(tileSize.y) + tileSize.y/2.f};
    }
    // Fallback: ritorna la posizione attuale
    return pos;
}

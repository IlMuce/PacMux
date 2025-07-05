#include "Ghost.hpp"
#include <cmath>
#include <iostream>

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
        return; // Non aggiornare Inky e Clyde
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
    
    // Aggiorna direzione se centrato E (prima volta O bloccato)
    bool shouldUpdateDirection = centered && 
                               ((m_direction.x == 0 && m_direction.y == -1) ||
                               !canMove(m_direction, map, tileSize));
                               
    if (shouldUpdateDirection) {
        int sx = int(std::round(cx));
        int sy = int(std::round(cy));
        
        // Controlla se il fantasma è uscito dalla ghost house
        if (!m_hasLeftGhostHouse && !map.isGhostHouse(sx, sy)) {
            m_hasLeftGhostHouse = true;
        }
        
        // Se siamo nella ghost house ma il flag dice che siamo usciti, correggi
        if (m_hasLeftGhostHouse && map.isGhostHouse(sx, sy)) {
            m_hasLeftGhostHouse = false;
        }
        
        // Calcola target basato sul tipo di fantasma
        sf::Vector2f target;
        if (m_mode == Mode::Scatter) {
            // Scatter mode: ogni fantasma va al proprio angolo
            switch (m_type) {
                case Type::Blinky: target = {(w-1) * float(tileSize.x), 0}; break; // Alto destra
                case Type::Pinky: target = {0, 0}; break; // Alto sinistra
                case Type::Inky: target = {(w-1) * float(tileSize.x), (h-1) * float(tileSize.y)}; break; // Basso destra
                case Type::Clyde: target = {0, (h-1) * float(tileSize.y)}; break; // Basso sinistra
            }
        } else {
            // Chase mode: calcola target specifico
            target = calculateTarget(pacmanPos, pacmanDirection, map, tileSize);
        }
        
        // Trova la direzione verso il target
        m_direction = findPath(target, map, tileSize);
    }
    
    // Movimento
    int nextX = int(std::round(cx + m_direction.x));
    int nextY = int(std::round(cy + m_direction.y));
    
    // Verifica se il movimento è valido
    bool validMove = true;
    
    // Wrap-around solo se necessario e valido
    if (nextX < 0 && int(std::round(cy)) == 10) nextX = w - 1;  // Solo per tunnel
    else if (nextX >= w && int(std::round(cy)) == 10) nextX = 0;  // Solo per tunnel
    else if (nextX < 0 || nextX >= w) {
        // Fuori dai bordi senza tunnel - movimento non valido
        validMove = false;
    }
    
    if (nextY < 0 || nextY >= h) {
        // Fuori dai bordi verticali - movimento non valido
        validMove = false;
    }
    
    if (validMove && canMove(m_direction, map, tileSize)) {
        sf::Vector2f dest{nextX * float(tileSize.x) + tileSize.x/2.f, nextY * float(tileSize.y) + tileSize.y/2.f};
        sf::Vector2f delta = dest - m_shape.getPosition();
        float step = m_speed * dt;
        
        // Movimento fluido verso la destinazione
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
        // Se non può muoversi, forza un aggiornamento della direzione
        int sx = int(std::round(cx));
        int sy = int(std::round(cy));
        
        sf::Vector2f target;
        if (m_mode == Mode::Scatter) {
            switch (m_type) {
                case Type::Blinky: target = {(w-1) * float(tileSize.x), 0}; break;
                case Type::Pinky: target = {0, 0}; break;
                case Type::Inky: target = {(w-1) * float(tileSize.x), (h-1) * float(tileSize.y)}; break;
                case Type::Clyde: target = {0, (h-1) * float(tileSize.y)}; break;
            }
        } else {
            target = calculateTarget(pacmanPos, pacmanDirection, map, tileSize);
        }
        
        m_direction = findPath(target, map, tileSize);
        
        if (m_direction == sf::Vector2f(0,0)) {
            m_direction = {0, -1}; // Default per uscire dalla ghost house
        }
    }
    
    // Aggiorna sempre la posizione di disegno
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

sf::Vector2f Ghost::findPath(const sf::Vector2f& target, const TileMap& map, const sf::Vector2u& tileSize) {
    // Implementazione base greedy - può essere sovrascritta dalle classi derivate
    sf::Vector2f pos = m_shape.getPosition();
    int startX = int(std::round((pos.x - tileSize.x/2.f) / tileSize.x));
    int startY = int(std::round((pos.y - tileSize.y/2.f) / tileSize.y));
    
    std::vector<sf::Vector2f> directions = {{0,-1}, {-1,0}, {0,1}, {1,0}};
    float minDist = 1e9f;
    sf::Vector2f bestDir = {0, -1}; // Default sicuro
    bool foundValidMove = false;
    
    // Prima prova senza reverse
    for (const auto& dir : directions) {
        // Evita reverse solo se abbiamo altre opzioni
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
    
    // Se non abbiamo trovato nessuna mossa senza reverse, accetta anche il reverse
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

bool Ghost::canMove(const sf::Vector2f& direction, const TileMap& map, const sf::Vector2u& tileSize) {
    sf::Vector2f pos = m_shape.getPosition();
    float cx = std::round((pos.x - tileSize.x/2.f) / tileSize.x);
    float cy = std::round((pos.y - tileSize.y/2.f) / tileSize.y);
    
    int nextX = int(std::round(cx + direction.x));
    int nextY = int(std::round(cy + direction.y));
    int w = map.getSize().x, h = map.getSize().y;
    
    // Wrap-around solo per tunnel orizzontali (riga 10)
    if (nextX < 0 && int(std::round(cy)) == 10) nextX = w - 1;
    else if (nextX >= w && int(std::round(cy)) == 10) nextX = 0;
    else if (nextX < 0 || nextX >= w) return false;
    
    if (nextY < 0 || nextY >= h) return false;
    
    // Aggiungi controlli di sicurezza prima di accedere alla mappa
    if (nextX < 0 || nextX >= w || nextY < 0 || nextY >= h) return false;
    
    if (map.isWall(nextX, nextY)) return false;
    if (m_hasLeftGhostHouse && map.isGhostHouse(nextX, nextY)) return false;
    
    return true;
}

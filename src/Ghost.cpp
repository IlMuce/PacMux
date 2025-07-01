#include "Ghost.hpp"
#include <cmath>
#include <queue>
#include <unordered_map>

Ghost::Ghost(const sf::Vector2f& pos, sf::Color color, float radius, Type type)
    : m_shape(radius), m_direction(0, -1), m_speed(90.f), m_type(type), m_mode(Mode::Chase), m_drawPos(pos), m_hasLeftGhostHouse(false)
{
    m_shape.setFillColor(color);
    m_shape.setOrigin({radius, radius});
    m_shape.setPosition(pos);
    m_target = pos;
}

void Ghost::update(float dt, const TileMap& map, const sf::Vector2u& tileSize, const sf::Vector2f& pacmanPos, Mode mode) {
    m_mode = mode;
    sf::Vector2f pos = m_shape.getPosition();
    float cx = std::round((pos.x - tileSize.x/2.f) / tileSize.x);
    float cy = std::round((pos.y - tileSize.y/2.f) / tileSize.y);
    sf::Vector2f center{
        cx * tileSize.x + tileSize.x/2.f,
        cy * tileSize.y + tileSize.y/2.f
    };
    bool centered = (std::abs(pos.x - center.x) < 1e-2f && std::abs(pos.y - center.y) < 1e-2f);
    int w = map.getSize().x, h = map.getSize().y;
    // Aggiorna direzione solo se centrato su una tile
    if (centered) {
        int sx = int(std::round(cx));
        int sy = int(std::round(cy));
        
        // Controlla se il fantasma è uscito dalla ghost house
        if (!m_hasLeftGhostHouse && !map.isGhostHouse(sx, sy)) {
            m_hasLeftGhostHouse = true;
        }
        
        // Target sempre Pac-Man (solo chase)
        sf::Vector2f target = pacmanPos;
        std::vector<sf::Vector2f> dirs = {{-1,0},{1,0},{0,-1},{0,1}};
        float minDist = 1e9f;
        sf::Vector2f bestDir = m_direction;
        bool foundValidDirection = false;
        
        for (auto& d : dirs) {
            if (d + m_direction == sf::Vector2f(0,0) && m_direction != sf::Vector2f(0,0)) continue; // no reverse
            int nx = sx + int(d.x);
            int ny = sy + int(d.y);
            if (nx < 0) nx = w-1; if (nx >= w) nx = 0;
            if (ny < 0) ny = h-1; if (ny >= h) ny = 0;
            if (map.isWall(nx, ny)) continue;
            
            // IMPEDISCI COMPLETAMENTE il rientro nella ghost house una volta usciti
            if (m_hasLeftGhostHouse && map.isGhostHouse(nx, ny)) continue;
            
            sf::Vector2f candPos{nx * float(tileSize.x) + tileSize.x/2.f, ny * float(tileSize.y) + tileSize.y/2.f};
            float dist = std::abs(target.x - candPos.x) + std::abs(target.y - candPos.y);
            if (dist < minDist) {
                minDist = dist;
                bestDir = d;
                foundValidDirection = true;
            }
        }
        
        // Se non trova direzioni valide (es: bloccato), mantieni la direzione corrente
        if (foundValidDirection) {
            m_direction = bestDir;
        }
        m_shape.setPosition(center);
    }
    // Movimento: solo se la prossima tile non è muro
    int nextX = int(std::round(cx + m_direction.x));
    int nextY = int(std::round(cy + m_direction.y));
    if (nextX < 0) nextX = w - 1;
    if (nextX >= w) nextX = 0;
    if (nextY < 0) nextY = h - 1;
    if (nextY >= h) nextY = 0;
    
    // Controlla se il movimento è valido
    bool canMove = !map.isWall(nextX, nextY);
    // Se il fantasma è uscito dalla ghost house, non può rientrarvi
    if (m_hasLeftGhostHouse && map.isGhostHouse(nextX, nextY)) {
        canMove = false;
    }
    
    if (m_direction != sf::Vector2f(0,0) && canMove) {
        sf::Vector2f dest{nextX * float(tileSize.x) + tileSize.x/2.f, nextY * float(tileSize.y) + tileSize.y/2.f};
        sf::Vector2f delta = dest - m_shape.getPosition();
        float step = m_speed * dt;
        if (std::hypot(delta.x, delta.y) <= step) {
            m_shape.setPosition(dest);
        } else {
            m_shape.move(m_direction * step);
            // Se superi la destinazione, riallinea
            sf::Vector2f after = m_shape.getPosition();
            if ((m_direction.x != 0 && ((m_direction.x > 0 && after.x > dest.x) || (m_direction.x < 0 && after.x < dest.x))) ||
                (m_direction.y != 0 && ((m_direction.y > 0 && after.y > dest.y) || (m_direction.y < 0 && after.y < dest.y)))) {
                m_shape.setPosition(dest);
            }
        }
        // Wrap-around effettivo
        sf::Vector2f p = m_shape.getPosition();
        if (p.x < 0) p.x = map.getSize().x * tileSize.x + p.x;
        if (p.x >= map.getSize().x * tileSize.x) p.x -= map.getSize().x * tileSize.x;
        if (p.y < 0) p.y = map.getSize().y * tileSize.y + p.y;
        if (p.y >= map.getSize().y * tileSize.y) p.y -= map.getSize().y * tileSize.y;
        m_shape.setPosition(p);
    } else {
        m_shape.setPosition(center);
    }
    // Interpolazione posizione grafica verso la posizione logica
    float interp = 0.5f;
    m_drawPos += (m_shape.getPosition() - m_drawPos) * interp;
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
    m_hasLeftGhostHouse = false; // Reset del flag quando il fantasma viene riposizionato
}

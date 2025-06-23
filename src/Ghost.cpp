#include "Ghost.hpp"
#include <cmath>
#include <queue>
#include <unordered_map>

Ghost::Ghost(const sf::Vector2f& pos, sf::Color color, float radius, Type type)
    : m_shape(radius), m_direction(0, -1), m_speed(114.f), m_type(type), m_mode(Mode::Chase), m_drawPos(pos)
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
        // Target in base alla modalità
        sf::Vector2f target;
        if (m_mode == Mode::Chase) {
            target = pacmanPos;
        } else {
            // Scatter: angolo in alto a destra
            target = { (w-1) * tileSize.x + tileSize.x/2.f, tileSize.y/2.f };
        }
        // Greedy Manhattan verso il target
        std::vector<sf::Vector2f> dirs = {{-1,0},{1,0},{0,-1},{0,1}};
        float minDist = 1e9f;
        sf::Vector2f bestDir = m_direction;
        for (auto& d : dirs) {
            if (d + m_direction == sf::Vector2f(0,0)) continue; // no reverse
            int nx = int(cx) + int(d.x);
            int ny = int(cy) + int(d.y);
            if (nx < 0) nx = w-1; if (nx >= w) nx = 0;
            if (ny < 0) ny = h-1; if (ny >= h) ny = 0;
            if (map.isWall(nx, ny)) continue;
            sf::Vector2f candPos{nx * float(tileSize.x) + tileSize.x/2.f, ny * float(tileSize.y) + tileSize.y/2.f};
            float dist = std::abs(target.x - candPos.x) + std::abs(target.y - candPos.y);
            if (dist < minDist) {
                minDist = dist;
                bestDir = d;
            }
        }
        m_direction = bestDir;
        m_shape.setPosition(center);
    }
    // Movimento: solo se la prossima tile non è muro
    int nextX = int(cx + m_direction.x);
    int nextY = int(cy + m_direction.y);
    if (nextX < 0) nextX = w - 1;
    if (nextX >= w) nextX = 0;
    if (nextY < 0) nextY = h - 1;
    if (nextY >= h) nextY = 0;
    if (m_direction != sf::Vector2f(0,0) && !map.isWall(nextX, nextY)) {
        sf::Vector2f dest{nextX * float(tileSize.x) + tileSize.x/2.f, nextY * float(tileSize.y) + tileSize.y/2.f};
        sf::Vector2f delta = dest - m_shape.getPosition();
        float step = m_speed * dt;
        if (std::hypot(delta.x, delta.y) <= step) {
            m_shape.setPosition(dest);
        } else {
            m_shape.move(m_direction * step);
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
    // Centra su tile e resetta la direzione
    m_direction = {0, -1};
}

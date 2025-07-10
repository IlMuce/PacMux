#include "Blinky.hpp"
#include <cmath>
#include <iostream>

// Blinky: il fantasma rosso, insegue direttamente Pac-Man
Blinky::Blinky(const sf::Vector2f& pos) : Ghost(pos, sf::Color::Red, 12.0f, Type::Blinky) {
    // Carica la texture solo una volta
    m_texture = std::make_unique<sf::Texture>();
    if (m_texture->loadFromFile("assets/pacman.png")) {
        m_sprite = std::make_unique<sf::Sprite>(*m_texture);
        m_sprite->setTextureRect(BLINKY_FRAMES[2][0]); // frame iniziale: destra, anim 0
        m_sprite->setOrigin(sf::Vector2f{8.f, 8.f}); // centro per 16x16
        float scale = 12.f / 8.f; // raggio 12px, sprite 16px
        m_sprite->setScale(sf::Vector2f{scale, scale});
        m_hasTexture = true;
    } else {
        m_hasTexture = false;
    }
    m_animTime = 0.f;
    m_animFrame = 0;
}

// Target = posizione attuale di Pac-Man
sf::Vector2f Blinky::calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f&, 
                                   const TileMap&, const sf::Vector2u&) {
    return pacmanPos;
}

void Blinky::update(float dt, const TileMap& map, const sf::Vector2u& tileSize,
                  const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode, bool gameStarted) {
    static float debugTimer = 0.f;
    m_mode = mode;
    sf::Vector2f pos = m_shape.getPosition();
    debugTimer += dt;
    if (debugTimer >= 1.0f) {
        std::string modeStr = (m_mode == Mode::Chase) ? "Chase" : (m_mode == Mode::Scatter) ? "Scatter" : "Other";
        std::cout << "[BLINKY] Pos: (" << pos.x << "," << pos.y << ") Dir: (" << m_direction.x << "," << m_direction.y << ") Pacman: (" << pacmanPos.x << "," << pacmanPos.y << ") Mode: " << modeStr << std::endl;
        debugTimer = 0.f;
    }
    // --- Animazione sprite ---
    if (m_hasTexture && m_sprite) {
        m_sprite->setPosition(m_shape.getPosition());
        m_animTime += dt;
        if (m_direction != sf::Vector2f{0,0}) {
            if (m_animTime >= GHOST_ANIMATION_INTERVAL) {
                m_animFrame = 1 - m_animFrame; // alterna 0/1
                m_animTime = 0.f;
            }
        } else {
            m_animFrame = 0;
        }
        // Direzione logica: 0=sinistra, 1=su, 2=destra, 3=giù
        int dir = 2; // default destra
        if (m_direction.x < 0) dir = 0;
        else if (m_direction.x > 0) dir = 2;
        else if (m_direction.y < 0) dir = 1;
        else if (m_direction.y > 0) dir = 3;
        m_sprite->setTextureRect(BLINKY_FRAMES[dir][m_animFrame]);
    }
    Ghost::update(dt, map, tileSize, pacmanPos, pacmanDirection, mode, gameStarted);
}

// Disegna Blinky (override draw se vuoi sprite)
void Blinky::draw(sf::RenderTarget& target, sf::RenderStates states) const {
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

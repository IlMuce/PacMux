#include "Blinky.hpp"
#include <cmath>
#include <iostream>

// Blinky: il fantasma rosso, insegue direttamente Pac-Man
Blinky::Blinky(const sf::Vector2f& pos) : Ghost(pos, sf::Color::Red, 12.0f, Type::Blinky) {}

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
    Ghost::update(dt, map, tileSize, pacmanPos, pacmanDirection, mode, gameStarted);
}

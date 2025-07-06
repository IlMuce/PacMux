#include "Clyde.hpp"

Clyde::Clyde(const sf::Vector2f& pos) : Ghost(pos, sf::Color(255, 165, 0), 12.0f, Type::Clyde) {
}

sf::Vector2f Clyde::calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                                  const TileMap& map, const sf::Vector2u& tileSize) {
    // Implementazione temporanea - punta direttamente a Pac-Man
    return pacmanPos;
}

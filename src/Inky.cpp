#include "Inky.hpp"

Inky::Inky(const sf::Vector2f& pos) : Ghost(pos, sf::Color::Cyan, 12.0f, Type::Inky) {
}

sf::Vector2f Inky::calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                                 const TileMap& map, const sf::Vector2u& tileSize) {
    // Implementazione temporanea - punta direttamente a Pac-Man
    return pacmanPos;
}

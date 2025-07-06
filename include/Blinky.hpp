#pragma once

#include "Ghost.hpp"

class Blinky : public Ghost {
public:
    Blinky(const sf::Vector2f& pos);
    
protected:
    sf::Vector2f calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                               const TileMap& map, const sf::Vector2u& tileSize) override;
    
    sf::Vector2f findPath(const sf::Vector2f& target, const TileMap& map, const sf::Vector2u& tileSize) override;

private:
    sf::Vector2f findPathToPacman(const sf::Vector2f& pacmanPos, const TileMap& map, const sf::Vector2u& tileSize);
    sf::Vector2f greedyFallback(const sf::Vector2f& pacmanPos, const TileMap& map, const sf::Vector2u& tileSize, 
                               int startX, int startY, int w, int h);
};

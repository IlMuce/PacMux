#pragma once

#include "Ghost.hpp"

class Blinky : public Ghost {
public:
    Blinky(const sf::Vector2f& pos);
    void update(float dt, const TileMap& map, const sf::Vector2u& tileSize,
                const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode) override;
    
protected:
    sf::Vector2f calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                               const TileMap& map, const sf::Vector2u& tileSize) override;

private:
    sf::Vector2f findPathToPacman(const sf::Vector2f& pacmanPos, const TileMap& map, const sf::Vector2u& tileSize);
    sf::Vector2f greedyFallback(const sf::Vector2f& pacmanPos, const TileMap& map, const sf::Vector2u& tileSize, 
                               int startX, int startY, int w, int h);
};

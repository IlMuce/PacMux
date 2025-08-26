#pragma once

#include "Ghost.hpp"

class Pinky : public Ghost {
public:
    Pinky(const sf::Vector2f& pos);
    
    void update(float dt, const TileMap& map, const sf::Vector2u& tileSize,
                const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode, bool gameStarted = true, bool released = true);
    
protected:
    sf::Vector2f calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                               const TileMap& map, const sf::Vector2u& tileSize) override;
    // Pinky non ridefinisce findPath: usa quello della classe base Ghost
};

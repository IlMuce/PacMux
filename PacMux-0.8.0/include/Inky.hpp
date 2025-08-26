#pragma once

#include "Ghost.hpp"

class Inky : public Ghost {
public:
    Inky(const sf::Vector2f& pos);
    // Overload: update with Blinky's position for correct targeting and release logic
    void update(float dt, const TileMap& map, const sf::Vector2u& tileSize,
                const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode,
                const sf::Vector2f& blinkyPos, bool gameStarted = true, bool released = true);
    
protected:
    // Override richiesto dalla base Ghost
    sf::Vector2f calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                                 const TileMap& map, const sf::Vector2u& tileSize) override;
    // Overload: Inky needs Blinky's position for its targeting logic
    sf::Vector2f calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                                 const TileMap& map, const sf::Vector2u& tileSize, const sf::Vector2f& blinkyPos);
};

#pragma once

#include "Ghost.hpp"

class Clyde : public Ghost {
public:
    Clyde(const sf::Vector2f& pos);
    void update(float dt, const TileMap& map, const sf::Vector2u& tileSize,
                const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode) override;
protected:
    sf::Vector2f calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                                 const TileMap& map, const sf::Vector2u& tileSize) override;
};

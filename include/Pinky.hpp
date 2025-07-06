#pragma once

#include "Ghost.hpp"

class Pinky : public Ghost {
public:
    Pinky(const sf::Vector2f& pos);
    
protected:
    sf::Vector2f calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                               const TileMap& map, const sf::Vector2u& tileSize) override;
    
    sf::Vector2f findPath(const sf::Vector2f& target, const TileMap& map, const sf::Vector2u& tileSize) override;

private:
    // A* pathfinding per Pinky
    struct Node {
        int x, y;
        float gCost, hCost, fCost;
        Node* parent;
        
        Node(int x, int y) : x(x), y(y), gCost(0), hCost(0), fCost(0), parent(nullptr) {}
    };
    
    sf::Vector2f aStar(const sf::Vector2f& start, const sf::Vector2f& target, 
                      const TileMap& map, const sf::Vector2u& tileSize);
    float heuristic(int x1, int y1, int x2, int y2);
};

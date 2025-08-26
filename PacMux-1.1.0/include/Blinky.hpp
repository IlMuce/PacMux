#pragma once

#include "Ghost.hpp"

class Blinky : public Ghost {
public:
    Blinky(const sf::Vector2f& pos);
    void update(float dt, const TileMap& map, const sf::Vector2u& tileSize,
                const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode, bool gameStarted = true) override;
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    
protected:
    sf::Vector2f calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                               const TileMap& map, const sf::Vector2u& tileSize) override;

private:
    // --- Sprite e animazione Blinky ---
    std::unique_ptr<sf::Texture> m_texture;
    std::unique_ptr<sf::Sprite> m_sprite;
    bool m_hasTexture = false;
    float m_animTime = 0.f;
    int m_animFrame = 0;

    sf::Vector2f findPathToPacman(const sf::Vector2f& pacmanPos, const TileMap& map, const sf::Vector2u& tileSize);
    sf::Vector2f greedyFallback(const sf::Vector2f& pacmanPos, const TileMap& map, const sf::Vector2u& tileSize, 
                               int startX, int startY, int w, int h);
};

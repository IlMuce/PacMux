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

private:
    // --- Sprite e animazione Pinky ---
    std::unique_ptr<sf::Texture> m_texture;
    std::unique_ptr<sf::Sprite> m_sprite;
    bool m_hasTexture = false;
    float m_animTime = 0.f;
    int m_animFrame = 0;
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
};

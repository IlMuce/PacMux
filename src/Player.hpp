#pragma once
#include <SFML/Graphics.hpp>
#include "TileMap.hpp"

class Player : public sf::Drawable, public sf::Transformable {
public:
    Player(float speed, const sf::Vector2f& startPos, const sf::Vector2u& tileSize);
    // ora riceve TileMap per collisioni
    void update(float dt, const TileMap& map, const sf::Vector2u& tileSize);
    sf::Vector2f getPosition() const { return m_shape.getPosition(); }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    sf::CircleShape m_shape;
    float           m_speed;
    sf::Vector2f    m_direction;
};

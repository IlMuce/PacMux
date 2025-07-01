#pragma once
#include <SFML/Graphics.hpp>
#include "TileMap.hpp"
#include <vector>
#include <memory>

class Ghost : public sf::Drawable, public sf::Transformable {
public:
    enum class Type { Blinky, Pinky, Inky, Clyde };
    enum class Mode { Scatter, Chase };

    Ghost(const sf::Vector2f& pos, sf::Color color = sf::Color::Red, float radius = 12.f, Type type = Type::Blinky);

    void update(float dt, const TileMap& map, const sf::Vector2u& tileSize, const sf::Vector2f& pacmanPos, Mode mode = Mode::Chase);
    sf::Vector2f getPosition() const { return m_shape.getPosition(); }
    void setTarget(const sf::Vector2f& target) { m_target = target; }
    void setMode(Mode mode) { m_mode = mode; }
    void setPosition(const sf::Vector2f& pos);
    void setDirection(const sf::Vector2f& dir) { m_direction = dir; }
    sf::Vector2f getDirection() const { return m_direction; }
    Type getType() const { return m_type; }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    sf::CircleShape m_shape;
    sf::Vector2f m_direction; // direzione attuale
    sf::Vector2f m_target;    // bersaglio corrente (tile center)
    float m_speed;
    Type m_type;
    Mode m_mode;
    bool m_hasLeftGhostHouse; // Traccia se il fantasma è uscito dalla ghost house
    sf::Vector2f m_drawPos;   // posizione grafica per effetto fluido
};

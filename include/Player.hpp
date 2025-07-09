#pragma once
#include <SFML/Graphics.hpp>
#include "TileMap.hpp"

class Player : public sf::Drawable, public sf::Transformable {
public:
    Player(float speed, const sf::Vector2f& startPos, const sf::Vector2u& tileSize);

    // ora prende solo map per collisioni
    void update(float dt, const TileMap& map, const sf::Vector2u& tileSize);

    sf::Vector2f getPosition() const { return m_shape.getPosition(); }
    sf::Vector2f getDirection() const { return m_direction; }

    // Imposta la posizione di Pac-Man
    void setPosition(const sf::Vector2f& position) { 
        m_shape.setPosition(position); 
        m_logicalPosition = position; 
    }

    // Restituisce la posizione logica di Pac-Man
    sf::Vector2f getLogicalPosition() const { return m_logicalPosition; }

    // Aggiorna la posizione logica di Pac-Man
    void setLogicalPosition(const sf::Vector2f& position) { m_logicalPosition = position; }

    // Gestione vite
    int getLives() const { return m_lives; }
    void setLives(int lives) { m_lives = lives; }
    void loseLife() { if (m_lives > 0) m_lives--; }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::CircleShape   m_shape;
    float             m_speed;
    sf::Vector2f      m_direction;      // direzione corrente (-1,0),(1,0),(0,-1),(0,1)
    sf::Vector2f      m_nextDirection;  // direzione desiderata dal giocatore
    sf::Vector2u      m_tileSize;
    sf::Vector2f      m_logicalPosition; // Posizione logica di Pac-Man
    int               m_lives;           // Numero di vite del giocatore
};

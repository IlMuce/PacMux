#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
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
        if (m_hasTexture && m_sprite) {
            m_sprite->setPosition(position);
        }
    }

    // Restituisce la posizione logica di Pac-Man
    sf::Vector2f getLogicalPosition() const { return m_logicalPosition; }

    // Aggiorna la posizione logica di Pac-Man
    void setLogicalPosition(const sf::Vector2f& position) { m_logicalPosition = position; }

    // Gestione vite
    int getLives() const { return m_lives; }
    void setLives(int lives) { m_lives = lives; }
    void loseLife() { if (m_lives > 0) m_lives--; }
    
    // Ferma il movimento di Pac-Man (per situazioni speciali come vita extra)
    void stopMovement() { 
        m_direction = {0.f, 0.f}; 
        m_nextDirection = {0.f, 0.f}; 
    }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    sf::CircleShape   m_shape;
    float             m_speed;
    sf::Vector2f      m_direction;      // direzione corrente (-1,0),(1,0),(0,-1),(0,1)
    sf::Vector2f      m_nextDirection;  // direzione desiderata dal giocatore
    sf::Vector2u      m_tileSize;
    sf::Vector2f      m_logicalPosition; // Posizione logica di Pac-Man
    int               m_lives;           // Numero di vite del giocatore
    
    // Texture e sprite per Pac-Man
    std::unique_ptr<sf::Texture> m_texture;
    std::unique_ptr<sf::Sprite>  m_sprite;
    bool                         m_hasTexture;
    // --- Animazione Pac-Man ---
    float m_animTime = 0.f;
    int m_animFrame = 0;
};

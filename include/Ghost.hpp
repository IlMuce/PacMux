#pragma once

#include <SFML/Graphics.hpp>
#include "TileMap.hpp"

class Ghost : public sf::Drawable, public sf::Transformable {
public:
    enum class Type {
        Blinky, // Rosso
        Pinky,  // Rosa
        Inky,   // Azzurro
        Clyde   // Arancione
    };

    enum class Mode {
        Chase,
        Scatter,
        Frightened
    };

    Ghost(const sf::Vector2f& pos, sf::Color color, float radius, Type type);
    virtual ~Ghost() = default;

    virtual void update(float dt, const TileMap& map, const sf::Vector2u& tileSize, 
                       const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, Mode mode, bool gameStarted = true);
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    
    void setPosition(const sf::Vector2f& pos);
    sf::Vector2f getPosition() const { return m_shape.getPosition(); }
    
    Type getType() const { return m_type; }
    bool hasLeftGhostHouse() const { return m_hasLeftGhostHouse; }

    // Set frightened mode for a duration (in seconds)
    void setFrightened(float duration);
    bool isFrightened() const { return m_isFrightened; }

    // Eaten/respawn state
    void setEaten(bool eaten);
    bool isEaten() const { return m_eaten; }
    bool isReturningToHouse() const { return m_isReturningToHouse; }

    void setSpeed(float speed) { m_speed = speed; }
    void setReleaseDelay(float delay) { m_releaseDelay = delay; m_canLeaveHouse = false; }

    // --- RELEASE STATE ---
    void setReleased(bool released) { 
        m_released = released; 
        m_canLeaveHouse = released; // Simply allow/disallow leaving based on release state
    }
    bool isReleased() const { return m_released; }
    // True se il fantasma ha lasciato la ghost house (già presente: hasLeftGhostHouse)
    bool hasLeftHouse() const { return m_hasLeftGhostHouse; }

    static const char* getTypeName(Type type) {
        switch(type) {
            case Type::Blinky: return "Blinky";
            case Type::Pinky:  return "Pinky";
            case Type::Inky:   return "Inky";
            case Type::Clyde:  return "Clyde";
            default: return "Unknown";
        }
    }

    // Permette di impostare la direzione manualmente (es. workaround teleport)
    void setDirection(const sf::Vector2f& dir) { m_direction = dir; }
    // Restituisce la direzione corrente del fantasma
    sf::Vector2f getDirection() const { return m_direction; }

    // Forza il flag m_hasLeftGhostHouse (workaround per log spam ai bordi)
    void forceHasLeftGhostHouse(bool val) { m_hasLeftGhostHouse = val; }

protected:
    virtual sf::Vector2f calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                                        const TileMap& map, const sf::Vector2u& tileSize) = 0;
    
    virtual sf::Vector2f findPath(const sf::Vector2f& target, const TileMap& map, const sf::Vector2u& tileSize);
    bool canMove(const sf::Vector2f& direction, const TileMap& map, const sf::Vector2u& tileSize);
    
    // Trova la porta di uscita della ghost house (prima cella non ghost house sopra la posizione attuale)
    sf::Vector2f getGhostHouseExit(const TileMap& map, const sf::Vector2u& tileSize) const;

    sf::CircleShape m_shape;
    sf::Vector2f m_direction;
    sf::Vector2f m_target;
    sf::Vector2f m_drawPos;
    float m_speed;
    Type m_type;
    Mode m_mode;
    bool m_hasLeftGhostHouse;

    // Frightened mode state
    bool m_isFrightened = false;
    float m_frightenedTimer = 0.f;
    float m_frightenedDuration = 0.f;

    // Eaten/respawn state
    bool m_eaten = false;
    bool m_isReturningToHouse = false;
    float m_respawnTimer = 0.f;
    float m_respawnDuration = 3.f; // seconds in ghost house after being eaten

    // Ghost release state
    float m_releaseDelay = 0.f; // quanto deve aspettare prima di poter uscire (for reference only)
    bool m_canLeaveHouse = false;

    bool m_released = false;
};

// --- ANIMAZIONE SPRITE FANTASMI ---
// Array di frame per Blinky (e altri se vuoi)
extern const sf::IntRect BLINKY_FRAMES[4][2];
extern const sf::IntRect INKY_FRAMES[4][2];
extern const sf::IntRect CLYDE_FRAMES[4][2];
extern const sf::IntRect PINKY_FRAMES[4][2];
extern const sf::IntRect FRIGHTENED_FRAMES[2];
extern const sf::IntRect EYES_FRAMES[4];
extern const float GHOST_ANIMATION_INTERVAL;
// Puoi aggiungere qui anche PINKY_FRAMES se vuoi usarli altrove

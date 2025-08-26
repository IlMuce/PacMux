#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

// Frutto collezionabile semplice, disegnato dalla sprite sheet condivisa pacman.png.
// Le celle della mappa marcate con 'F' generano i frutti. Supportiamo almeno 2 tipi.
class Fruit : public sf::Drawable, public sf::Transformable {
public:
    enum class Type { Cherry = 0, Strawberry = 1, Mushroom = 2, Egg = 3 };

    // Costruttore: posizione (centro in coordinate mondo) e tipo di frutto
    Fruit(const sf::Vector2f& pos, Type type);

    // Ritorna true se Pac-Man alla posizione indicata raccoglie il frutto (point-in-bounds)
    bool eaten(const sf::Vector2f& playerPos) const;

    // Aggiorna il timer di vita del frutto; dopo 10s scompare
    void update(float dt);
    bool expired() const;

    // Punteggio assegnato quando viene raccolto
    int getScore() const;

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    // Texture e Sprite per il rendering (come Pac-Man e i Ghost)
    std::unique_ptr<sf::Texture> m_texture;
    std::unique_ptr<sf::Sprite> m_sprite;
    bool m_hasTexture = false;
    Type m_type;

    // Timer di vita del frutto (in secondi)
    float m_timeAlive = 0.f;
    static constexpr float DESPAWN_AFTER = 10.0f; // secondi

    // Forma di fallback nel caso la texture non sia disponibile
    sf::CircleShape m_fallbackShape;
};

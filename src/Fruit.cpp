#include "Fruit.hpp"
#include <filesystem>

using namespace std;

// Coordinate su pacman.png per due frutti (adatta se la tua sprite sheet differisce):
// Riutilizziamo regioni della stessa sprite sheet usata per Pac-Man/Fantasmi.
// Forniamo due rettangoli esempio; potrai rifinirli in seguito.
// NOTA: Le coordinate sono indicative; se il tuo pacman.png differisce, aggiorna qui.
static const sf::IntRect FRUIT_RECTS[4] = {
    // Mela
    sf::IntRect(sf::Vector2i{151, 549}, sf::Vector2i{16, 16}),
    // Banana
    sf::IntRect(sf::Vector2i{168, 549}, sf::Vector2i{16, 16}),
    // Fungo
    sf::IntRect(sf::Vector2i{338, 549}, sf::Vector2i{16, 16}),
    // Uovo
    sf::IntRect(sf::Vector2i{219, 549}, sf::Vector2i{16, 16})
};

// Punteggi per frutto, valori classici (personalizzabili)
// Punteggi suggeriti (bilanciati): Cherry 100, Strawberry 300, Mushroom 500, Egg 700
static const int FRUIT_SCORES[4] = {100, 300, 500, 700};

Fruit::Fruit(const sf::Vector2f& pos, Type type)
    : m_type(type), m_fallbackShape(8.f) // fallback radius ~8px
{
    // Carica texture come fanno Pac-Man e Ghost
    m_texture = std::make_unique<sf::Texture>();
    if (m_texture->loadFromFile("assets/pacman.png")) {
        m_sprite = std::make_unique<sf::Sprite>(*m_texture);
        auto idx = static_cast<int>(type);
        m_sprite->setTextureRect(FRUIT_RECTS[idx]);
        // Center origin based on rect size
        m_sprite->setOrigin(sf::Vector2f(FRUIT_RECTS[idx].size.x / 2.f, FRUIT_RECTS[idx].size.y / 2.f));
        // Scale sprite to roughly 0.75 of a 32x32 tile (similar to Pac-Man sizing)
        float scale = 32.f / static_cast<float>(FRUIT_RECTS[idx].size.x) * 0.75f;
        m_sprite->setScale(sf::Vector2f(scale, scale));
        m_sprite->setPosition(pos);
        m_hasTexture = true;
    } else {
    // Fallback: disegna un cerchio colorato se la texture non è disponibile
        m_fallbackShape.setOrigin(sf::Vector2f(m_fallbackShape.getRadius(), m_fallbackShape.getRadius()));
        m_fallbackShape.setPosition(pos);
        m_fallbackShape.setFillColor(sf::Color(255, 64, 64));
        m_hasTexture = false;
    }
}

bool Fruit::eaten(const sf::Vector2f& playerPos) const {
    if (m_hasTexture) {
    return m_sprite && m_sprite->getGlobalBounds().contains(playerPos);
    }
    return m_fallbackShape.getGlobalBounds().contains(playerPos);
}

int Fruit::getScore() const {
    return FRUIT_SCORES[static_cast<int>(m_type)];
}

void Fruit::update(float dt) {
    m_timeAlive += dt;
}

bool Fruit::expired() const {
    return m_timeAlive >= DESPAWN_AFTER;
}

void Fruit::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    if (m_hasTexture) {
    if (m_sprite) target.draw(*m_sprite, states);
    } else {
        target.draw(m_fallbackShape, states);
    }
}

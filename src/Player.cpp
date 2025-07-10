// src/Player.cpp
#include "Player.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <iostream>

// Costruttore: inizializza Pac-Man con velocità, posizione e dimensione cella
Player::Player(float speed, const sf::Vector2f& startPos, const sf::Vector2u& tileSize)
    : m_shape(tileSize.x * 0.4f)
    , m_speed(speed)
    , m_direction{0.f, 0.f}
    , m_nextDirection{0.f, 0.f}
    , m_tileSize(tileSize)
    , m_lives(3)  // Inizializza con 3 vite
    , m_hasTexture(false)
{
    m_shape.setFillColor(sf::Color::Yellow);
    m_shape.setOrigin(sf::Vector2f(m_shape.getRadius(), m_shape.getRadius()));
    m_shape.setPosition(startPos);
    m_logicalPosition = startPos; // Inizializza la posizione logica
    
    // Prova a caricare la texture di Pac-Man
    m_texture = std::make_unique<sf::Texture>();
    if (m_texture->loadFromFile("assets/pacman.png")) {
        std::cout << "[DEBUG] Texture Pac-Man caricata con successo!" << std::endl;
        
        // Crea lo sprite
        m_sprite = std::make_unique<sf::Sprite>(*m_texture);
        
        // Imposta il frame per Pac-Man bocca chiusa 
        // Test: spostiamo 5px a destra e 5px in basso
        // x=5, y=497, dimensioni=32x32
        m_sprite->setTextureRect(sf::IntRect(sf::Vector2i{1, 498}, sf::Vector2i{32, 32}));
        
        std::cout << "[DEBUG] Frame impostato a (5, 497, 32x32)" << std::endl;
        
        // Imposta origine al centro
        m_sprite->setOrigin(sf::Vector2f{16.f, 16.f});
        
        // Scala lo sprite per adattarlo alla dimensione della cella (ridotto all'80%)
        float scale = static_cast<float>(tileSize.x) / 32.f * 0.75f;
        m_sprite->setScale(sf::Vector2f{scale, scale});
        
        // Posiziona lo sprite
        m_sprite->setPosition(startPos);
        
        m_hasTexture = true;
        std::cout << "[DEBUG] Sprite Pac-Man configurato (scala: " << scale << ")" << std::endl;
    } else {
        std::cout << "[WARNING] Impossibile caricare assets/pacman.png, uso cerchio giallo" << std::endl;
    }
}

// Aggiorna la posizione e la direzione di Pac-Man in base all'input e alle collisioni
void Player::update(float dt, const TileMap& map, const sf::Vector2u& tileSize) {
    using Key = sf::Keyboard::Key;

    // Leggi input direzionale
    if      (sf::Keyboard::isKeyPressed(Key::Left))  m_nextDirection = {-1.f,  0.f};
    else if (sf::Keyboard::isKeyPressed(Key::Right)) m_nextDirection = { 1.f,  0.f};
    else if (sf::Keyboard::isKeyPressed(Key::Up))    m_nextDirection = { 0.f, -1.f};
    else if (sf::Keyboard::isKeyPressed(Key::Down))  m_nextDirection = { 0.f,  1.f};

    sf::Vector2f pos = m_shape.getPosition();
    float r = m_shape.getRadius();

    // Calcola cella corrente e centro della cella
    unsigned cellX = unsigned(pos.x / tileSize.x);
    unsigned cellY = unsigned(pos.y / tileSize.y);
    sf::Vector2f center{
        cellX * float(tileSize.x) + tileSize.x/2.f,
        cellY * float(tileSize.y) + tileSize.y/2.f
    };

    // Tolleranza per l'allineamento al centro cella
    float tolX = tileSize.x / 4.f;
    float tolY = tileSize.y / 4.f;

    // Se retromarcia, accetta subito la nuova direzione
    if (m_nextDirection == -m_direction && m_direction != sf::Vector2f{0,0}) {
        m_direction = m_nextDirection;
    }
    // Altrimenti, prova a svoltare se sei allineato o fermo
    else if (m_nextDirection != m_direction) {
        bool canTurn = false;
        // Svolta verticale da orizzontale
        if (m_direction.x != 0.f && m_nextDirection.y != 0.f
            && std::abs(pos.x - center.x) < tolX)
            canTurn = true;
        // Svolta orizzontale da verticale
        if (m_direction.y != 0.f && m_nextDirection.x != 0.f
            && std::abs(pos.y - center.y) < tolY)
            canTurn = true;
        // Se fermo, svolta sempre
        if (m_direction == sf::Vector2f{0,0})
            canTurn = true;
        // Se esattamente al centro
        if (std::abs(pos.x - center.x) < 1.f && std::abs(pos.y - center.y) < 1.f)
            canTurn = true;

        if (canTurn) {
            int nx = int(cellX) + int(m_nextDirection.x);
            int ny = int(cellY) + int(m_nextDirection.y);
            bool inBounds = nx>=0 && ny>=0
                         && nx<int(map.getSize().x) && ny<int(map.getSize().y);
            // Pac-Man non può entrare nella ghost house
            if (inBounds && !map.isWall(nx,ny) && !map.isGhostHouse(nx,ny)) {
                m_direction = m_nextDirection;
                m_shape.setPosition(center); // riallinea
                pos = center;
            }
        }
    }

    // Calcola nuova posizione
    sf::Vector2f delta = m_direction * m_speed * dt;
    sf::Vector2f newPos = pos + delta;
    int tx = int(newPos.x / tileSize.x);
    int ty = int(newPos.y / tileSize.y);

    // --- Teletrasporto laterale (wrap-around) ---
    // Se Pac-Man esce dal bordo sinistro o destro, riappare dall'altro lato
    int mapWidth = int(map.getSize().x);
    int mapHeight = int(map.getSize().y);
    if (newPos.x < 0) {
        newPos.x = mapWidth * tileSize.x - tileSize.x/2.f;
    } else if (newPos.x > mapWidth * tileSize.x) {
        newPos.x = tileSize.x/2.f;
    }
    // --- Teletrasporto verticale (wrap-around) ---
    // Se Pac-Man esce dal bordo alto o basso, riappare dall'altro lato
    if (newPos.y < 0) {
        newPos.y = mapHeight * tileSize.y - tileSize.y/2.f;
    } else if (newPos.y > mapHeight * tileSize.y) {
        newPos.y = tileSize.y/2.f;
    }
    tx = int(newPos.x / tileSize.x); // ricalcola la cella dopo il wrap
    ty = int(newPos.y / tileSize.y);

    // Se la cella è libera E NON è ghost house, muovi Pac-Man
    if (tx>=0 && ty>=0 && tx<int(map.getSize().x) && ty<int(map.getSize().y)
        && !map.isWall(tx, ty) && !map.isGhostHouse(tx, ty))
    {
        m_shape.setPosition(newPos);
    }
    // Altrimenti, blocca il movimento e riallinea
    else {
        if (m_direction.x != 0.f) {
            float limitX = center.x + (m_direction.x>0
                ? (tileSize.x/2.f - r)
                : -(tileSize.x/2.f - r));
            m_shape.setPosition(sf::Vector2f(limitX, pos.y));
        } else if (m_direction.y != 0.f) {
            float limitY = center.y + (m_direction.y>0
                ? (tileSize.y/2.f - r)
                : -(tileSize.y/2.f - r));
            m_shape.setPosition(sf::Vector2f(pos.x, limitY));
        }
        m_direction = {0.f,0.f};
    }

    // Aggiorna la posizione logica con la posizione corrente
    m_logicalPosition = m_shape.getPosition();
    
    // Sincronizza lo sprite con la posizione del cerchio
    if (m_hasTexture && m_sprite) {
        m_sprite->setPosition(m_shape.getPosition());
    }
}

// Disegna Pac-Man sulla finestra
void Player::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    
    if (m_hasTexture && m_sprite) {
        // Disegna lo sprite se la texture è disponibile
        target.draw(*m_sprite, states);
    } else {
        // Fallback: disegna il cerchio giallo
        target.draw(m_shape, states);
    }
}

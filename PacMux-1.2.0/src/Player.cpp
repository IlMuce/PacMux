// src/Player.cpp
#include "Player.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <iostream>

// Imposta la direzione di Pac-Man
void Player::setDirection(const sf::Vector2f& dir) {
    m_direction = dir;
}

// --- ANIMAZIONE SPRITE PAC-MAN ---
// Ordine righe: sinistra, su, destra, giù (y=499, 531, 563, 595)
// Colonne: bocca chiusa (x=1), semi-aperta (x=33), aperta (x=65)
static const sf::IntRect PACMAN_FRAMES[4][3] = {
    { sf::IntRect(sf::Vector2i{1,498}, sf::Vector2i{32,32}), sf::IntRect(sf::Vector2i{34,498}, sf::Vector2i{32,32}), sf::IntRect(sf::Vector2i{67,498}, sf::Vector2i{32,32}) },   // SINISTRA
    { sf::IntRect(sf::Vector2i{1,531}, sf::Vector2i{32,32}), sf::IntRect(sf::Vector2i{34,531}, sf::Vector2i{32,32}), sf::IntRect(sf::Vector2i{67,531}, sf::Vector2i{32,32}) },   // SU
    { sf::IntRect(sf::Vector2i{1,564}, sf::Vector2i{32,32}), sf::IntRect(sf::Vector2i{34,564}, sf::Vector2i{32,32}), sf::IntRect(sf::Vector2i{67,564}, sf::Vector2i{32,32}) },   // DESTRA
    { sf::IntRect(sf::Vector2i{1,597}, sf::Vector2i{32,32}), sf::IntRect(sf::Vector2i{34,597}, sf::Vector2i{32,32}), sf::IntRect(sf::Vector2i{67,597}, sf::Vector2i{32,32}) }    // GIÙ
};
static constexpr float ANIMATION_INTERVAL = 0.08f; // secondi tra un frame e l'altro

// --- ANIMAZIONE MORTE PAC-MAN ---
static const sf::IntRect PACMAN_DEATH_FRAMES[11] = {
    sf::IntRect(sf::Vector2i{151,498}, sf::Vector2i{16,16}),
    sf::IntRect(sf::Vector2i{168,498}, sf::Vector2i{16,16}),
    sf::IntRect(sf::Vector2i{185,498}, sf::Vector2i{16,16}),
    sf::IntRect(sf::Vector2i{202,498}, sf::Vector2i{16,16}),
    sf::IntRect(sf::Vector2i{219,498}, sf::Vector2i{16,16}),
    sf::IntRect(sf::Vector2i{236,498}, sf::Vector2i{16,16}),
    sf::IntRect(sf::Vector2i{253,498}, sf::Vector2i{16,16}),
    sf::IntRect(sf::Vector2i{270,498}, sf::Vector2i{16,16}),
    sf::IntRect(sf::Vector2i{287,498}, sf::Vector2i{16,16}),
    sf::IntRect(sf::Vector2i{304,498}, sf::Vector2i{16,16}),
    sf::IntRect(sf::Vector2i{321,498}, sf::Vector2i{16,16})
};
static constexpr float DEATH_ANIMATION_INTERVAL = 0.09f; // secondi tra i frame di morte

// Enum direzione logica Pac-Man
enum PacmanDir { LEFT=0, UP=1, RIGHT=2, DOWN=3, NONE=4 };

// Costruttore: inizializza Pac-Man con velocità, posizione e dimensione cella
Player::Player(float speed, const sf::Vector2f& startPos, const sf::Vector2u& tileSize)
    : m_shape(tileSize.x * 0.4f)
    , m_speed(speed)
    , m_direction{0.f, 0.f}
    , m_nextDirection{0.f, 0.f}
    , m_tileSize(tileSize)
    , m_lives(3) // Inizializza con 3 vite
    , m_hasTexture(false)
    , m_animTime(0.f)
    , m_animFrame(0)
    // --- VARIABILI ANIMAZIONE MORTE ---
    , m_isDying(false)
    , m_deathAnimTime(0.f)
    , m_deathFrame(0)
    , m_deathAnimFinished(false)
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
        // Imposta il frame iniziale: DESTRA, bocca chiusa
        m_sprite->setTextureRect(PACMAN_FRAMES[2][0]);
        std::cout << "[DEBUG] Frame iniziale impostato a destra, bocca chiusa (1,563,32,32)" << std::endl;
        
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

// Utility: calcola la direzione logica Pac-Man
static PacmanDir getPacmanDir(const sf::Vector2f& dir) {
    if (dir.x < 0) return LEFT;
    if (dir.x > 0) return RIGHT;
    if (dir.y < 0) return UP;
    if (dir.y > 0) return DOWN;
    return NONE;
}

// Avvia l'animazione di morte
void Player::startDeathAnimation() {
    m_isDying = true;
    m_deathAnimTime = 0.f;
    m_deathFrame = 0;
    m_deathAnimFinished = false;
    m_direction = {0.f, 0.f}; // Blocca movimento
}

// Controlla se l'animazione di morte è finita
bool Player::isDeathAnimationFinished() const {
    return m_deathAnimFinished;
}

// Aggiorna la posizione e la direzione di Pac-Man in base all'input e alle collisioni
void Player::update(float dt, const TileMap& map, const sf::Vector2u& tileSize) {
    if (m_isDying) {
        // Solo animazione morte
        m_deathAnimTime += dt;
        if (m_deathAnimTime >= DEATH_ANIMATION_INTERVAL) {
            m_deathAnimTime = 0.f;
            m_deathFrame++;
            if (m_deathFrame >= 11) {
                m_deathFrame = 10;
                m_isDying = false;
                m_deathAnimFinished = true;
            }
        }
        if (m_hasTexture && m_sprite) {
            m_sprite->setTextureRect(PACMAN_DEATH_FRAMES[m_deathFrame]);
            // Centra lo sprite (frame 16x16)
            m_sprite->setOrigin(sf::Vector2f{8.f, 8.f});
            m_sprite->setScale(sf::Vector2f(
                static_cast<float>(tileSize.x) / 16.f * 0.75f,
                static_cast<float>(tileSize.y) / 16.f * 0.75f
            ));
        }
        return;
    }

    using Key = sf::Keyboard::Key;

    // Leggi input direzionale (Frecce o WASD)
    bool left  = sf::Keyboard::isKeyPressed(Key::Left)  || sf::Keyboard::isKeyPressed(Key::A);
    bool right = sf::Keyboard::isKeyPressed(Key::Right) || sf::Keyboard::isKeyPressed(Key::D);
    bool up    = sf::Keyboard::isKeyPressed(Key::Up)    || sf::Keyboard::isKeyPressed(Key::W);
    bool down  = sf::Keyboard::isKeyPressed(Key::Down)  || sf::Keyboard::isKeyPressed(Key::S);

    if      (left)  m_nextDirection = {-1.f,  0.f};
    else if (right) m_nextDirection = { 1.f,  0.f};
    else if (up)    m_nextDirection = { 0.f, -1.f};
    else if (down)  m_nextDirection = { 0.f,  1.f};

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
        // --- ANIMAZIONE SPRITE ---
        m_animTime += dt;
        int animFrames = 3;
        if (m_direction != sf::Vector2f{0,0}) { // Anima solo se si muove
            if (m_animTime >= ANIMATION_INTERVAL) {
                m_animFrame = (m_animFrame + 1) % animFrames; // 0,1,2
                m_animTime = 0.f;
            }
        } else {
            // Se fermo, bocca chiusa
            m_animFrame = 0;
        }
        PacmanDir dir = getPacmanDir(m_direction);
        if (dir == NONE) dir = getPacmanDir(m_nextDirection); // fallback
        if (dir == NONE) dir = RIGHT; // default
        m_sprite->setTextureRect(PACMAN_FRAMES[dir][m_animFrame]);
        // Niente rotazione: frame già orientato
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

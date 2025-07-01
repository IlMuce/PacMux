#include "Ghost.hpp"
#include <cmath>
#include <queue>
#include <unordered_map>

Ghost::Ghost(const sf::Vector2f& pos, sf::Color color, float radius, Type type)
    : m_shape(radius), m_direction(0, -1), m_speed(90.f), m_type(type), m_mode(Mode::Chase), m_drawPos(pos), m_hasLeftGhostHouse(false)
{
    m_shape.setFillColor(color);
    m_shape.setOrigin({radius, radius});
    m_shape.setPosition(pos);
    m_target = pos;
}

void Ghost::update(float dt, const TileMap& map, const sf::Vector2u& tileSize, const sf::Vector2f& pacmanPos, Mode mode) {
    m_mode = mode;
    sf::Vector2f pos = m_shape.getPosition();
    float cx = std::round((pos.x - tileSize.x/2.f) / tileSize.x);
    float cy = std::round((pos.y - tileSize.y/2.f) / tileSize.y);
    sf::Vector2f center{
        cx * tileSize.x + tileSize.x/2.f,
        cy * tileSize.y + tileSize.y/2.f
    };
    bool centered = (std::abs(pos.x - center.x) < 2.0f && std::abs(pos.y - center.y) < 2.0f);
    bool nearCenter = (std::abs(pos.x - center.x) < 8.0f && std::abs(pos.y - center.y) < 8.0f);
    int w = map.getSize().x, h = map.getSize().y;
    
    // Aggiorna direzione se:
    // - È centrato su una tile
    // - È la prima volta (direzione è verso l'alto di default)
    // - È bloccato (non può muoversi nella direzione corrente)
    bool shouldUpdateDirection = centered || 
                               (m_direction.x == 0 && m_direction.y == -1); // Prima volta
                               
    if (shouldUpdateDirection) {
        int sx = int(std::round(cx));
        int sy = int(std::round(cy));
        
        // Controlla se il fantasma è uscito dalla ghost house
        if (!m_hasLeftGhostHouse && !map.isGhostHouse(sx, sy)) {
            m_hasLeftGhostHouse = true;
        }
        
        // Se siamo nella ghost house ma il flag dice che siamo usciti, correggi
        if (m_hasLeftGhostHouse && map.isGhostHouse(sx, sy)) {
            m_hasLeftGhostHouse = false;
        }
        
        // Calcola il target e la direzione basata su modalità e tipo di fantasma
        sf::Vector2f target = pacmanPos; // Per ora solo Chase mode
        sf::Vector2f newDirection;
        
        if (m_type == Type::Blinky) {
            // Blinky: algoritmo greedy avanzato
            newDirection = findPathToPacman(pacmanPos, map, tileSize);
        } else {
            // Altri fantasmi: logica greedy semplice (da migliorare in futuro)
            std::vector<sf::Vector2f> dirs = {{0,-1},{-1,0},{0,1},{1,0}};
            float minDist = 1e9f;
            newDirection = m_direction;
            
            for (auto& d : dirs) {
                if (d + m_direction == sf::Vector2f(0,0) && m_direction != sf::Vector2f(0,0)) continue;
                
                int nx = sx + int(d.x);
                int ny = sy + int(d.y);
                if (nx < 0) nx = w-1; if (nx >= w) nx = 0;
                if (ny < 0) ny = h-1; if (ny >= h) ny = 0;
                if (map.isWall(nx, ny)) continue;
                if (m_hasLeftGhostHouse && map.isGhostHouse(nx, ny)) continue;
                
                sf::Vector2f candPos{nx * float(tileSize.x) + tileSize.x/2.f, ny * float(tileSize.y) + tileSize.y/2.f};
                float dist = std::hypot(target.x - candPos.x, target.y - candPos.y);
                if (dist < minDist) {
                    minDist = dist;
                    newDirection = d;
                }
            }
        }
        
        m_direction = newDirection;
    }
    // Movimento: solo se la prossima tile non è muro
    int nextX = int(std::round(cx + m_direction.x));
    int nextY = int(std::round(cy + m_direction.y));
    if (nextX < 0) nextX = w - 1;
    if (nextX >= w) nextX = 0;
    if (nextY < 0) nextY = h - 1;
    if (nextY >= h) nextY = 0;
    
    // Controlla se il movimento è valido
    bool canMove = !map.isWall(nextX, nextY);
    // Se il fantasma è uscito dalla ghost house, non può rientrarvi
    if (m_hasLeftGhostHouse && map.isGhostHouse(nextX, nextY)) {
        canMove = false;
    }
    
    // Se è bloccato e Blinky, prova a cambiare direzione
    if (!canMove && m_type == Type::Blinky && !shouldUpdateDirection) {
        int sx = int(std::round(cx));
        int sy = int(std::round(cy));
        
        sf::Vector2f newDirection = findPathToPacman(pacmanPos, map, tileSize);
        if (newDirection != m_direction) {
            m_direction = newDirection;
            
            // Ricalcola con la nuova direzione
            nextX = int(std::round(cx + m_direction.x));
            nextY = int(std::round(cy + m_direction.y));
            if (nextX < 0) nextX = w - 1;
            if (nextX >= w) nextX = 0;
            if (nextY < 0) nextY = h - 1;
            if (nextY >= h) nextY = 0;
            
            canMove = !map.isWall(nextX, nextY);
            if (m_hasLeftGhostHouse && map.isGhostHouse(nextX, nextY)) {
                canMove = false;
            }
        }
    }
    
    if (m_direction != sf::Vector2f(0,0) && canMove) {
        sf::Vector2f dest{nextX * float(tileSize.x) + tileSize.x/2.f, nextY * float(tileSize.y) + tileSize.y/2.f};
        sf::Vector2f delta = dest - m_shape.getPosition();
        float step = m_speed * dt;
        
        // Movimento fluido verso la destinazione
        if (std::hypot(delta.x, delta.y) <= step) {
            m_shape.setPosition(dest);
        } else {
            // Normalizza la direzione per movimento uniforme
            float deltaLen = std::hypot(delta.x, delta.y);
            if (deltaLen > 0) {
                sf::Vector2f normalizedDelta = delta / deltaLen;
                m_shape.move(normalizedDelta * step);
            }
        }
        
        // Wrap-around effettivo
        sf::Vector2f p = m_shape.getPosition();
        if (p.x < 0) p.x = map.getSize().x * tileSize.x + p.x;
        if (p.x >= map.getSize().x * tileSize.x) p.x -= map.getSize().x * tileSize.x;
        if (p.y < 0) p.y = map.getSize().y * tileSize.y + p.y;
        if (p.y >= map.getSize().y * tileSize.y) p.y -= map.getSize().y * tileSize.y;
        m_shape.setPosition(p);
    } else if (m_direction == sf::Vector2f(0,0)) {
        // Se la direzione è nulla, prova a muoverti verso l'alto (direzione di default per uscire dalla ghost house)
        m_direction = {0, -1};
    }
    // Rimuovo l'else che riportava il fantasma al centro quando non poteva muoversi
    // Aggiorna posizione grafica direttamente alla posizione logica (rimuove interpolazione che causa effetti strani)
    m_drawPos = m_shape.getPosition();
}

void Ghost::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    sf::CircleShape shape = m_shape;
    shape.setPosition(m_drawPos);
    target.draw(shape, states);
}

void Ghost::setPosition(const sf::Vector2f& pos) {
    m_shape.setPosition(pos);
    m_direction = {0, -1};
    m_drawPos = pos;
    m_hasLeftGhostHouse = false; // Reset del flag quando il fantasma viene riposizionato
}

// Algoritmo greedy intelligente per Blinky - semplice ma efficace
sf::Vector2f Ghost::findPathToPacman(const sf::Vector2f& pacmanPos, const TileMap& map, const sf::Vector2u& tileSize) {
    sf::Vector2f pos = m_shape.getPosition();
    
    // Converti posizioni in coordinate tile
    int startX = int(std::round((pos.x - tileSize.x/2.f) / tileSize.x));
    int startY = int(std::round((pos.y - tileSize.y/2.f) / tileSize.y));
    int targetX = int(std::round((pacmanPos.x - tileSize.x/2.f) / tileSize.x));
    int targetY = int(std::round((pacmanPos.y - tileSize.y/2.f) / tileSize.y));
    
    int w = map.getSize().x, h = map.getSize().y;
    
    // Clamp alle dimensioni della mappa
    startX = std::max(0, std::min(w-1, startX));
    startY = std::max(0, std::min(h-1, startY));
    targetX = std::max(0, std::min(w-1, targetX));
    targetY = std::max(0, std::min(h-1, targetY));
    
    // Se siamo ancora nella ghost house, priorità assoluta per uscire
    if (map.isGhostHouse(startX, startY)) {
        // Cerca una via d'uscita dalla ghost house andando verso l'alto
        if (startY > 0 && !map.isWall(startX, startY - 1)) {
            return {0, -1}; // Su
        }
        // Se non può andare su, prova le altre direzioni
        std::vector<sf::Vector2f> directions = {{-1,0}, {1,0}, {0,1}};
        for (const auto& dir : directions) {
            int nextX = startX + int(dir.x);
            int nextY = startY + int(dir.y);
            
            if (nextX >= 0 && nextX < w && nextY >= 0 && nextY < h) {
                if (!map.isWall(nextX, nextY)) {
                    return dir;
                }
            }
        }
        // Fallback: va su anche se c'è un muro
        return {0, -1};
    }
    
    // Direzioni con tie-breaking: Up, Left, Down, Right
    std::vector<sf::Vector2f> directions = {{0,-1}, {-1,0}, {0,1}, {1,0}};
    
    sf::Vector2f bestDir = m_direction;
    float bestScore = -1e9f;
    bool foundAnyMove = false;
    
    // Verifica se ci sono alternative al reverse
    int validMoves = 0;
    for (const auto& testDir : directions) {
        int testX = startX + int(testDir.x);
        int testY = startY + int(testDir.y);
        
        if (testX < 0 && startY == 10) testX = w - 1;
        else if (testX >= w && startY == 10) testX = 0;
        else if (testX < 0 || testX >= w) continue;
        
        if (testY < 0 || testY >= h) continue;
        
        if (!map.isWall(testX, testY) && !map.isGhostHouse(testX, testY)) {
            validMoves++;
        }
    }
    bool onlyReverseAvailable = (validMoves == 1);
    
    for (const auto& dir : directions) {
        int nextX = startX + int(dir.x);
        int nextY = startY + int(dir.y);
        
        // Gestione wrap-around solo per tunnel orizzontali (riga 10)
        if (nextX < 0 && startY == 10) nextX = w - 1;
        else if (nextX >= w && startY == 10) nextX = 0;
        else if (nextX < 0 || nextX >= w) continue;
        
        if (nextY < 0 || nextY >= h) continue;
        
        // Verifica se la mossa è valida
        if (map.isWall(nextX, nextY)) continue;
        if (map.isGhostHouse(nextX, nextY)) continue;
        
        // Calcola la posizione candidata
        sf::Vector2f candPos{nextX * float(tileSize.x) + tileSize.x/2.f, 
                           nextY * float(tileSize.y) + tileSize.y/2.f};
        
        // Distanza diretta a Pac-Man (più piccola è meglio)
        float dist = std::hypot(pacmanPos.x - candPos.x, pacmanPos.y - candPos.y);
        
        // Calcola un punteggio considerando diversi fattori
        float score = 0.0f;
        
        // 1. Vicinanza a Pac-Man (peso maggiore)
        score -= dist * 10.0f;
        
        // 2. Penalità per reverse (evita oscillazioni ma non blocca)
        bool isReverse = (dir + m_direction == sf::Vector2f(0,0) && m_direction != sf::Vector2f(0,0));
        if (isReverse) {
            // Caso speciale: se Pac-Man è direttamente sopra/sotto e noi stiamo andando orizzontalmente,
            // il reverse potrebbe essere necessario per raggiungerlo
            bool pacmanAboveOrBelow = (startX == targetX && std::abs(startY - targetY) <= 2);
            bool movingHorizontally = (m_direction.x != 0);
            
            if (pacmanAboveOrBelow && movingHorizontally) {
                // Riduci drasticamente la penalità per reverse se Pac-Man è sopra/sotto
                score -= 5.0f;
            } else if (onlyReverseAvailable) {
                // Se è l'unica mossa disponibile, riduci drasticamente la penalità
                score -= 10.0f;
            } else {
                // Penalità normale per reverse
                score -= 50.0f;
            }
        }
        
        // 3. Bonus per muoversi verso la stessa coordinata X o Y di Pac-Man
        if (nextX == targetX) score += 50.0f; // Stesso X
        if (nextY == targetY) score += 50.0f; // Stesso Y
        
        // 4. Tie-breaking standard: Up > Left > Down > Right
        if (dir.y < 0) score += 3.0f; // Up
        else if (dir.x < 0) score += 2.0f; // Left
        else if (dir.y > 0) score += 1.0f; // Down
        // Right ha score 0
        
        // 5. Considera la direzione generale verso Pac-Man
        float dx = targetX - startX;
        float dy = targetY - startY;
        
        // Gestione wrap-around per calcolo direzione
        if (startY == 10 && targetY == 10) {
            // Sulla riga del tunnel, considera il percorso più breve
            float normalDx = dx;
            float wrapDx = (dx > 0) ? (dx - w) : (dx + w);
            dx = (std::abs(normalDx) < std::abs(wrapDx)) ? normalDx : wrapDx;
        }
        
        // Bonus se la direzione si allinea con la direzione generale
        if ((dx > 0 && dir.x > 0) || (dx < 0 && dir.x < 0)) score += 25.0f;
        if ((dy > 0 && dir.y > 0) || (dy < 0 && dir.y < 0)) score += 25.0f;
        
        // Bonus extra per movimento verso coordinate diverse (anti-stallo)
        if (std::abs(dx) > 0 && ((dx > 0 && dir.x > 0) || (dx < 0 && dir.x < 0))) score += 15.0f;
        if (std::abs(dy) > 0 && ((dy > 0 && dir.y > 0) || (dy < 0 && dir.y < 0))) score += 15.0f;
        
        // Bonus speciale quando Pac-Man è sullo stesso asse X o Y
        if (startX == targetX && dir.y != 0) {
            // Pac-Man è sullo stesso X, movimento verticale è molto importante
            if ((dy > 0 && dir.y > 0) || (dy < 0 && dir.y < 0)) {
                score += 100.0f; // Bonus molto alto per movimento verso Pac-Man
            }
        }
        if (startY == targetY && dir.x != 0) {
            // Pac-Man è sullo stesso Y, movimento orizzontale è molto importante
            if ((dx > 0 && dir.x > 0) || (dx < 0 && dir.x < 0)) {
                score += 100.0f; // Bonus molto alto per movimento verso Pac-Man
            }
        }
        
        // Seleziona la direzione migliore
        if (!foundAnyMove || score > bestScore) {
            bestScore = score;
            bestDir = dir;
            foundAnyMove = true;
        }
    }
    
    // Se non troviamo nessuna mossa valida, usa il fallback
    if (!foundAnyMove) {
        return greedyFallback(pacmanPos, map, tileSize, startX, startY, w, h);
    }
    
    return bestDir;
}

// Fallback greedy aggressivo - va sempre verso Pac-Man
sf::Vector2f Ghost::greedyFallback(const sf::Vector2f& pacmanPos, const TileMap& map, const sf::Vector2u& tileSize, 
                                   int startX, int startY, int w, int h) {
    std::vector<sf::Vector2f> directions = {{0,-1}, {-1,0}, {0,1}, {1,0}};
    float minDist = 1e9f;
    sf::Vector2f bestDir = m_direction;
    bool foundValidMove = false;
    
    // Primo tentativo: trova la mossa migliore evitando reverse
    for (const auto& dir : directions) {
        // Evita reverse solo se ci sono alternative
        if (dir + m_direction == sf::Vector2f(0,0) && m_direction != sf::Vector2f(0,0)) continue;
        
        int nextX = startX + int(dir.x);
        int nextY = startY + int(dir.y);
        
        // Gestione wrap-around per tunnel
        if (nextX < 0 && startY == 10) nextX = w - 1;
        else if (nextX >= w && startY == 10) nextX = 0;
        else if (nextX < 0 || nextX >= w) continue;
        
        if (nextY < 0 || nextY >= h) continue;
        
        if (!map.isWall(nextX, nextY)) {
            if (!map.isGhostHouse(nextX, nextY)) {
                sf::Vector2f candPos{nextX * float(tileSize.x) + tileSize.x/2.f, 
                                   nextY * float(tileSize.y) + tileSize.y/2.f};
                float dist = std::hypot(pacmanPos.x - candPos.x, pacmanPos.y - candPos.y);
                if (dist < minDist) {
                    minDist = dist;
                    bestDir = dir;
                    foundValidMove = true;
                }
            }
        }
    }
    
    // Se non trova mosse valide senza reverse, accetta anche il reverse
    if (!foundValidMove) {
        for (const auto& dir : directions) {
            int nextX = startX + int(dir.x);
            int nextY = startY + int(dir.y);
            
            // Gestione wrap-around per tunnel
            if (nextX < 0 && startY == 10) nextX = w - 1;
            else if (nextX >= w && startY == 10) nextX = 0;
            else if (nextX < 0 || nextX >= w) continue;
            
            if (nextY < 0 || nextY >= h) continue;
            
            if (!map.isWall(nextX, nextY)) {
                if (!map.isGhostHouse(nextX, nextY)) {
                    sf::Vector2f candPos{nextX * float(tileSize.x) + tileSize.x/2.f, 
                                       nextY * float(tileSize.y) + tileSize.y/2.f};
                    float dist = std::hypot(pacmanPos.x - candPos.x, pacmanPos.y - candPos.y);
                    if (dist < minDist) {
                        minDist = dist;
                        bestDir = dir;
                        foundValidMove = true;
                    }
                }
            }
        }
    }
    
    // Se ancora non abbiamo trovato nessuna mossa valida, prendi qualsiasi direzione valida
    if (!foundValidMove) {
        for (const auto& dir : directions) {
            int nextX = startX + int(dir.x);
            int nextY = startY + int(dir.y);
            
            // Gestione wrap-around per tunnel
            if (nextX < 0 && startY == 10) nextX = w - 1;
            else if (nextX >= w && startY == 10) nextX = 0;
            else if (nextX < 0 || nextX >= w) continue;
            
            if (nextY < 0 || nextY >= h) continue;
            
            if (!map.isWall(nextX, nextY)) {
                // Accetta qualsiasi mossa valida, anche nella ghost house
                return dir;
            }
        }
    }
    
    return bestDir;
}

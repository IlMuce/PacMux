#include "Blinky.hpp"
#include <cmath>
#include <algorithm>

Blinky::Blinky(const sf::Vector2f& pos) : Ghost(pos, sf::Color::Red, 12.0f, Type::Blinky) {
}

sf::Vector2f Blinky::calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                                   const TileMap& map, const sf::Vector2u& tileSize) {
    // Blinky punta direttamente a Pac-Man
    return pacmanPos;
}

// Override del metodo findPath per usare la logica avanzata di Blinky
sf::Vector2f Blinky::findPath(const sf::Vector2f& target, const TileMap& map, const sf::Vector2u& tileSize) {
    return findPathToPacman(target, map, tileSize);
}

sf::Vector2f Blinky::findPathToPacman(const sf::Vector2f& pacmanPos, const TileMap& map, const sf::Vector2u& tileSize) {
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
        if (startY > 0 && !map.isWall(startX, startY - 1)) {
            return {0, -1}; // Su
        }
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
        
        // Gestione tunnel avanzata con wrap-around
        bool isValidX = true, isValidY = true;
        if (testX < 0) {
            if (startY == 10) testX = w - 1; // Tunnel a sinistra
            else isValidX = false;
        } else if (testX >= w) {
            if (startY == 10) testX = 0; // Tunnel a destra
            else isValidX = false;
        }
        
        if (testY < 0 || testY >= h) isValidY = false;
        
        if (!isValidX || !isValidY) continue;
        
        if (!map.isWall(testX, testY) && !map.isGhostHouse(testX, testY)) {
            validMoves++;
        }
    }
    bool onlyReverseAvailable = (validMoves == 1);
    
    for (const auto& dir : directions) {
        int nextX = startX + int(dir.x);
        int nextY = startY + int(dir.y);
        
        // Gestione tunnel avanzata con wrap-around
        bool isValidX = true, isValidY = true;
        int originalNextX = nextX;
        
        if (nextX < 0) {
            if (startY == 10) nextX = w - 1; // Tunnel a sinistra
            else isValidX = false;
        } else if (nextX >= w) {
            if (startY == 10) nextX = 0; // Tunnel a destra
            else isValidX = false;
        }
        
        if (nextY < 0 || nextY >= h) isValidY = false;
        
        if (!isValidX || !isValidY) continue;
        
        // Verifica se la mossa è valida
        if (map.isWall(nextX, nextY)) continue;
        if (map.isGhostHouse(nextX, nextY)) continue;
        
        // Calcola la posizione candidata
        sf::Vector2f candPos{nextX * float(tileSize.x) + tileSize.x/2.f, 
                           nextY * float(tileSize.y) + tileSize.y/2.f};
        
        // Distanza diretta a Pac-Man
        float dist = std::hypot(pacmanPos.x - candPos.x, pacmanPos.y - candPos.y);
        
        // Calcola un punteggio considerando diversi fattori
        float score = 0.0f;
        
        // 1. Vicinanza a Pac-Man
        score -= dist * 10.0f;
        
        // 2. Penalità per reverse (rafforzata)
        bool isReverse = (dir + m_direction == sf::Vector2f(0,0) && m_direction != sf::Vector2f(0,0));
        if (isReverse) {
            if (onlyReverseAvailable) {
                score -= 50.0f; // Penalità ridotta se è l'unica opzione
            } else {
                score -= 200.0f; // Penalità alta per evitare reverse quando ci sono alternative
            }
        }
        
        // 3. Bonus per allineamento
        if (nextX == targetX) score += 50.0f;
        if (nextY == targetY) score += 50.0f;
        
        // 4. Tie-breaking
        if (dir.y < 0) score += 3.0f;
        else if (dir.x < 0) score += 2.0f;
        else if (dir.y > 0) score += 1.0f;
        
        // 5. Direzione generale verso Pac-Man con gestione tunnel intelligente
        float dx = targetX - startX;
        float dy = targetY - startY;
        
        // Gestione tunnel avanzata (riga 10)
        if (startY == 10 && targetY == 10) {
            float normalDx = dx;
            float wrapDx = (dx > 0) ? (dx - w) : (dx + w);
            
            // Scegli il percorso più breve, ma con una soglia per evitare oscillazioni
            if (std::abs(wrapDx) < std::abs(normalDx) - 2) {
                dx = wrapDx;
                // Bonus per usare il tunnel quando è vantaggioso
                if ((originalNextX < 0 && wrapDx < 0) || (originalNextX >= w && wrapDx > 0)) {
                    score += 40.0f;
                }
            } else {
                dx = normalDx;
                // Penalità per usare il tunnel quando non è vantaggioso
                if ((originalNextX < 0 && normalDx > 0) || (originalNextX >= w && normalDx < 0)) {
                    score -= 30.0f;
                }
            }
        }
        
        if ((dx > 0 && dir.x > 0) || (dx < 0 && dir.x < 0)) score += 25.0f;
        if ((dy > 0 && dir.y > 0) || (dy < 0 && dir.y < 0)) score += 25.0f;
        
        // Bonus extra per movimento verso coordinate diverse
        if (std::abs(dx) > 0 && ((dx > 0 && dir.x > 0) || (dx < 0 && dir.x < 0))) score += 15.0f;
        if (std::abs(dy) > 0 && ((dy > 0 && dir.y > 0) || (dy < 0 && dir.y < 0))) score += 15.0f;
        
        // Penalità per oscillazione rafforzata
        if (dir == m_direction && std::abs(dx) <= 1 && std::abs(dy) <= 1) {
            score -= 30.0f; // Penalità aumentata per evitare di rimanere bloccato
        }
        
        // Bonus per continuare nella direzione corrente se si sta avvicinando
        if (dir == m_direction && (std::abs(dx) > 1 || std::abs(dy) > 1)) {
            score += 35.0f; // Bonus per continuità aumentato
        }
        
        // Bonus speciale quando allineati - MA solo se non ci sono muri nel percorso
        if (startX == targetX && dir.y != 0) {
            if ((dy > 0 && dir.y > 0) || (dy < 0 && dir.y < 0)) {
                // Controlla se c'è un percorso libero verso Pac-Man
                bool pathClear = true;
                int checkY = startY + int(dir.y);
                while (checkY != targetY && checkY >= 0 && checkY < h) {
                    if (map.isWall(startX, checkY)) {
                        pathClear = false;
                        break;
                    }
                    checkY += int(dir.y);
                }
                if (pathClear) {
                    score += 100.0f; // Bonus alto per percorso diretto
                } else {
                    score += 10.0f; // Bonus ridotto se c'è un ostacolo
                }
            }
        }
        if (startY == targetY && dir.x != 0) {
            if ((dx > 0 && dir.x > 0) || (dx < 0 && dir.x < 0)) {
                // Controlla se c'è un percorso libero verso Pac-Man
                bool pathClear = true;
                int checkX = startX + int(dir.x);
                while (checkX != targetX && checkX >= 0 && checkX < w) {
                    if (map.isWall(checkX, startY)) {
                        pathClear = false;
                        break;
                    }
                    checkX += int(dir.x);
                }
                if (pathClear) {
                    score += 100.0f; // Bonus alto per percorso diretto
                } else {
                    score += 10.0f; // Bonus ridotto se c'è un ostacolo
                }
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
        sf::Vector2f fallbackResult = greedyFallback(pacmanPos, map, tileSize, startX, startY, w, h);
        // Se anche il fallback fallisce, continua nella direzione corrente o forza movimento
        if (fallbackResult == sf::Vector2f(0,0)) {
            // Prova a continuare nella direzione corrente se possibile
            if (m_direction != sf::Vector2f(0,0) && canMove(m_direction, map, tileSize)) {
                return m_direction;
            }
            // Altrimenti, trova qualsiasi direzione valida
            std::vector<sf::Vector2f> emergencyDirections = {{0,-1}, {-1,0}, {0,1}, {1,0}};
            for (const auto& dir : emergencyDirections) {
                if (canMove(dir, map, tileSize)) {
                    return dir;
                }
            }
            return {0, -1}; // Default: su
        }
        return fallbackResult;
    }

    return bestDir;
}

sf::Vector2f Blinky::greedyFallback(const sf::Vector2f& pacmanPos, const TileMap& map, const sf::Vector2u& tileSize, 
                                   int startX, int startY, int w, int h) {
    std::vector<sf::Vector2f> directions = {{0,-1}, {-1,0}, {0,1}, {1,0}};
    float minDist = 1e9f;
    sf::Vector2f bestDir = m_direction;
    bool foundValidMove = false;
    
    // Primo tentativo: evita reverse
    for (const auto& dir : directions) {
        if (dir + m_direction == sf::Vector2f(0,0) && m_direction != sf::Vector2f(0,0)) continue;
        
        int nextX = startX + int(dir.x);
        int nextY = startY + int(dir.y);
        
        // Gestione tunnel avanzata
        bool isValidX = true;
        if (nextX < 0) {
            if (startY == 10) nextX = w - 1; // Tunnel a sinistra
            else isValidX = false;
        } else if (nextX >= w) {
            if (startY == 10) nextX = 0; // Tunnel a destra
            else isValidX = false;
        }
        
        if (!isValidX || nextY < 0 || nextY >= h) continue;
        
        if (!map.isWall(nextX, nextY) && !map.isGhostHouse(nextX, nextY)) {
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
    
    // Se non trovato, accetta anche reverse
    if (!foundValidMove) {
        for (const auto& dir : directions) {
            int nextX = startX + int(dir.x);
            int nextY = startY + int(dir.y);
            
            // Gestione tunnel avanzata
            bool isValidX = true;
            if (nextX < 0) {
                if (startY == 10) nextX = w - 1; // Tunnel a sinistra
                else isValidX = false;
            } else if (nextX >= w) {
                if (startY == 10) nextX = 0; // Tunnel a destra
                else isValidX = false;
            }
            
            if (!isValidX || nextY < 0 || nextY >= h) continue;
            
            if (!map.isWall(nextX, nextY)) {
                return dir;
            }
        }
        
        // Ultimo fallback: se proprio non trova nulla, continua nella direzione corrente
        // o prova qualsiasi direzione non-muro
        if (m_direction != sf::Vector2f(0,0)) {
            // Verifica se la direzione corrente è ancora valida
            int currentNextX = startX + int(m_direction.x);
            int currentNextY = startY + int(m_direction.y);
            
            // Gestione tunnel per direzione corrente
            bool isValidCurrentX = true;
            if (currentNextX < 0) {
                if (startY == 10) currentNextX = w - 1;
                else isValidCurrentX = false;
            } else if (currentNextX >= w) {
                if (startY == 10) currentNextX = 0;
                else isValidCurrentX = false;
            }
            
            if (isValidCurrentX && currentNextY >= 0 && currentNextY < h) {
                if (!map.isWall(currentNextX, currentNextY)) {
                    return m_direction;
                }
            }
        }
        
        // Se anche questo fallisce, forza una direzione qualsiasi non-muro
        for (const auto& dir : directions) {
            int nextX = startX + int(dir.x);
            int nextY = startY + int(dir.y);
            
            if (nextX >= 0 && nextX < w && nextY >= 0 && nextY < h) {
                if (!map.isWall(nextX, nextY)) {
                    return dir;
                }
            }
        }
        
        // Ultimo disperato tentativo - qualsiasi direzione anche se è un muro (dovrebbe mai succedere)
        return {0, -1}; // Su per default
    }

    return bestDir;
}

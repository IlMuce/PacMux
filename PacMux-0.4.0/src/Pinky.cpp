#include "Pinky.hpp"
#include <cmath>
#include <queue>
#include <unordered_set>
#include <algorithm>

// Pinky: il fantasma rosa, mira 4 caselle avanti a Pac-Man
Pinky::Pinky(const sf::Vector2f& pos) : Ghost(pos, sf::Color::Magenta, 12.0f, Type::Pinky) {}

// Target = 4 caselle avanti nella direzione di Pac-Man
sf::Vector2f Pinky::calculateTarget(const sf::Vector2f& pacmanPos, const sf::Vector2f& pacmanDirection, 
                                  const TileMap& map, const sf::Vector2u& tileSize) {
    sf::Vector2f target = pacmanPos;
    if (std::hypot(pacmanDirection.x, pacmanDirection.y) > 0.1f) {
        sf::Vector2f dir = pacmanDirection / std::hypot(pacmanDirection.x, pacmanDirection.y);
        target.x += 4.0f * dir.x * float(tileSize.x);
        target.y += 4.0f * dir.y * float(tileSize.y);
        // Clamp ai bordi
        int w = map.getSize().x, h = map.getSize().y;
        target.x = std::max(float(tileSize.x/2), std::min(target.x, (w-1) * float(tileSize.x) + float(tileSize.x/2)));
        target.y = std::max(float(tileSize.y/2), std::min(target.y, (h-1) * float(tileSize.y) + float(tileSize.y/2)));
    }
    return target;
}

sf::Vector2f Pinky::findPath(const sf::Vector2f& target, const TileMap& map, const sf::Vector2u& tileSize) {
    return aStar(m_shape.getPosition(), target, map, tileSize);
}

sf::Vector2f Pinky::aStar(const sf::Vector2f& start, const sf::Vector2f& target, 
                         const TileMap& map, const sf::Vector2u& tileSize) {
    int w = map.getSize().x, h = map.getSize().y;
    
    // Converti a coordinate tile
    int startX = int(std::round((start.x - tileSize.x/2.f) / tileSize.x));
    int startY = int(std::round((start.y - tileSize.y/2.f) / tileSize.y));
    int targetX = int(std::round((target.x - tileSize.x/2.f) / tileSize.x));
    int targetY = int(std::round((target.y - tileSize.y/2.f) / tileSize.y));
    
    // Clamp
    startX = std::max(0, std::min(w-1, startX));
    startY = std::max(0, std::min(h-1, startY));
    targetX = std::max(0, std::min(w-1, targetX));
    targetY = std::max(0, std::min(h-1, targetY));
    
    // Se siamo ancora nella ghost house, priorità per uscire
    if (map.isGhostHouse(startX, startY)) {
        // Prima prova ad uscire verso l'alto
        if (startY > 0 && !map.isWall(startX, startY - 1) && !map.isGhostHouse(startX, startY - 1)) {
            return {0, -1}; // Su
        }
        // Se non può andare su, prova altre direzioni
        std::vector<sf::Vector2f> directions = {{0,-1}, {-1,0}, {1,0}, {0,1}};
        for (const auto& dir : directions) {
            int nextX = startX + int(dir.x);
            int nextY = startY + int(dir.y);
            if (nextX >= 0 && nextX < w && nextY >= 0 && nextY < h) {
                if (!map.isWall(nextX, nextY)) {
                    return dir;
                }
            }
        }
        return {0, -1}; // Default
    }
    
    // Strutture per A*
    auto comp = [](Node* a, Node* b) { return a->fCost > b->fCost; };
    std::priority_queue<Node*, std::vector<Node*>, decltype(comp)> openSet(comp);
    std::unordered_set<int> openSetHash;
    std::unordered_set<int> closedSet;
    
    auto getHash = [w](int x, int y) { return y * w + x; };
    
    Node* startNode = new Node(startX, startY);
    startNode->gCost = 0;
    startNode->hCost = heuristic(startX, startY, targetX, targetY);
    startNode->fCost = startNode->gCost + startNode->hCost;
    
    openSet.push(startNode);
    openSetHash.insert(getHash(startX, startY));
    
    std::vector<Node*> allNodes; // Per cleanup
    allNodes.push_back(startNode);
    
    std::vector<sf::Vector2f> directions = {{0,-1}, {-1,0}, {0,1}, {1,0}};
    
    while (!openSet.empty()) {
        Node* current = openSet.top();
        openSet.pop();
        openSetHash.erase(getHash(current->x, current->y));
        closedSet.insert(getHash(current->x, current->y));
        
        // Se abbiamo raggiunto il target
        if (current->x == targetX && current->y == targetY) {
            // Ricostruisci il percorso
            std::vector<Node*> path;
            Node* pathNode = current;
            while (pathNode != nullptr) {
                path.push_back(pathNode);
                pathNode = pathNode->parent;
            }
            
            sf::Vector2f result = {0, -1}; // Default
            if (path.size() > 1) {
                Node* nextStep = path[path.size() - 2];
                result = {float(nextStep->x - startX), float(nextStep->y - startY)};
            }
            
            // Cleanup
            for (Node* node : allNodes) delete node;
            return result;
        }
        
        // Esplora i vicini
        for (const auto& dir : directions) {
            int nextX = current->x + int(dir.x);
            int nextY = current->y + int(dir.y);
            
            // Gestione wrap-around per tunnel
            if (nextX < 0 && current->y == 10) nextX = w - 1;
            else if (nextX >= w && current->y == 10) nextX = 0;
            else if (nextX < 0 || nextX >= w) continue;
            
            if (nextY < 0 || nextY >= h) continue;
            
            // Controlla se è percorribile
            if (map.isWall(nextX, nextY)) continue;
            if (m_hasLeftGhostHouse && map.isGhostHouse(nextX, nextY)) continue;
            
            int hash = getHash(nextX, nextY);
            if (closedSet.count(hash)) continue;
            
            float tentativeGCost = current->gCost + 1.0f;
            
            // Penalità per reverse
            sf::Vector2f moveDir = {float(nextX - current->x), float(nextY - current->y)};
            if (moveDir + m_direction == sf::Vector2f(0,0) && m_direction != sf::Vector2f(0,0)) {
                tentativeGCost += 0.5f; // Leggera penalità per reverse
            }
            
            // Bonus per tunnel
            if (current->y == 10 && (nextX == 0 || nextX == w-1)) {
                tentativeGCost -= 2.0f; // Incentiva l'uso del tunnel
            }
            
            if (!openSetHash.count(hash)) {
                Node* neighbor = new Node(nextX, nextY);
                neighbor->gCost = tentativeGCost;
                neighbor->hCost = heuristic(nextX, nextY, targetX, targetY);
                neighbor->fCost = neighbor->gCost + neighbor->hCost;
                neighbor->parent = current;
                
                openSet.push(neighbor);
                openSetHash.insert(hash);
                allNodes.push_back(neighbor);
            } else {
                // Trova il nodo negli openSet e aggiorna se necessario
                // Per semplicità, creiamo un nuovo nodo se migliore
                Node* neighbor = new Node(nextX, nextY);
                neighbor->gCost = tentativeGCost;
                neighbor->hCost = heuristic(nextX, nextY, targetX, targetY);
                neighbor->fCost = neighbor->gCost + neighbor->hCost;
                neighbor->parent = current;
                
                openSet.push(neighbor);
                allNodes.push_back(neighbor);
            }
        }
    }
    
    // Cleanup
    for (Node* node : allNodes) delete node;
    
    // Fallback: comportamento greedy semplice (usa la logica base di Ghost)
    return Ghost::findPath(target, map, tileSize);
}

float Pinky::heuristic(int x1, int y1, int x2, int y2) {
    // Distanza di Manhattan
    return float(std::abs(x1 - x2) + std::abs(y1 - y2));
}

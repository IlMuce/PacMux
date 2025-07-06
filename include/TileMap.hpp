#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <fstream>

class TileMap : public sf::Drawable, public sf::Transformable {
public:
    bool load(const std::string& filename, const sf::Vector2u& tileSize);
    bool isWall(unsigned x, unsigned y) const {
        return m_data[y][x] == '1';
    }
    
    // Controlla se una posizione è nella ghost house (per impedire il rientro ai fantasmi)
    bool isGhostHouse(unsigned x, unsigned y) const {
        // Definisce l'area della ghost house: 
        // - Riga 11 dove spawnano Inky, Pinky e Clyde
        // - Tile sotto Blinky (porta della ghost house)
        return (y == 10 && x >= 9 && x <= 11) || (y == 9 && x == 10);
    }
    
    sf::Vector2u getSize() const { return m_size; }

    // ← Getter per leggere la griglia di caratteri (per trovare 'P')
    const std::vector<std::string>& getData() const { return m_data; }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    std::vector<std::string>          m_data;
    std::vector<sf::RectangleShape>   m_tiles;
    sf::Vector2u                      m_size;
};

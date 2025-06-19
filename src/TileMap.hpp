#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <fstream>

class TileMap : public sf::Drawable, public sf::Transformable {
public:
    // carica la mappa e salva le linee in m_data
    bool load(const std::string& filename, const sf::Vector2u& tileSize);

    // restituisce true se nella cella (x,y) c’è un muro
    bool isWall(unsigned x, unsigned y) const {
        return m_data[y][x] == '1';
    }

    sf::Vector2u getSize() const { return m_size; }

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    std::vector<std::string>   m_data;
    std::vector<sf::RectangleShape> m_tiles;
    sf::Vector2u               m_size;
};

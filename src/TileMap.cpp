#include "TileMap.hpp"

bool TileMap::load(const std::string& filename, const sf::Vector2u& tileSize) {
    std::ifstream file(filename);
    if (!file) return false;

    m_data.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty())
            m_data.push_back(line);
    }
    file.close();

    m_size.x = static_cast<unsigned>(m_data[0].size());
    m_size.y = static_cast<unsigned>(m_data.size());
    m_tiles.clear();
    m_tiles.reserve(m_size.x * m_size.y);

    for (unsigned y = 0; y < m_size.y; ++y) {
        for (unsigned x = 0; x < m_size.x; ++x) {
            sf::RectangleShape tile(
                sf::Vector2f(
                    static_cast<float>(tileSize.x),
                    static_cast<float>(tileSize.y)
                )
            );
            tile.setPosition(
                sf::Vector2f(
                    static_cast<float>(x * tileSize.x),
                    static_cast<float>(y * tileSize.y)
                )
            );
            tile.setFillColor(m_data[y][x] == '1'
                ? sf::Color::Blue
                : sf::Color::Black
            );
            m_tiles.push_back(tile);
        }
    }
    return true;
}

void TileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    for (const auto& tile : m_tiles)
        target.draw(tile, states);
}

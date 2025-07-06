#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <stdexcept>

class Score {
public:
    // Costruttore: riceve il path del font da caricare
    explicit Score(const std::string& fontFile);

    // Aggiunge al punteggio corrente
    void add(unsigned value);

    // Disegna il punteggio sul target
    void draw(sf::RenderTarget& target) const;

private:
    unsigned                     m_score;  // punteggio attuale
    sf::Font                     m_font;   // font usato per il testo
    std::unique_ptr<sf::Text>    m_text;   // sf::Text costruito dinamicamente
};

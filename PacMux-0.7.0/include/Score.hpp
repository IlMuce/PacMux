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

    // Ottiene il punteggio corrente
    unsigned getScore() const { return m_score; }

    // Resetta il punteggio
    void resetScore() { m_score = 0; m_extraLifeThreshold = 10000; m_extraLifeGiven = false; updateText(); }

    // Controlla se è stata raggiunta una soglia per vita extra
    bool checkExtraLife();

    // Disegna il punteggio sul target
    void draw(sf::RenderTarget& target) const;

private:
    unsigned                     m_score;                 // punteggio attuale
    sf::Font                     m_font;                  // font usato per il testo
    std::unique_ptr<sf::Text>    m_text;                  // sf::Text costruito dinamicamente
    unsigned                     m_extraLifeThreshold;    // soglia per vita extra (10000)
    bool                         m_extraLifeGiven;        // flag per tracciare se la vita extra è già stata assegnata
    
    void updateText(); // Aggiorna il testo del punteggio
};

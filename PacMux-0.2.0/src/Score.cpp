#include "Score.hpp"

// Costruttore: inizializza il punteggio e prepara il testo a schermo
Score::Score(const std::string& fontFile)
    : m_score(0)
{
    // Carica il font dal file, lancia eccezione se fallisce
    if (!m_font.openFromFile(fontFile))
        throw std::runtime_error("Cannot load font: " + fontFile);

    // Crea l'oggetto sf::Text con font, stringa iniziale e dimensione carattere
    m_text = std::make_unique<sf::Text>(
        m_font,
        sf::String("Score: 0"),
        24u
    );
    // Imposta colore e posizione del testo
    m_text->setFillColor(sf::Color::White);
    m_text->setPosition({ 10.f, 10.f });
}

// Aggiunge punti al punteggio e aggiorna il testo
void Score::add(unsigned value) {
    m_score += value;
    m_text->setString(sf::String("Score: " + std::to_string(m_score)));
}

// Disegna il punteggio sulla finestra
void Score::draw(sf::RenderTarget& target) const {
    target.draw(*m_text);
}

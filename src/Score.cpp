#include "Score.hpp"

// Costruttore: inizializza il punteggio e prepara il testo a schermo
Score::Score(const std::string& fontFile)
    : m_score(0), m_extraLifeThreshold(10000), m_extraLifeGiven(false)
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
    updateText();
}

// Aggiorna il testo del punteggio
void Score::updateText() {
    m_text->setString(sf::String("Score: " + std::to_string(m_score)));
}

// Controlla se è stata raggiunta una soglia per vita extra (solo UNA volta per partita)
bool Score::checkExtraLife() {
    if (!m_extraLifeGiven && m_score >= m_extraLifeThreshold) {
        m_extraLifeGiven = true; // Segna che la vita extra è stata assegnata
        return true;
    }
    return false;
}

// Disegna il punteggio sulla finestra
void Score::draw(sf::RenderTarget& target) const {
    target.draw(*m_text);
}

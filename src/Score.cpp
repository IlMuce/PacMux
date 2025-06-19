#include "Score.hpp"

Score::Score(const std::string& fontFile)
: m_score(0)
{
    // 1) Carica il font, lancia eccezione se fallisce
    if (!m_font.openFromFile(fontFile))
        throw std::runtime_error("Cannot load font: " + fontFile);

    // 2) Crea sf::Text con la firma SFML 3: (const sf::Font&, sf::String, unsigned int)
    m_text = std::make_unique<sf::Text>(
        m_font,
        sf::String("Score: 0"),
        24u
    );

    // 3) Imposta stile e posizione
    m_text->setFillColor(sf::Color::White);
    m_text->setPosition({ 10.f, 10.f });
}

void Score::add(unsigned value) {
    m_score += value;
    // Aggiorna la stringa del testo
    m_text->setString(sf::String("Score: " + std::to_string(m_score)));
}

void Score::draw(sf::RenderTarget& target) const {
    target.draw(*m_text);
}

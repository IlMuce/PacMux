#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>

struct HighScoreEntry {
    std::string playerName;
    unsigned int score;
    std::string date; // formato: "DD/MM/YYYY"
};

class HighScore {
public:
    // Costruttore: riceve il path del font da caricare
    explicit HighScore(const std::string& fontFile);

    // Carica i record dal file
    void loadFromFile(const std::string& filename = "highscores.json");

    // Salva i record nel file
    void saveToFile(const std::string& filename = "highscores.json") const;

    // Controlla se un punteggio è tra i migliori (top 10)
    bool isHighScore(unsigned int score) const;

    // Aggiunge un nuovo record alla lista (mantiene solo top 10)
    void addScore(const std::string& playerName, unsigned int score);

    // Ottiene la lista dei record (ordinata dal più alto al più basso)
    const std::vector<HighScoreEntry>& getScores() const { return m_scores; }

    // Disegna la schermata dei record
    void draw(sf::RenderTarget& target, const sf::Vector2u& windowSize) const;

    // Ottiene il punteggio più alto
    unsigned int getTopScore() const;

private:
    static const size_t MAX_SCORES = 10; // Numero massimo di record da mantenere
    std::vector<HighScoreEntry> m_scores;
    sf::Font m_font;
    std::string m_filename;

    // Metodi di utilità
    std::string getCurrentDate() const;
    void sortScores();
    std::string parseJsonFile(const std::string& filename) const;
    void writeJsonFile(const std::string& filename, const std::string& jsonContent) const;
};

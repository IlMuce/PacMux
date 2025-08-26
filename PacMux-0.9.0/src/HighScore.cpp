#include "HighScore.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>

// Costruttore: inizializza il font e il nome del file
HighScore::HighScore(const std::string& fontFile)
    : m_filename("highscores.json")
{
    // Carica il font dal file, lancia eccezione se fallisce
    if (!m_font.openFromFile(fontFile))
        throw std::runtime_error("Cannot load font: " + fontFile);
}

// Carica i record dal file JSON
void HighScore::loadFromFile(const std::string& filename) {
    m_filename = filename;
    m_scores.clear();

    // std::cout << "[DEBUG] Tentativo di caricamento highscore da: " << filename << std::endl;

    try {
        std::string jsonContent = parseJsonFile(filename);
        // std::cout << "[DEBUG] Contenuto JSON letto: " << jsonContent.length() << " caratteri" << std::endl;
        
        if (jsonContent.empty()) {
            // std::cout << "[DEBUG] File JSON vuoto o inesistente, creando record predefiniti" << std::endl;
            // File non esiste o è vuoto, inizializza con record predefiniti
            m_scores.push_back({"PAC-MAN", 50000, getCurrentDate()});
            m_scores.push_back({"BLINKY", 40000, getCurrentDate()});
            m_scores.push_back({"PINKY", 30000, getCurrentDate()});
            m_scores.push_back({"INKY", 20000, getCurrentDate()});
            m_scores.push_back({"CLYDE", 10000, getCurrentDate()});
            saveToFile(filename); // Salva i record predefiniti
            // std::cout << "[DEBUG] Record predefiniti salvati: " << m_scores.size() << std::endl;
            return;
        }

        // Parse manuale del JSON (semplificato per evitare dipendenze esterne)
        std::istringstream stream(jsonContent);
        std::string line;
        
        int recordsFound = 0;
        HighScoreEntry currentEntry;
        bool inRecord = false;
        bool inHighscoresArray = false;
        int fieldsFound = 0;
        
        while (std::getline(stream, line)) {
            // Rileva inizio array highscores
            if (line.find("\"highscores\"") != std::string::npos) {
                inHighscoresArray = true;
                // std::cout << "[DEBUG] Trovato array highscores" << std::endl;
                continue;
            }
            
            // Rileva inizio di un record (solo se siamo nell'array highscores)
            if (line.find("{") != std::string::npos && !inRecord && inHighscoresArray) {
                inRecord = true;
                fieldsFound = 0;
                currentEntry = HighScoreEntry(); // Reset
                // std::cout << "[DEBUG] Inizio parsing record" << std::endl;
                continue;
            }
            
            // Rileva fine di un record
            if (line.find("}") != std::string::npos && inRecord) {
                if (fieldsFound == 3) { // Tutti i campi trovati
                    m_scores.push_back(currentEntry);
                    recordsFound++;
                    // std::cout << "[DEBUG] Record trovato: " << currentEntry.playerName << " - " << currentEntry.score << " - " << currentEntry.date << std::endl;
                } else {
                    // std::cout << "[DEBUG] Record incompleto, campi trovati: " << fieldsFound << std::endl;
                }
                inRecord = false;
                continue;
            }
            
            if (inRecord) {
                // Cerca campo nome
                size_t namePos = line.find("\"name\":");
                if (namePos != std::string::npos) {
                    size_t nameStart = line.find("\"", namePos + 7) + 1;
                    size_t nameEnd = line.find("\"", nameStart);
                    if (nameStart != std::string::npos && nameEnd != std::string::npos) {
                        currentEntry.playerName = line.substr(nameStart, nameEnd - nameStart);
                        fieldsFound++;
                        // std::cout << "[DEBUG] Nome trovato: " << currentEntry.playerName << std::endl;
                    }
                }
                
                // Cerca campo score
                size_t scorePos = line.find("\"score\":");
                if (scorePos != std::string::npos) {
                    size_t scoreStart = line.find(":", scorePos) + 1;
                    size_t scoreEnd = line.find(",", scoreStart);
                    if (scoreEnd == std::string::npos) scoreEnd = line.length();
                    std::string scoreStr = line.substr(scoreStart, scoreEnd - scoreStart);
                    // Rimuovi spazi e virgole
                    scoreStr.erase(std::remove_if(scoreStr.begin(), scoreStr.end(), 
                        [](char c) { return std::isspace(c) || c == ','; }), scoreStr.end());
                    if (!scoreStr.empty()) {
                        currentEntry.score = static_cast<unsigned int>(std::stoul(scoreStr));
                        fieldsFound++;
                        // std::cout << "[DEBUG] Score trovato: " << currentEntry.score << std::endl;
                    }
                }
                
                // Cerca campo data
                size_t datePos = line.find("\"date\":");
                if (datePos != std::string::npos) {
                    size_t dateStart = line.find("\"", datePos + 6) + 1;
                    size_t dateEnd = line.find("\"", dateStart);
                    if (dateStart != std::string::npos && dateEnd != std::string::npos) {
                        currentEntry.date = line.substr(dateStart, dateEnd - dateStart);
                        fieldsFound++;
                        // std::cout << "[DEBUG] Data trovata: " << currentEntry.date << std::endl;
                    }
                }
            }
        }
        
        // std::cout << "[DEBUG] Totale record trovati: " << recordsFound << std::endl;
        sortScores();
    }
    catch (const std::exception& e) {
        // In caso di errore, inizializza con record predefiniti
        std::cout << "Errore caricamento highscores: " << e.what() << std::endl;
        m_scores.clear();
        m_scores.push_back({"PAC-MAN", 50000, getCurrentDate()});
        m_scores.push_back({"BLINKY", 40000, getCurrentDate()});
        m_scores.push_back({"PINKY", 30000, getCurrentDate()});
        m_scores.push_back({"INKY", 20000, getCurrentDate()});
        m_scores.push_back({"CLYDE", 10000, getCurrentDate()});
    }
}

// Salva i record nel file JSON
void HighScore::saveToFile(const std::string& filename) const {
    // std::cout << "[DEBUG] Tentativo di salvataggio highscore in: " << filename << std::endl;
    // std::cout << "[DEBUG] Numero di record da salvare: " << m_scores.size() << std::endl;
    
    try {
        std::ostringstream json;
        json << "{\n";
        json << "  \"highscores\": [\n";
        
        for (size_t i = 0; i < m_scores.size(); ++i) {
            json << "    {\n";
            json << "      \"name\": \"" << m_scores[i].playerName << "\",\n";
            json << "      \"score\": " << m_scores[i].score << ",\n";
            json << "      \"date\": \"" << m_scores[i].date << "\"\n";
            json << "    }";
            if (i < m_scores.size() - 1) {
                json << ",";
            }
            json << "\n";
        }
        
        json << "  ]\n";
        json << "}\n";
        
        // std::cout << "[DEBUG] JSON generato: " << json.str() << std::endl;
        
        writeJsonFile(filename, json.str());
        // std::cout << "[DEBUG] File salvato con successo" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "Errore salvataggio highscores: " << e.what() << std::endl;
    }
}

// Controlla se un punteggio è tra i migliori
bool HighScore::isHighScore(unsigned int score) const {
    if (m_scores.size() < MAX_SCORES) {
        return true; // Ci sono ancora posti liberi
    }
    
    // Controlla se il punteggio è maggiore del più basso nella lista
    return score > m_scores.back().score;
}

// Aggiunge un nuovo record
void HighScore::addScore(const std::string& playerName, unsigned int score) {
    HighScoreEntry newEntry;
    newEntry.playerName = playerName.substr(0, 10); // Limita a 10 caratteri
    newEntry.score = score;
    newEntry.date = getCurrentDate();
    
    m_scores.push_back(newEntry);
    sortScores();
    
    // Mantieni solo i migliori MAX_SCORES record
    if (m_scores.size() > MAX_SCORES) {
        m_scores.resize(MAX_SCORES);
    }
    
    saveToFile(m_filename);
}

// Disegna la schermata dei record
void HighScore::draw(sf::RenderTarget& target, const sf::Vector2u& windowSize) const {
    // Titolo
    sf::Text titleText(m_font, "HALL OF FAME", 36);
    titleText.setFillColor(sf::Color::Yellow);
    titleText.setOutlineColor(sf::Color::Blue);
    titleText.setOutlineThickness(2);
    
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition(sf::Vector2f((windowSize.x - titleBounds.size.x) / 2.0f, windowSize.y * 0.1f));
    target.draw(titleText);
    
    // Intestazioni
    sf::Text headerText(m_font, "POS   NOME        PUNTEGGIO    DATA", 20);
    headerText.setFillColor(sf::Color::Cyan);
    headerText.setPosition(sf::Vector2f(windowSize.x * 0.05f, windowSize.y * 0.25f));
    target.draw(headerText);
    
    // Lista dei record
    float startY = windowSize.y * 0.32f;
    float lineHeight = 35.0f;
    
    for (size_t i = 0; i < m_scores.size() && i < MAX_SCORES; ++i) {
        std::ostringstream line;
        line << std::setw(2) << (i + 1) << ".  ";
        line << std::left << std::setw(10) << m_scores[i].playerName << "  ";
        line << std::right << std::setw(8) << m_scores[i].score << "    ";
        line << m_scores[i].date;
        
        sf::Text scoreText(m_font, line.str(), 18);
        
        // Colore diverso per il primo posto
        if (i == 0) {
            scoreText.setFillColor(sf::Color(255, 215, 0)); // Oro
        } else if (i == 1) {
            scoreText.setFillColor(sf::Color(192, 192, 192)); // Argento
        } else if (i == 2) {
            scoreText.setFillColor(sf::Color(205, 127, 50)); // Bronzo
        } else {
            scoreText.setFillColor(sf::Color::White);
        }
        
        scoreText.setPosition(sf::Vector2f(windowSize.x * 0.05f, startY + i * lineHeight));
        target.draw(scoreText);
    }
    
    // Istruzioni per tornare al menu
    sf::Text backText(m_font, "Premi ESC per tornare al menu", 16);
    backText.setFillColor(sf::Color::Green);
    sf::FloatRect backBounds = backText.getLocalBounds();
    backText.setPosition(sf::Vector2f((windowSize.x - backBounds.size.x) / 2.0f, windowSize.y * 0.85f));
    target.draw(backText);
}

// Ottiene il punteggio più alto
unsigned int HighScore::getTopScore() const {
    if (m_scores.empty()) {
        return 0;
    }
    return m_scores[0].score;
}

// Metodi di utilità privati

std::string HighScore::getCurrentDate() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << tm.tm_mday << "/"
        << std::setfill('0') << std::setw(2) << (tm.tm_mon + 1) << "/"
        << (tm.tm_year + 1900);
    return oss.str();
}

void HighScore::sortScores() {
    std::sort(m_scores.begin(), m_scores.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
        return a.score > b.score; // Ordine decrescente
    });
}

std::string HighScore::parseJsonFile(const std::string& filename) const {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return ""; // File non esiste
    }
    
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void HighScore::writeJsonFile(const std::string& filename, const std::string& jsonContent) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot write to file: " + filename);
    }
    
    file << jsonContent;
    file.close();
}

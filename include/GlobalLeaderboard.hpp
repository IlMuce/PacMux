#pragma once

#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <future>
#include <memory>
#include <ctime>
#include <iostream>
#include <cstdlib>
#include <deque>
#include "HighScore.hpp"

class GlobalLeaderboard {
public:
    struct GlobalEntry {
        std::string playerName;
        unsigned int score;
        std::time_t timestamp;  // Cambio da date a timestamp per JSONBin
        std::string date;       // Data formattata per visualizzazione
        std::string country;    // opzionale, basato su IP
    };

    enum class Status {
        Idle,
        Uploading,
        Downloading,
        Success,
        Error
    };

    explicit GlobalLeaderboard(const std::string& fontFile);
    
    // Upload del punteggio a GitHub Raw Files (asincrono)
    void uploadScore(const std::string& playerName, unsigned int score);
    
    // Download della leaderboard globale da GitHub Raw Files (asincrono)
    void downloadLeaderboard();
    
    // Force refresh della leaderboard (utile dopo upload)
    void forceRefresh() { 
        if (m_status != Status::Downloading && m_status != Status::Uploading) {
            downloadLeaderboard(); 
        }
    }
    
    // Aggiorna stato delle operazioni asincrone
    void update();
    
    // Disegna leaderboard globale
    void draw(sf::RenderTarget& target, const sf::Vector2u& windowSize) const;

    // Navigazione/scroll
    void scroll(int delta);
    void scrollToStart();
    void scrollToEnd(const sf::Vector2u& windowSize);
    
    // Getter per stato
    Status getStatus() const { return m_status; }
    const std::string& getErrorMessage() const { return m_errorMessage; }
    bool hasGlobalData() const { return !m_globalScores.empty(); }
    
    // Combina scores locali e globali per visualizzazione
    void drawCombined(sf::RenderTarget& target, const sf::Vector2u& windowSize, const HighScore& localHighScore) const;

private:
    // GitHub Raw Files configuration 
    static const std::string JSONBIN_URL;        // "http://raw.githubusercontent.com/..."
    static const std::string JSONBIN_API_KEY;    // Token per scrittura via API
    
    std::vector<GlobalEntry> m_globalScores;
    sf::Font m_font;
    Status m_status;
    std::string m_errorMessage;
    std::string m_apiToken;  // Token caricato da variabile d'ambiente
    
    // Per operazioni asincrone
    std::future<bool> m_uploadFuture;
    std::future<bool> m_downloadFuture;
    std::size_t m_firstVisibleIndex = 0; // indice del primo record visibile per lo scroll
    std::deque<GlobalEntry> m_pendingUploads; // coda di upload in attesa quando c'è già un upload in corso
    
    // HTTP helpers per GitHub Gist
    std::string httpGetGist();
    bool httpUpdateGist(const std::string& jsonData);
    std::string getFallbackData();
    std::string createScoresJson(const std::vector<GlobalEntry>& scores);
    bool parseGlobalScores(const std::string& jsonResponse, std::vector<GlobalEntry>& scores);
    std::string getCurrentDate() const;
    
    // GitHub Raw Files helpers
    std::string extractGistIdFromUrl();
    std::string extractRepoInfo();
    std::string extractFileContent(const std::string& response, const std::string& filename);
    std::string createGitHubUpdatePayload(const std::string& jsonData);
    std::string createGistUpdatePayload(const std::string& jsonData);
    std::string base64Encode(const std::string& data);
    std::string getCurrentFileSha();

    // Calcola quanti record possono stare a schermo
    std::size_t computeVisibleCount(const sf::Vector2u& windowSize) const;

    // Avvia un upload asincrono per una entry già composta (senza accodarlo)
    void startUpload(const GlobalEntry& entry);
};

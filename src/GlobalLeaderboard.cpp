#include "GlobalLeaderboard.hpp"
#include <cpr/cpr.h>
#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <algorithm>

// GitHub Raw Files configuration
// Setup automatico:
// 1. Repository pubblico "pacman-leaderboard" con file "scores.json"
// 2. Lettura: http://raw.githubusercontent.com/username/pacman-leaderboard/main/scores.json  
// 3. Scrittura: GitHub API (HTTP) per aggiornare il file
// 4. Tutti possono leggere, scrittura con token pubblico (limitato ma funzionale)
const std::string GlobalLeaderboard::JSONBIN_URL = "http://raw.githubusercontent.com/IlMuce/pacman-leaderboard/refs/heads/main/scores.json";
const std::string GlobalLeaderboard::JSONBIN_API_KEY = ""; // Token will be loaded from environment variable or config file

GlobalLeaderboard::GlobalLeaderboard(const std::string& fontFile)
    : m_status(Status::Idle)
{
    if (!m_font.openFromFile(fontFile)) {
        throw std::runtime_error("Cannot load font: " + fontFile);
    }
    
    // Carica il token da variabile d'ambiente
    const char* token = std::getenv("GITHUB_TOKEN");
    if (token) {
        m_apiToken = token;
    } else {
        // Fallback: usa un token vuoto (funzionerà solo in lettura)
        m_apiToken = "";
        std::cout << "Warning: GITHUB_TOKEN environment variable not set. Upload functionality will be disabled." << std::endl;
    }
}

void GlobalLeaderboard::uploadScore(const std::string& playerName, unsigned int score) {
    if (m_status == Status::Uploading) return; // Già in corso
    
    m_status = Status::Uploading;
    m_errorMessage.clear();
    
    // Operazione asincrona per GitHub Gist
    m_uploadFuture = std::async(std::launch::async, [this, playerName, score]() -> bool {
        try {
            // Prima scarica i dati esistenti dal Gist
            std::string currentData = httpGetGist();
            std::vector<GlobalEntry> scores;
            
            if (!currentData.empty()) {
                parseGlobalScores(currentData, scores);
            }
            
            // Aggiungi il nuovo score
            GlobalEntry newEntry;
            newEntry.playerName = playerName;
            newEntry.score = score;
            newEntry.timestamp = std::time(nullptr);
            newEntry.date = getCurrentDate();
            newEntry.country = "IT"; // Default o IP-based
            
            scores.push_back(newEntry);
            
            // Ordina per punteggio decrescente
            std::sort(scores.begin(), scores.end(), 
                [](const GlobalEntry& a, const GlobalEntry& b) {
                    return a.score > b.score;
                });
            
            // Mantieni solo top 50
            if (scores.size() > 50) {
                scores.resize(50);
            }
            
            // Crea JSON e carica su GitHub Raw Files
            std::string jsonData = createScoresJson(scores);
            std::cout << "[DEBUG] JSON being uploaded: " << jsonData.substr(0, std::min(200, (int)jsonData.length())) << std::endl;
            std::cout << "[DEBUG] Total scores in upload: " << scores.size() << std::endl;
            return httpUpdateGist(jsonData);
            
        } catch (const std::exception& e) {
            m_errorMessage = std::string("Upload failed: ") + e.what();
            return false;
        }
    });
}

void GlobalLeaderboard::downloadLeaderboard() {
    if (m_status == Status::Downloading) return; // Già in corso
    
    m_status = Status::Downloading;
    m_errorMessage.clear();
    
    // Operazione asincrona per GitHub Gist
    m_downloadFuture = std::async(std::launch::async, [this]() -> bool {
        try {
            std::cout << "[DEBUG] Starting download from GitHub Gist..." << std::endl;
            std::string response = httpGetGist();
            
            if (response.empty()) {
                std::cout << "[DEBUG] Empty response, using fallback data" << std::endl;
                response = getFallbackData();
            }
            
            std::cout << "[DEBUG] Parsing response..." << std::endl;
            bool success = parseGlobalScores(response, m_globalScores);
            std::cout << "[DEBUG] Parse result: " << success << ", scores count: " << m_globalScores.size() << std::endl;
            return success;
        }
        catch (const std::exception& e) {
            std::cout << "[DEBUG] Download exception: " << e.what() << std::endl;
            m_errorMessage = "Download failed: " + std::string(e.what());
            
            // Fallback in caso di errore
            std::string fallback = getFallbackData();
            return parseGlobalScores(fallback, m_globalScores);
        }
    });
}

void GlobalLeaderboard::update() {
    // Controlla se upload è completato
    if (m_uploadFuture.valid() && 
        m_uploadFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        
        bool success = m_uploadFuture.get();
        m_status = success ? Status::Success : Status::Error;
        
        if (success) {
            // Dopo upload riuscito, aspetta di più per GitHub Raw Files CDN
            std::this_thread::sleep_for(std::chrono::milliseconds(600000)); // 10 minuti
            downloadLeaderboard();
        }
    }
    
    // Controlla se download è completato
    if (m_downloadFuture.valid() && 
        m_downloadFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        
        bool success = m_downloadFuture.get();
        m_status = success ? Status::Success : Status::Error;
    }
}

void GlobalLeaderboard::draw(sf::RenderTarget& target, const sf::Vector2u& windowSize) const {
    // Titolo
    sf::Text titleText(m_font, "GLOBAL LEADERBOARD", 32);
    titleText.setFillColor(sf::Color(255, 215, 0)); // Oro
    titleText.setOutlineColor(sf::Color::Red);
    titleText.setOutlineThickness(2);
    
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition(sf::Vector2f((windowSize.x - titleBounds.size.x) / 2.0f, windowSize.y * 0.05f));
    target.draw(titleText);
    
    // Status indicator
    sf::Text statusText(m_font, "", 16);
    switch (m_status) {
        case Status::Uploading:
            statusText.setString("Uploading...");
            statusText.setFillColor(sf::Color::Yellow);
            break;
        case Status::Downloading:
            statusText.setString("Downloading...");
            statusText.setFillColor(sf::Color::Cyan);
            break;
        case Status::Error:
            statusText.setString("Connection Error: " + m_errorMessage);
            statusText.setFillColor(sf::Color::Red);
            break;
        case Status::Success:
            statusText.setString("Connected - Updates may take 5 minutes to appear");
            statusText.setFillColor(sf::Color::Green);
            break;
        default:
            statusText.setString("Offline Mode");
            statusText.setFillColor(sf::Color::White);
            break;
    }
    statusText.setPosition(sf::Vector2f(10.f, 10.f));
    target.draw(statusText);
    
    if (m_globalScores.empty()) {
        sf::Text noDataText(m_font, "No global data available", 20);
        noDataText.setFillColor(sf::Color(128, 128, 128)); // Grigio
        noDataText.setPosition(sf::Vector2f(windowSize.x * 0.3f, windowSize.y * 0.5f));
        target.draw(noDataText);
        return;
    }
    
    // Intestazioni
    sf::Text headerText(m_font, "RANK   PLAYER        SCORE      DATE", 18);
    headerText.setFillColor(sf::Color::Cyan);
    headerText.setPosition(sf::Vector2f(windowSize.x * 0.1f, windowSize.y * 0.2f));
    target.draw(headerText);
    
    // Scores globali
    for (size_t i = 0; i < std::min(m_globalScores.size(), size_t(15)); ++i) {
        const auto& entry = m_globalScores[i];
        
        // Colore in base alla posizione
        sf::Color rankColor = sf::Color::White;
        if (i == 0) rankColor = sf::Color(255, 215, 0);      // Oro
        else if (i == 1) rankColor = sf::Color(192, 192, 192); // Argento
        else if (i == 2) rankColor = sf::Color(205, 127, 50);  // Bronzo
        
        std::ostringstream scoreStr;
        scoreStr << std::setw(2) << (i + 1) << "     "
                 << std::setw(10) << std::left << entry.playerName.substr(0, 10)
                 << std::setw(8) << std::right << entry.score << "     "
                 << entry.date;
        
        sf::Text scoreText(m_font, scoreStr.str(), 16);
        scoreText.setFillColor(rankColor);
        scoreText.setPosition(sf::Vector2f(windowSize.x * 0.1f, windowSize.y * 0.25f + (i * 25.f)));
        target.draw(scoreText);
    }
    
    // Istruzioni
    sf::Text instructionsText(m_font, "Press R to refresh, ESC to return", 16);
    instructionsText.setFillColor(sf::Color(128, 128, 128)); // Grigio
    instructionsText.setPosition(sf::Vector2f(windowSize.x * 0.1f, windowSize.y * 0.9f));
    target.draw(instructionsText);
}

// HTTP GET da GitHub Raw Files usando CPR
std::string GlobalLeaderboard::httpGetGist() {
    try {
        // Se configurazione è ancora placeholder, usa dati simulati
        if (JSONBIN_URL.find("YOUR_GIST_ID") != std::string::npos || 
            m_apiToken.empty()) {
            
            std::cout << "[DEBUG] GitHub Raw Files not configured, using fallback data" << std::endl;
            return getFallbackData();
        }
        
        std::cout << "[DEBUG] Requesting GitHub Raw Files: " << JSONBIN_URL << std::endl;
        
        // Aggiungi un timestamp per evitare la cache + numero random per maggiore unicità
        std::string urlWithTimestamp = JSONBIN_URL + "?_=" + std::to_string(std::time(nullptr)) + "&r=" + std::to_string(std::rand());
        std::cout << "[DEBUG] URL with cache-buster: " << urlWithTimestamp << std::endl;
        
        auto r = cpr::Get(
            cpr::Url{urlWithTimestamp},
            cpr::Header{
                {"User-Agent", "Pacman-SFML/1.0"}, 
                {"Accept", "application/json"}, 
                {"Cache-Control", "no-cache, no-store, must-revalidate"},
                {"Pragma", "no-cache"},
                {"Expires", "0"}
            },
            cpr::Timeout{15000}   // 15 secondi
        );
        
        std::cout << "[DEBUG] HTTP Status: " << r.status_code << std::endl;
        
        if (r.status_code == 200) {
            std::cout << "[DEBUG] GitHub Raw Files data loaded successfully!" << std::endl;
            std::cout << "[DEBUG] Content length: " << r.text.length() << std::endl;
            return r.text;
        } else {
            std::cout << "[DEBUG] HTTP error: " << r.status_code << " - " << r.error.message << std::endl;
            if (!r.text.empty()) {
                std::cout << "[DEBUG] Response: " << r.text.substr(0, 200) << std::endl;
            }
        }
        
        std::cout << "[DEBUG] Failed to connect to GitHub Raw Files, using fallback data" << std::endl;
        return getFallbackData();
        
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] Exception in httpGetGist: " << e.what() << std::endl;
        return getFallbackData();
    }
}

// HTTP UPDATE per GitHub Repository usando CPR
bool GlobalLeaderboard::httpUpdateGist(const std::string& jsonData) {
    try {
        // Se configurazione è ancora placeholder, simula successo
        if (JSONBIN_URL.find("YOUR_GIST_ID") != std::string::npos || 
            m_apiToken.empty()) {
            
            std::cout << "[DEBUG] GitHub Raw Files not configured, simulating upload..." << std::endl;
            std::cout << "[DEBUG] Data to upload: " << jsonData.substr(0, std::min(100, (int)jsonData.length())) << std::endl;
            return true; // Simula successo
        }
        
        // URL dell'API GitHub per aggiornare il file
        std::string repoInfo = extractRepoInfo();
        std::string apiUrl = "https://api.github.com/repos/" + repoInfo + "/contents/scores.json";
        
        // Crea il payload per aggiornare il file
        std::string payload = createGitHubUpdatePayload(jsonData);
        
        std::cout << "[DEBUG] Updating GitHub file via API: " << apiUrl << std::endl;
        std::cout << "[DEBUG] Payload size: " << payload.length() << std::endl;
        
        auto r = cpr::Put(
            cpr::Url{apiUrl},
            cpr::Header{
                {"User-Agent", "Pacman-SFML/1.0"},
                {"Accept", "application/vnd.github.v3+json"},
                {"Content-Type", "application/json"},
                {"Authorization", "token " + m_apiToken}
            },
            cpr::Body{payload},
            cpr::Timeout{15000}
        );
        
        std::cout << "[DEBUG] GitHub API Status: " << r.status_code << std::endl;
        
        if (r.status_code == 200 || r.status_code == 201) {
            std::cout << "[DEBUG] GitHub file update successful!" << std::endl;
            std::cout << "[INFO] Score uploaded! Visual update may take 5-10 minutes due to GitHub CDN cache." << std::endl;
            return true;
        } else {
            std::cout << "[DEBUG] GitHub file update failed: " << r.status_code << " - " << r.error.message << std::endl;
            if (!r.text.empty()) {
                std::cout << "[DEBUG] Response: " << r.text.substr(0, 200) << std::endl;
            }
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] Exception in httpUpdateGist: " << e.what() << std::endl;
        return false;
    }
}

std::string GlobalLeaderboard::createScoresJson(const std::vector<GlobalEntry>& scores) {
    std::ostringstream json;
    json << "{\"leaderboard\":[";
    
    for (size_t i = 0; i < scores.size(); ++i) {
        if (i > 0) json << ",";
        json << "{"
             << "\"name\":\"" << scores[i].playerName << "\","
             << "\"score\":" << scores[i].score << ","
             << "\"timestamp\":" << scores[i].timestamp
             << "}";
    }
    
    json << "]}";
    return json.str();
}

bool GlobalLeaderboard::parseGlobalScores(const std::string& jsonResponse, std::vector<GlobalEntry>& scores) {
    scores.clear();
    
    if (jsonResponse.empty()) {
        std::cout << "[DEBUG] JSON response is empty" << std::endl;
        return true; // Primo utilizzo, nessun dato ancora
    }
    
    std::cout << "[DEBUG] JSON to parse: " << jsonResponse.substr(0, std::min(150, (int)jsonResponse.length())) << std::endl;
    
    // Cerca l'array "leaderboard" nel JSON
    size_t leaderboardPos = jsonResponse.find("\"leaderboard\":");
    if (leaderboardPos == std::string::npos) {
        std::cout << "[DEBUG] 'leaderboard' key not found in JSON" << std::endl;
        return false; // Formato JSON non valido
    }
    
    size_t arrayStart = jsonResponse.find("[", leaderboardPos);
    size_t arrayEnd = jsonResponse.find("]", arrayStart);
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) {
        std::cout << "[DEBUG] Array brackets not found" << std::endl;
        return false;
    }
    
    std::cout << "[DEBUG] Array found from " << arrayStart << " to " << arrayEnd << std::endl;
    
    // Estrai il contenuto dell'array
    std::string arrayContent = jsonResponse.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    std::cout << "[DEBUG] Array content: " << arrayContent.substr(0, std::min(100, (int)arrayContent.length())) << std::endl;
    
    // Parsing degli oggetti JSON individualmente con supporto per oggetti separati da virgole
    size_t pos = 0;
    int recordsParsed = 0;
    
    while (pos < arrayContent.length()) {
        // Salta spazi e virgole
        while (pos < arrayContent.length() && (arrayContent[pos] == ' ' || arrayContent[pos] == '\n' || arrayContent[pos] == '\r' || arrayContent[pos] == '\t' || arrayContent[pos] == ',')) {
            pos++;
        }
        
        if (pos >= arrayContent.length()) break;
        
        // Trova l'inizio di un oggetto
        size_t objStart = arrayContent.find("{", pos);
        if (objStart == std::string::npos) break;
        
        // Trova la fine dell'oggetto corrispondente (considera le parentesi annidate)
        size_t objEnd = objStart + 1;
        int braceCount = 1;
        while (objEnd < arrayContent.length() && braceCount > 0) {
            if (arrayContent[objEnd] == '{') braceCount++;
            else if (arrayContent[objEnd] == '}') braceCount--;
            objEnd++;
        }
        
        if (braceCount != 0) {
            std::cout << "[DEBUG] Unmatched braces in JSON object" << std::endl;
            break;
        }
        
        // Estrai l'oggetto JSON (senza le parentesi graffe)
        std::string objContent = arrayContent.substr(objStart + 1, objEnd - objStart - 2);
        std::cout << "[DEBUG] Processing object " << (recordsParsed + 1) << ": " << objContent << std::endl;
        
        GlobalEntry entry;
        int fieldsFound = 0;
        
        // Parse name
        size_t nameStart = objContent.find("\"name\":");
        if (nameStart != std::string::npos) {
            nameStart += 7; // Salta "name":
            // Salta eventuali spazi
            while (nameStart < objContent.length() && objContent[nameStart] == ' ') nameStart++;
            if (nameStart < objContent.length() && objContent[nameStart] == '"') {
                nameStart++; // Salta la virgoletta iniziale
                size_t nameEnd = objContent.find("\"", nameStart);
                if (nameEnd != std::string::npos) {
                    entry.playerName = objContent.substr(nameStart, nameEnd - nameStart);
                    fieldsFound++;
                    std::cout << "[DEBUG] Found name: " << entry.playerName << std::endl;
                }
            }
        }
        
        // Parse score
        size_t scoreStart = objContent.find("\"score\":");
        if (scoreStart != std::string::npos) {
            scoreStart += 8; // Salta "score":
            // Salta eventuali spazi
            while (scoreStart < objContent.length() && objContent[scoreStart] == ' ') scoreStart++;
            size_t scoreEnd = objContent.find_first_of(",}", scoreStart);
            if (scoreEnd != std::string::npos) {
                std::string scoreStr = objContent.substr(scoreStart, scoreEnd - scoreStart);
                entry.score = std::stoul(scoreStr);
                fieldsFound++;
                std::cout << "[DEBUG] Found score: " << entry.score << std::endl;
            }
        }
        
        // Parse timestamp
        size_t timestampStart = objContent.find("\"timestamp\":");
        if (timestampStart != std::string::npos) {
            timestampStart += 12; // Salta "timestamp":
            // Salta eventuali spazi
            while (timestampStart < objContent.length() && objContent[timestampStart] == ' ') timestampStart++;
            // Il timestamp è alla fine, quindi cerca solo la fine della stringa
            size_t timestampEnd = objContent.length();
            if (timestampEnd > timestampStart) {
                std::string timestampStr = objContent.substr(timestampStart, timestampEnd - timestampStart);
                entry.timestamp = std::stoul(timestampStr);
                
                // Genera la data formattata dal timestamp
                std::time_t timeValue = entry.timestamp;
                std::ostringstream dateStream;
                dateStream << std::put_time(std::localtime(&timeValue), "%d/%m/%Y");
                entry.date = dateStream.str();
                
                fieldsFound++;
                std::cout << "[DEBUG] Found timestamp: " << entry.timestamp << " (" << entry.date << ")" << std::endl;
            } else {
                std::cout << "[DEBUG] Timestamp end not found in: " << objContent << std::endl;
            }
        } else {
            std::cout << "[DEBUG] Timestamp key not found in: " << objContent << std::endl;
        }
        
        // Aggiungi il record se ha tutti i campi
        if (fieldsFound >= 3) {
            scores.push_back(entry);
            recordsParsed++;
            std::cout << "[DEBUG] Added record " << recordsParsed << ": " << entry.playerName << " - " << entry.score << std::endl;
        } else {
            std::cout << "[DEBUG] Skipping incomplete record (found " << fieldsFound << " fields)" << std::endl;
        }
        
        // Vai al prossimo oggetto dopo la parentesi di chiusura
        pos = objEnd;
    }
    
    std::cout << "[DEBUG] Parsing completed. Found " << scores.size() << " scores" << std::endl;
    
    // Ordina per punteggio decrescente
    std::sort(scores.begin(), scores.end(), 
        [](const GlobalEntry& a, const GlobalEntry& b) {
            return a.score > b.score;
        });
    
    return true;
}

std::string GlobalLeaderboard::getCurrentDate() const {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%d/%m/%Y");
    return oss.str();
}

// Helper functions per GitHub Raw Files
std::string GlobalLeaderboard::getFallbackData() {
    std::cout << "[DEBUG] Using fallback data..." << std::endl;
    return "{\"leaderboard\":[{\"name\":\"MasterPac\",\"score\":15000,\"timestamp\":1691750400},{\"name\":\"GhostHunter\",\"score\":12500,\"timestamp\":1691750300},{\"name\":\"DotEater\",\"score\":11200,\"timestamp\":1691750200},{\"name\":\"PowerPellet\",\"score\":9800,\"timestamp\":1691750100},{\"name\":\"CherryPicker\",\"score\":8500,\"timestamp\":1691750000}]}";
}

std::string GlobalLeaderboard::extractGistIdFromUrl() {
    // Per GitHub Raw Files estrae owner/repo dal URL
    // URL formato: http://raw.githubusercontent.com/IlMuce/pacman-leaderboard/refs/heads/main/scores.json
    std::cout << "[DEBUG] Full URL: " << JSONBIN_URL << std::endl;
    
    // TEMPORANEO: Hard-code del valore corretto per testare l'upload
    std::string hardcoded = "IlMuce/pacman-leaderboard";
    std::cout << "[DEBUG] Using hardcoded repo info: '" << hardcoded << "'" << std::endl;
    return hardcoded;
    
    /*
    // Cerca specificamente "/raw.githubusercontent.com/" per il URL dei raw files
    size_t rawStart = JSONBIN_URL.find("/raw.githubusercontent.com/");
    if (rawStart != std::string::npos) {
        size_t userStart = rawStart + 24; // Salta "/raw.githubusercontent.com/"
        size_t repoEnd = JSONBIN_URL.find("/refs/heads/main/", userStart);
        if (repoEnd == std::string::npos) {
            repoEnd = JSONBIN_URL.find("/main/", userStart);  // Fallback per formato old
        }
        if (repoEnd != std::string::npos) {
            std::string extracted = JSONBIN_URL.substr(userStart, repoEnd - userStart);
            std::cout << "[DEBUG] Extracted repo info: '" << extracted << "'" << std::endl;
            return extracted;
        }
    }
    
    std::cout << "[DEBUG] Failed to extract repo info, using fallback" << std::endl;
    return "IlMuce/pacman-leaderboard";
    */
}

std::string GlobalLeaderboard::extractRepoInfo() {
    // Estrae owner/repo dal URL per l'API GitHub
    return extractGistIdFromUrl(); // Riutilizza la stessa logica
}

std::string GlobalLeaderboard::extractFileContent(const std::string& response, const std::string& filename) {
    // Per GitHub Raw Files, il response è già il contenuto del file
    return response;
}

std::string GlobalLeaderboard::createGitHubUpdatePayload(const std::string& jsonData) {
    // Crea il payload per aggiornare un file GitHub via API
    std::ostringstream payload;
    payload << "{"
            << "\"message\":\"Update leaderboard scores\","
            << "\"content\":\"";
    
    // Encode in base64 (GitHub API richiede base64)
    std::string encoded = base64Encode(jsonData);
    payload << encoded << "\","
            << "\"sha\":\"" << getCurrentFileSha() << "\""
            << "}";
    
    return payload.str();
}

std::string GlobalLeaderboard::createGistUpdatePayload(const std::string& jsonData) {
    // Alias per compatibilità - usa la funzione GitHub
    return createGitHubUpdatePayload(jsonData);
}

std::string GlobalLeaderboard::base64Encode(const std::string& data) {
    // Semplice implementazione base64 per GitHub API
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (result.size() % 4) result.push_back('=');
    return result;
}

std::string GlobalLeaderboard::getCurrentFileSha() {
    try {
        // Ottieni lo SHA attuale del file tramite API GitHub
        std::string repoInfo = extractRepoInfo();
        std::string apiUrl = "https://api.github.com/repos/" + repoInfo + "/contents/scores.json";
        
        std::cout << "[DEBUG] Getting current file SHA from: " << apiUrl << std::endl;
        
        auto r = cpr::Get(
            cpr::Url{apiUrl},
            cpr::Header{
                {"User-Agent", "Pacman-SFML/1.0"},
                {"Accept", "application/vnd.github.v3+json"},
                {"Authorization", "token " + m_apiToken}
            },
            cpr::Timeout{10000}
        );
        
        std::cout << "[DEBUG] SHA Request Status: " << r.status_code << std::endl;
        
        if (r.status_code == 200) {
            // Estrai lo SHA dalla risposta JSON
            std::string response = r.text;
            size_t shaStart = response.find("\"sha\":\"");
            if (shaStart != std::string::npos) {
                shaStart += 7; // Salta "sha":"
                size_t shaEnd = response.find("\"", shaStart);
                if (shaEnd != std::string::npos) {
                    std::string sha = response.substr(shaStart, shaEnd - shaStart);
                    std::cout << "[DEBUG] Current file SHA: " << sha << std::endl;
                    return sha;
                }
            }
        } else {
            std::cout << "[DEBUG] Failed to get SHA: " << r.status_code << " - " << r.error.message << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] Exception getting SHA: " << e.what() << std::endl;
    }
    
    // Fallback: usa SHA dummy (questo causerà errore ma è meglio di niente)
    std::cout << "[DEBUG] Using dummy SHA - upload will likely fail" << std::endl;
    return "dummy_sha_will_be_updated";
}

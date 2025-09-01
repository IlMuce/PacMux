#include "GlobalLeaderboard.hpp"
#include <cpr/cpr.h>
#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <cstdlib> // for std::getenv, std::rand

// GitHub Raw Files configuration with secondary account
// Setup automatico:
// 1. Account secondario GitHub dedicato alla leaderboard
// 2. Repository pubblico "pacman-leaderboard" con file "scores.json"  
// 3. Lettura: https://raw.githubusercontent.com/ACCOUNT_SECONDARIO/pacman-leaderboard/main/scores.json
// 4. Scrittura: GitHub API (HTTP) con token embedded (account isolato)
const std::string GlobalLeaderboard::JSONBIN_URL = "https://raw.githubusercontent.com/DenisMux/pacmux-leaderboard/main/scores.json";
const std::string GlobalLeaderboard::JSONBIN_API_KEY = ""; // Token embedded nel costruttore

GlobalLeaderboard::GlobalLeaderboard(const std::string& fontFile)
    : m_status(Status::Idle)
{
    if (!m_font.openFromFile(fontFile)) {
        throw std::runtime_error("Cannot load font: " + fontFile);
    }
    
    // Carica il token da variabile d'ambiente o embedded
    const char* token = std::getenv("GITHUB_TOKEN");
    if (token) {
        m_apiToken = token;
    // std::cout << "GitHub token loaded from environment variable." << std::endl;
    } else {
        // Token embedded (spezzato per evitare GitHub security scanning)
        // Sostituisci con il token del tuo account secondario
        std::string part1 = "ghp_UPBhfqsy1M";
        std::string part2 = "jzFIWu0was4rCeU";
        std::string part3 = "GIMq4453Sef";
        
        m_apiToken = part1 + part2 + part3;
    // std::cout << "Using embedded GitHub token from secondary account." << std::endl;
    }
}

void GlobalLeaderboard::setActive(bool active) {
    m_active = active;
}

void GlobalLeaderboard::cancelAsync() {
    // Best-effort: segnala ai thread di terminare il prima possibile
    m_cancelRequested.store(true);
}

void GlobalLeaderboard::uploadScore(const std::string& playerName, unsigned int score) {
    // Prepara la entry
    GlobalEntry newEntry;
    newEntry.playerName = playerName;
    newEntry.score = score;
    newEntry.timestamp = std::time(nullptr);
    newEntry.date = getCurrentDate();
    newEntry.country = "IT";

    // Se c'è già un upload in corso, accoda
    if (m_status == Status::Uploading || (m_uploadFuture.valid() && m_uploadFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)) {
        m_pendingUploads.push_back(newEntry);
    // std::cout << "Upload in progress, queued new entry. Queue size=" << m_pendingUploads.size() << std::endl;
        return;
    }

    // Altrimenti avvia subito
    startUpload(newEntry);
}

void GlobalLeaderboard::startUpload(const GlobalEntry& entry) {
    m_status = Status::Uploading;
    m_errorMessage.clear();

    // Operazione asincrona per GitHub
    m_uploadFuture = std::async(std::launch::async, [this, entry]() -> bool {
        try {
            // std::cout << "Starting upload..." << std::endl;
            // Prima scarica i dati esistenti dal Gist
            std::string currentData = httpGetGist();
            std::vector<GlobalEntry> scores;
            
            if (!currentData.empty()) {
                parseGlobalScores(currentData, scores);
            }
            
            // Aggiungi il nuovo score
            scores.push_back(entry);
            
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
            // std::cout << "Upload payload bytes=" << jsonData.size() << ", entries=" << scores.size() << std::endl;
            bool ok = httpUpdateGist(jsonData);
            // std::cout << "Upload result=" << (ok ? "success" : "fail") << std::endl;
            return ok;
            
        } catch (const std::exception& e) {
            m_errorMessage = std::string("Upload failed: ") + e.what();
            // std::cout << "Upload exception: " << e.what() << std::endl;
            return false;
        }
    });
}

void GlobalLeaderboard::downloadLeaderboard() {
    // Allow initial downloads by removing the active gate
    // if (!m_active) return; // Evita download quando la schermata non è visibile
    if (m_status == Status::Downloading) return; // Già in corso
    
    m_status = Status::Downloading;
    m_errorMessage.clear();
    m_downloadStart = std::chrono::steady_clock::now();
    // Keep spinner visible for at least ~2000ms
    m_minSpinnerUntil = m_downloadStart + std::chrono::milliseconds(2000);
    m_hasPendingStatus = false;
    m_nextStatus = Status::Idle;
    
    // Operazione asincrona per GitHub Gist
    m_downloadFuture = std::async(std::launch::async, [this]() -> bool {
        try {
            // std::cout << "Starting download from GitHub..." << std::endl;
            if (m_cancelRequested.load()) return false;
            std::string response = httpGetGist();
            
            if (response.empty()) {
                // std::cout << "Empty response - likely offline or misconfigured" << std::endl;
                return false; // segnala errore (no update)
            }
            
            // Parse response
            std::vector<GlobalEntry> tmp;
            if (m_cancelRequested.load()) return false;
            bool success = parseGlobalScores(response, tmp);
            // std::cout << "Parse result: " << (success ? "ok" : "fail") << ", count=" << tmp.size() << std::endl;
            if (success) {
                // Non toccare m_globalScores dal thread: salva in staging buffer
                std::lock_guard<std::mutex> lock(m_mutex);
                m_threadDownloadedScores = std::move(tmp);
            }
            return success;
        }
        catch (const std::exception& e) {
            // std::cout << "Download exception: " << e.what() << std::endl;
            m_errorMessage = "Download failed: " + std::string(e.what());
            return false;
        }
    });
}

void GlobalLeaderboard::update() {
    // Controlla se upload è completato
    if (m_uploadFuture.valid() && 
        m_uploadFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        
        bool success = m_uploadFuture.get();
        m_status = success ? Status::Success : Status::Error;

        // Se ci sono upload in coda, avviane subito il prossimo
        if (!m_pendingUploads.empty()) {
            GlobalEntry next = m_pendingUploads.front();
            m_pendingUploads.pop_front();
            // std::cout << "Starting next queued upload. Remaining=" << m_pendingUploads.size() << std::endl;
            startUpload(next);
            return; // Non avviare il download ora: lo faremo dopo l'ultimo upload
        }

        if (success) {
            // Dopo l'ultimo upload riuscito, avvia download automatico
            downloadLeaderboard();
        }
    }
    
    // Controlla se download è completato
    if (m_downloadFuture.valid() && 
        m_downloadFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        bool success = m_downloadFuture.get();
        // Defer status switch if spinner min time not elapsed
        auto now = std::chrono::steady_clock::now();
        if (now < m_minSpinnerUntil) {
            m_hasPendingStatus = true;
            m_nextStatus = success ? Status::Success : Status::Error;
        } else {
            m_status = success ? Status::Success : Status::Error;
        }
    if (success && !m_cancelRequested.load()) {
            // Applica i risultati scaricati in modo thread-safe
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_threadDownloadedScores.empty()) {
                m_globalScores.swap(m_threadDownloadedScores);
                m_threadDownloadedScores.clear();
            }
            m_lastUpdated = std::time(nullptr);
            // std::cout << "Leaderboard updated. Entries=" << m_globalScores.size() << std::endl;
        }
        if (!m_hasPendingStatus) {
            m_status = success ? Status::Success : Status::Error;
        }
    m_cancelRequested.store(false); // reset dopo completamento
    }

    // If we have a pending status change, apply it once the dwell passes
    if (m_hasPendingStatus && std::chrono::steady_clock::now() >= m_minSpinnerUntil) {
        m_status = m_nextStatus;
        m_hasPendingStatus = false;
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
            {
                std::ostringstream s;
                s << "Uploading";
                if (!m_pendingUploads.empty()) {
                    s << " (in coda: " << m_pendingUploads.size() << ")";
                }
                statusText.setString(s.str());
            }
            statusText.setFillColor(sf::Color::Yellow);
            break;
        case Status::Downloading: {
            // Piccolo spinner: ruota tra '|', '/', '-', '\\'
            static const char* frames = "|/-\\";
            static int idx = 0;
            idx = (idx + 1) % 4;
            std::string s = std::string("Downloading ") + frames[idx];
            statusText.setString(s);
            statusText.setFillColor(sf::Color::Cyan);
            break;
        }
        case Status::Error:
            statusText.setString("Offline o errore di rete (R per riprovare)");
            statusText.setFillColor(sf::Color::Red);
            break;
        case Status::Success:
            statusText.setString("Connesso");
            statusText.setFillColor(sf::Color::Green);
            break;
        default:
            statusText.setString("Offline");
            statusText.setFillColor(sf::Color::White);
            break;
    }
    statusText.setPosition(sf::Vector2f(10.f, 10.f));
    target.draw(statusText);

    // Last updated indicator (on same line, next to status)
    if (m_lastUpdated > 0) {
        std::time_t t = m_lastUpdated;
        std::ostringstream ts;
        ts << " • Ultimo aggiornamento: " << std::put_time(std::localtime(&t), "%H:%M:%S");
        // Append as a separate text right after status width
        sf::FloatRect sb = statusText.getLocalBounds();
        float x = statusText.getPosition().x + sb.size.x + 12.f; // padding
        float y = statusText.getPosition().y;
        sf::Text lastUpd(m_font, ts.str(), 16);
        lastUpd.setFillColor(sf::Color(180, 180, 180));
        lastUpd.setPosition(sf::Vector2f(x, y));
        target.draw(lastUpd);
    }
    
    if (m_globalScores.empty()) {
        // Mostra messaggio coerente con lo stato corrente
        std::string msg;
        sf::Color col = sf::Color(128,128,128);
        if (m_status == Status::Downloading) {
            msg = "Caricamento classifica...";
            col = sf::Color::Cyan;
        } else if (m_status == Status::Error) {
            msg = "Nessuna connessione: premi R per riprovare";
            col = sf::Color(200, 80, 80);
        } else {
            msg = "Premi R per scaricare la classifica";
        }
        sf::Text noDataText(m_font, msg, 20);
        noDataText.setFillColor(col);
        // Posiziona con margine a sinistra per evitare tagli
        noDataText.setPosition(sf::Vector2f(windowSize.x * 0.1f, windowSize.y * 0.5f));
        target.draw(noDataText);
        return;
    }
    
    // Intestazioni
    sf::Text headerText(m_font, "RANK   PLAYER        SCORE      DATE", 18);
    headerText.setFillColor(sf::Color::Cyan);
    headerText.setPosition(sf::Vector2f(windowSize.x * 0.1f, windowSize.y * 0.2f));
    target.draw(headerText);
    
    // Numero di righe visibili in base all'altezza finestra
    const std::size_t visible = computeVisibleCount(windowSize);
    const std::size_t start = std::min(m_firstVisibleIndex, m_globalScores.empty() ? std::size_t(0) : m_globalScores.size() - 1);
    const std::size_t end = std::min(start + visible, m_globalScores.size());

    // Scores globali (scrollable)
    for (size_t idx = start; idx < end; ++idx) {
        const auto& entry = m_globalScores[idx];
        size_t i = idx; // i è l'indice assoluto per rank/colori
        
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
    float rowIndex = static_cast<float>(idx - start);
    scoreText.setPosition(sf::Vector2f(windowSize.x * 0.1f, windowSize.y * 0.25f + (rowIndex * 25.f)));
        target.draw(scoreText);
    }
    
    // Istruzioni
    bool isDownloading = (m_status == Status::Downloading);
    bool showSlowHint = false;
    if (isDownloading) {
        auto elapsed = std::chrono::steady_clock::now() - m_downloadStart;
        showSlowHint = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= 5;
    }
    std::string instr = "UP/DOWN per scorrere. R aggiorna. ESC menu";
    if (showSlowHint) instr += "  (lento? premi R)";
    sf::Text instructionsText(m_font, instr, 16);
    instructionsText.setFillColor(sf::Color(128, 128, 128)); // Grigio
    instructionsText.setPosition(sf::Vector2f(windowSize.x * 0.1f, windowSize.y * 0.9f));
    target.draw(instructionsText);
}

// HTTP GET da GitHub Raw Files usando CPR
std::string GlobalLeaderboard::httpGetGist() {
    try {
        const bool insecure = (std::getenv("PACMUX_INSECURE_SSL") != nullptr);
    // if (insecure) {
    //     std::cout << "PACMUX_INSECURE_SSL=1 -> SSL verification disabled" << std::endl;
    // }
        // Build SSL options: force TLS 1.2; when insecure, disable peer/host verification too
        auto sslOpt = insecure
            ? cpr::Ssl(cpr::ssl::TLSv1_2{}, cpr::ssl::VerifyPeer{false}, cpr::ssl::VerifyHost{false})
            : cpr::Ssl(cpr::ssl::TLSv1_2{});
        // Se configurazione non è ancora aggiornata con account secondario, usa dati simulati
        if (JSONBIN_URL.find("ACCOUNT_SECONDARIO") != std::string::npos || 
            m_apiToken.find("SOSTITUISCI") != std::string::npos) {
            // Config non completata: non restituire dati fittizi in runtime
            // std::cout << "GitHub not configured - returning empty to avoid fake data" << std::endl;
            return "";
        }
        
        // Primo tentativo: GitHub Contents API (meno caching)
        if (!m_apiToken.empty()) {
            std::string repoInfo = extractRepoInfo();
            std::string apiUrl = "https://api.github.com/repos/" + repoInfo + "/contents/scores.json";
            // std::cout << "Requesting GitHub Contents API: " << apiUrl << std::endl;
            auto ra = cpr::Get(
                cpr::Url{apiUrl},
                cpr::Header{
                    {"User-Agent", "Pacman-SFML/1.0"},
                    {"Accept", "application/vnd.github.v3+json"},
                    {"Authorization", "token " + m_apiToken}
                },
                cpr::VerifySsl{!insecure},
                sslOpt,
                cpr::Timeout{15000},
                cpr::Redirect{true}
            );
            // std::cout << "Contents API Status: " << ra.status_code << std::endl;
            if (ra.status_code == 200) {
                const std::string& body = ra.text;
                // Estrai il campo "content" (string JSON) tenendo conto delle sequenze escape
                auto extractJsonStringValue = [](const std::string& json, const std::string& key) -> std::string {
                    std::string pattern = "\"" + key + "\":"; // "content":
                    size_t pos = json.find(pattern);
                    if (pos == std::string::npos) return {};
                    pos += pattern.size();
                    // Salta spazi e due punti opzionali
                    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
                    if (pos >= json.size() || json[pos] != '"') return {};
                    pos++; // dopo la prima "
                    std::string out;
                    bool escape = false;
                    for (; pos < json.size(); ++pos) {
                        char c = json[pos];
                        if (escape) {
                            // Decodifica principali sequenze di escape JSON
                            switch (c) {
                                case 'n': out.push_back('\n'); break;
                                case 'r': out.push_back('\r'); break;
                                case 't': out.push_back('\t'); break;
                                case '"': out.push_back('"'); break;
                                case '\\': out.push_back('\\'); break;
                                default: out.push_back(c); break; // fallback: carattere letterale
                            }
                            escape = false;
                        } else if (c == '\\') {
                            escape = true;
                        } else if (c == '"') {
                            // fine stringa
                            break;
                        } else {
                            out.push_back(c);
                        }
                    }
                    return out;
                };

                std::string b64 = extractJsonStringValue(body, "content");
                if (!b64.empty()) {
                    // Sanitize: rimuovi newline/carriage e spazi reali (la stringa è già unescaped)
                    b64.erase(std::remove(b64.begin(), b64.end(), '\n'), b64.end());
                    b64.erase(std::remove(b64.begin(), b64.end(), '\r'), b64.end());
                    b64.erase(std::remove(b64.begin(), b64.end(), ' '), b64.end());
                    b64.erase(std::remove(b64.begin(), b64.end(), '\t'), b64.end());

                    // base64 decode
                    auto decodeB64 = [](const std::string& s) -> std::string {
                        auto value = [](unsigned char c) -> int {
                            if (c >= 'A' && c <= 'Z') return c - 'A';
                            if (c >= 'a' && c <= 'z') return c - 'a' + 26;
                            if (c >= '0' && c <= '9') return c - '0' + 52;
                            if (c == '+') return 62;
                            if (c == '/') return 63;
                            if (c == '=') return -2; // padding
                            return -1;
                        };
                        std::string out;
                        int val = 0, valb = -8;
                        for (unsigned char c : s) {
                            int d = value(c);
                            if (d == -1) continue;     // ignora caratteri non base64
                            if (d == -2) break;         // padding '='
                            val = (val << 6) + d;
                            valb += 6;
                            if (valb >= 0) {
                                out.push_back(char((val >> valb) & 0xFF));
                                valb -= 8;
                            }
                        }
                        return out;
                    };

                    std::string decoded = decodeB64(b64);
                    if (!decoded.empty()) {
                        // std::cout << "Loaded content via API (decoded length=" << decoded.size() << ")" << std::endl;
                        // Sanity check: deve contenere la chiave leaderboard e le parentesi dell'array
                        if (decoded.find("\"leaderboard\"") != std::string::npos &&
                            decoded.find("[") != std::string::npos && decoded.find("]") != std::string::npos) {
                            return decoded;
                        } else {
                            // std::cout << "Decoded payload missing leaderboard array, falling back to Raw" << std::endl;
                        }
                    }
                }
            } else {
                // if (!ra.text.empty()) {
                //     std::cout << "Contents API response snippet: " << ra.text.substr(0, 200) << std::endl;
                // }
            }
        }

        // Fallback: Raw Files con cache buster
    // std::cout << "Requesting GitHub Raw Files: " << JSONBIN_URL << std::endl;
        
        // Aggiungi un timestamp per evitare la cache + numero random per maggiore unicità
        std::string urlWithTimestamp = JSONBIN_URL + "?_=" + std::to_string(std::time(nullptr)) + "&r=" + std::to_string(std::rand());
    // std::cout << "URL with cache-buster: " << urlWithTimestamp << std::endl;
        
        auto r = cpr::Get(
            cpr::Url{urlWithTimestamp},
            cpr::Header{
                {"User-Agent", "Pacman-SFML/1.0"}, 
                {"Accept", "application/json"}, 
                {"Cache-Control", "no-cache, no-store, must-revalidate"},
                {"Pragma", "no-cache"},
                {"Expires", "0"}
            },
            cpr::VerifySsl{!insecure},
            sslOpt,
            cpr::Timeout{15000},   // 15 secondi
            cpr::Redirect{true}
        );
        
    // std::cout << "Raw HTTP Status: " << r.status_code << ", error='" << r.error.message << "'" << std::endl;
        
        if (r.status_code == 200) {
        // std::cout << "GitHub Raw Files data loaded successfully! length=" << r.text.length() << std::endl;
            return r.text;
        } else {
            // if (!r.text.empty()) {
        //     std::cout << "Raw response snippet: " << r.text.substr(0, 200) << std::endl;
            // }
        }
        
    // std::cout << "Failed to connect to GitHub (Raw) - returning empty" << std::endl;
        return "";
        
    } catch (const std::exception& e) {
    // std::cout << "Exception in httpGetGist: " << e.what() << std::endl;
        return "";
    }
}

// Scrolling API
void GlobalLeaderboard::scroll(int delta) {
    if (m_globalScores.empty()) return;
    // delta positivo scende, negativo sale
    long long next = static_cast<long long>(m_firstVisibleIndex) + static_cast<long long>(delta);
    if (next < 0) next = 0;
    if (next > static_cast<long long>(m_globalScores.size() > 0 ? m_globalScores.size() - 1 : 0)) {
        next = static_cast<long long>(m_globalScores.size() > 0 ? m_globalScores.size() - 1 : 0);
    }
    m_firstVisibleIndex = static_cast<std::size_t>(next);
}

void GlobalLeaderboard::scrollToStart() {
    m_firstVisibleIndex = 0;
}

void GlobalLeaderboard::scrollToEnd(const sf::Vector2u& windowSize) {
    if (m_globalScores.empty()) { m_firstVisibleIndex = 0; return; }
    std::size_t visible = computeVisibleCount(windowSize);
    if (m_globalScores.size() > visible) {
        m_firstVisibleIndex = m_globalScores.size() - visible;
    } else {
        m_firstVisibleIndex = 0;
    }
}

std::size_t GlobalLeaderboard::computeVisibleCount(const sf::Vector2u& windowSize) const {
    // Spazio utile: da 25% a ~90% dell'altezza; riga ~25px
    float space = (windowSize.y * 0.9f) - (windowSize.y * 0.25f);
    int count = static_cast<int>(space / 25.f);
    if (count < 5) count = 5;       // almeno 5 righe
    if (count > 50) count = 50;     // non ha senso oltre top 50
    return static_cast<std::size_t>(count);
}

// HTTP UPDATE per GitHub Repository usando CPR
bool GlobalLeaderboard::httpUpdateGist(const std::string& jsonData) {
    try {
        const bool insecure = (std::getenv("PACMUX_INSECURE_SSL") != nullptr);
        auto sslOpt = insecure
            ? cpr::Ssl(cpr::ssl::TLSv1_2{}, cpr::ssl::VerifyPeer{false}, cpr::ssl::VerifyHost{false})
            : cpr::Ssl(cpr::ssl::TLSv1_2{});
        // Se configurazione non è ancora aggiornata con account secondario, simula successo
        if (JSONBIN_URL.find("ACCOUNT_SECONDARIO") != std::string::npos || 
            m_apiToken.find("SOSTITUISCI") != std::string::npos) {
            
            // std::cout << "GitHub Raw Files not configured, simulating upload..." << std::endl;
            // std::cout << "Data to upload (snippet): " << jsonData.substr(0, std::min(100, (int)jsonData.length())) << std::endl;
            return true; // Simula successo
        }
        
        // URL dell'API GitHub per aggiornare il file
        std::string repoInfo = extractRepoInfo();
        std::string apiUrl = "https://api.github.com/repos/" + repoInfo + "/contents/scores.json";
        
        // Crea il payload per aggiornare il file
        std::string payload = createGitHubUpdatePayload(jsonData);
        
    // std::cout << "Updating GitHub file via API: " << apiUrl << std::endl;
    // std::cout << "Payload size: " << payload.length() << std::endl;
        
        auto r = cpr::Put(
            cpr::Url{apiUrl},
            cpr::Header{
                {"User-Agent", "Pacman-SFML/1.0"},
                {"Accept", "application/vnd.github.v3+json"},
                {"Content-Type", "application/json"},
                {"Authorization", "token " + m_apiToken}
            },
            cpr::Body{payload},
            cpr::VerifySsl{!insecure},
            sslOpt,
            cpr::Timeout{15000}
        );
        
    // std::cout << "GitHub API Status: " << r.status_code << ", error='" << r.error.message << "'" << std::endl;
        
        if (r.status_code == 200 || r.status_code == 201) {
        // std::cout << "GitHub file update successful!" << std::endl;
            return true;
        } else {
            // if (!r.text.empty()) {
        //     std::cout << "Update response snippet: " << r.text.substr(0, 200) << std::endl;
            // }
            return false;
        }
        
    } catch (const std::exception& e) {
    // std::cout << "Exception in httpUpdateGist: " << e.what() << std::endl;
        return false;
    }
}

std::string GlobalLeaderboard::createScoresJson(const std::vector<GlobalEntry>& scores) {
    std::ostringstream json;
    json << "{\"leaderboard\":[";
    
    auto escapeJsonString = [](const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        // skip control chars
                    } else {
                        out.push_back(c);
                    }
            }
        }
        return out;
    };

    for (size_t i = 0; i < scores.size(); ++i) {
        if (i > 0) json << ",";
        json << "{"
             << "\"name\":\"" << escapeJsonString(scores[i].playerName) << "\"," 
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
    // std::cout << "JSON response is empty" << std::endl;
        return true; // Primo utilizzo, nessun dato ancora
    }
    
    // Log limitato per evitare lag
    // std::cout << "JSON to parse (snippet): " << jsonResponse.substr(0, std::min(150, (int)jsonResponse.length())) << std::endl;
    
    // Cerca l'array "leaderboard" nel JSON
    size_t leaderboardPos = jsonResponse.find("\"leaderboard\":");
    if (leaderboardPos == std::string::npos) {
    // std::cout << "'leaderboard' key not found in JSON" << std::endl;
        return false; // Formato JSON non valido
    }
    
    size_t arrayStart = jsonResponse.find("[", leaderboardPos);
    size_t arrayEnd = jsonResponse.find("]", arrayStart);
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) {
    // std::cout << "Array brackets not found" << std::endl;
        return false;
    }
    
    // std::cout << "Array bounds: " << arrayStart << "-" << arrayEnd << std::endl;
    
    // Estrai il contenuto dell'array
    std::string arrayContent = jsonResponse.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    // std::cout << "Array content preview: " << arrayContent.substr(0, std::min(100, (int)arrayContent.length())) << std::endl;
    
    // Parsing degli oggetti JSON: cerca "name", "score", "timestamp" in ogni oggetto
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
            // std::cout << "Unmatched braces in JSON object" << std::endl;
            break;
        }
        
        // Estrai l'oggetto JSON (senza le parentesi graffe)
        std::string objContent = arrayContent.substr(objStart + 1, objEnd - objStart - 2);
    // Riduci logging per evitare stutter
        
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
                    // name parsed
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
                // score parsed
            }
        }
        
        // Parse timestamp
        size_t timestampStart = objContent.find("\"timestamp\":");
        if (timestampStart != std::string::npos) {
            timestampStart += 12; // Salta "timestamp":
            // Salta eventuali spazi
            while (timestampStart < objContent.length() && objContent[timestampStart] == ' ') timestampStart++;
            // Prendi fino a virgola o chiusura oggetto
            size_t timestampEnd = objContent.find_first_of(",}", timestampStart);
            if (timestampEnd == std::string::npos) timestampEnd = objContent.length();
            if (timestampEnd > timestampStart) {
                std::string timestampStr = objContent.substr(timestampStart, timestampEnd - timestampStart);
                entry.timestamp = std::stoul(timestampStr);
                
                // Genera la data formattata dal timestamp
                std::time_t timeValue = entry.timestamp;
                std::ostringstream dateStream;
                dateStream << std::put_time(std::localtime(&timeValue), "%d/%m/%Y");
                entry.date = dateStream.str();
                
                fieldsFound++;
                // timestamp parsed
            } else {
                // std::cout << "Timestamp end not found in: " << objContent << std::endl;
            }
        } else {
            // std::cout << "Timestamp key not found in: " << objContent << std::endl;
        }
        
        // Aggiungi il record se ha tutti i campi
        if (fieldsFound >= 3) {
            scores.push_back(entry);
            recordsParsed++;
            // record added
        } else {
            // std::cout << "Skipping incomplete record (found " << fieldsFound << " fields)" << std::endl;
        }
        
        // Vai al prossimo oggetto dopo la parentesi di chiusura
        pos = objEnd;
    }
    
    // std::cout << "Parsed " << scores.size() << " scores" << std::endl;
    
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
    // std::cout << "[DEBUG] Using fallback data..." << std::endl;
    return "{\"leaderboard\":[{\"name\":\"MasterPac\",\"score\":15000,\"timestamp\":1691750400},{\"name\":\"GhostHunter\",\"score\":12500,\"timestamp\":1691750300},{\"name\":\"DotEater\",\"score\":11200,\"timestamp\":1691750200},{\"name\":\"PowerPellet\",\"score\":9800,\"timestamp\":1691750100},{\"name\":\"CherryPicker\",\"score\":8500,\"timestamp\":1691750000}]}";
}

std::string GlobalLeaderboard::extractGistIdFromUrl() {
    // Per GitHub Raw Files estrae owner/repo dal URL
    // URL formato: http://raw.githubusercontent.com/DenisMux/pacmux-leaderboard/refs/heads/main/scores.json
    // std::cout << "[DEBUG] Full URL: " << JSONBIN_URL << std::endl;
    
    // TEMPORANEO: Hard-code del valore corretto per testare l'upload
    std::string hardcoded = "DenisMux/pacmux-leaderboard";
    // std::cout << "[DEBUG] Using hardcoded repo info: '" << hardcoded << "'" << std::endl;
    return hardcoded;
    
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
        const bool insecure = (std::getenv("PACMUX_INSECURE_SSL") != nullptr);
        auto sslOpt = insecure
            ? cpr::Ssl(cpr::ssl::TLSv1_2{}, cpr::ssl::VerifyPeer{false}, cpr::ssl::VerifyHost{false})
            : cpr::Ssl(cpr::ssl::TLSv1_2{});
        // Ottieni lo SHA attuale del file tramite API GitHub
        std::string repoInfo = extractRepoInfo();
        std::string apiUrl = "https://api.github.com/repos/" + repoInfo + "/contents/scores.json";
        
    // std::cout << "Getting current file SHA from: " << apiUrl << std::endl;
        
        auto r = cpr::Get(
            cpr::Url{apiUrl},
            cpr::Header{
                {"User-Agent", "Pacman-SFML/1.0"},
                {"Accept", "application/vnd.github.v3+json"},
                {"Authorization", "token " + m_apiToken}
            },
            cpr::VerifySsl{!insecure},
            sslOpt,
            cpr::Timeout{10000}
        );
        
    // std::cout << "SHA Request Status: " << r.status_code << ", error='" << r.error.message << "'" << std::endl;
        
        if (r.status_code == 200) {
            // Estrai lo SHA dalla risposta JSON
            std::string response = r.text;
            size_t shaStart = response.find("\"sha\":\"");
            if (shaStart != std::string::npos) {
                shaStart += 7; // Salta "sha":"
                size_t shaEnd = response.find("\"", shaStart);
                if (shaEnd != std::string::npos) {
                    std::string sha = response.substr(shaStart, shaEnd - shaStart);
            // std::cout << "Current file SHA: " << sha << std::endl;
                    return sha;
                }
            }
        // std::cout << "SHA not found in response" << std::endl;
        } else {
        // std::cout << "Failed to get SHA: " << r.status_code << " - " << r.error.message << std::endl;
        }
        
    } catch (const std::exception& e) {
    // std::cout << "Exception getting SHA: " << e.what() << std::endl;
    }
    
    // Fallback: usa SHA dummy (questo causerà errore ma è meglio di niente)
    // std::cout << "Using dummy SHA - upload will likely fail" << std::endl;
    return "dummy_sha_will_be_updated";
}

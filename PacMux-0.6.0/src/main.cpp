#include <SFML/Graphics.hpp>
#include <filesystem>
#include <vector>
#include <memory>
#include <string>
#include <iostream>          // Per il log di debug
#include <array>
#include <cstdint>
#include <algorithm>        // std::find_if, std::max
#include <cmath>            // std::pow, std::abs (float overloads)
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#if defined(__linux__)
#include <unistd.h>
#endif

#include "TileMap.hpp"
#include "Player.hpp"
#include "Pellet.hpp"
#include "Score.hpp"
#include "Blinky.hpp"
#include "Pinky.hpp"
#include "Inky.hpp"
#include "Clyde.hpp"

// Utility: mostra un messaggio grafico e attende INVIO (compatibile SFML 3)
void showMessage(sf::RenderWindow& window, const std::string& message, const std::string& fontPath) {
    sf::Font font(fontPath); // SFML 3: load font via constructor
    sf::Text text(font, message, 20); // Font size ridotto per evitare tagli
    text.setFillColor(sf::Color::Yellow);
    text.setOutlineColor(sf::Color::Blue);
    text.setOutlineThickness(2);
    
    // Calcola posizione per centrare meglio il testo
    float windowWidth = static_cast<float>(window.getSize().x);
    float windowHeight = static_cast<float>(window.getSize().y);
    
    // Posiziona il testo più in alto e con margini adeguati
    float textX = windowWidth * 0.1f; // 10% dal bordo sinistro
    float textY = windowHeight * 0.35f; // 35% dall'alto
    text.setPosition({textX, textY});
    
    // Crea un rettangolo di sfondo in stile Pac-Man
    sf::RectangleShape background;
    background.setSize({windowWidth * 0.8f, windowHeight * 0.3f});
    background.setPosition({windowWidth * 0.1f, windowHeight * 0.3f});
    background.setFillColor(sf::Color::Black);
    background.setOutlineColor(sf::Color::Blue);
    background.setOutlineThickness(3);
    
    bool waiting = true;
    while (waiting) {
        window.clear(sf::Color::Black);
        window.draw(background);
        window.draw(text);
        
        // Aggiungi "PRESS ENTER" lampeggiante
        static sf::Clock blinkClock;
        if (blinkClock.getElapsedTime().asSeconds() < 0.5f) {
            sf::Text pressEnter(font, "PRESS ENTER", 16);
            pressEnter.setFillColor(sf::Color::White);
            pressEnter.setPosition({windowWidth * 0.4f, windowHeight * 0.55f});
            window.draw(pressEnter);
        }
        if (blinkClock.getElapsedTime().asSeconds() >= 1.0f) {
            blinkClock.restart();
        }
        
        window.display();
        
        auto event = window.waitEvent(); // SFML 3: returns std::optional<sf::Event>
        if (!event) continue;
        if (event->is<sf::Event::KeyPressed>()) {
            // Access event data through the variant-like interface
            if (auto keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    waiting = false; // Exit the loop instead of breaking
                }
            }
        }
        if (event->is<sf::Event::Closed>()) {
            window.close();
            exit(0);
        }
    }
}

namespace {
    namespace fs = std::filesystem;
    static fs::path getExecutableDir() {
#if defined(_WIN32)
    char buf[32768];
    unsigned long len = GetModuleFileNameA(nullptr, buf, static_cast<unsigned long>(sizeof(buf)));
    if (len == 0 || len >= sizeof(buf)) return fs::current_path();
    return fs::path(buf).parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0; _NSGetExecutablePath(nullptr, &size);
    std::string tmp(size, '\0');
    if (_NSGetExecutablePath(tmp.data(), &size) == 0) return fs::path(tmp.c_str()).parent_path();
    return fs::current_path();
#elif defined(__linux__)
    std::array<char, 4096> buf{}; ssize_t n = readlink("/proc/self/exe", buf.data(), buf.size()-1);
    if (n > 0) { buf[static_cast<size_t>(n)]='\0'; return fs::path(buf.data()).parent_path(); }
    return fs::current_path();
#else
    return fs::current_path();
#endif
    }
    static void showDialog(const std::string& title, const std::string& msg, bool errorIcon=true) {
#if defined(_WIN32)
        MessageBoxA(nullptr, msg.c_str(), title.c_str(), MB_OK | (errorIcon?MB_ICONERROR:MB_ICONINFORMATION));
#else
    std::cerr << title << ": " << msg << std::endl;
#endif
    }
}

int main()
{
    // Recupera la cartella dell'eseguibile e degli asset
    fs::path exeDir = getExecutableDir();
    fs::path assets = exeDir / "assets";
    fs::path mapPath = assets / "map1.txt";
    fs::path fontPath = assets / "arial.ttf";

    // Verifica la presenza degli asset fondamentali
    if (!fs::exists(mapPath)) { showDialog("Errore Pacman", "Mappa non trovata:\n"+mapPath.string(), true); return EXIT_FAILURE; }
    if (!fs::exists(fontPath)) { showDialog("Errore Pacman", "Font non trovato:\n"+fontPath.string(), true); return EXIT_FAILURE; }

    // Configura la finestra di gioco
    const sf::Vector2u tileSize{32,32};
    TileMap map;
    if (!map.load(mapPath.string(), tileSize)) { showDialog("Errore Pacman", "Errore caricamento mappa:\n"+mapPath.string(), true); return EXIT_FAILURE; }
    auto mapSz = map.getSize();
    sf::Vector2f startPos{
        (mapSz.x * tileSize.x) / 2.f,
        (mapSz.y * tileSize.y) / 2.f
    };
    sf::VideoMode mode(sf::Vector2u{tileSize.x * mapSz.x, tileSize.y * mapSz.y}, 32);

    // Wrap-around per Pac-Man: correggi posizione se esce dai bordi
    if (startPos.x < 0) startPos.x += mapSz.x * tileSize.x;
    if (startPos.x >= mapSz.x * tileSize.x) startPos.x -= mapSz.x * tileSize.x;
    if (startPos.y < 0) startPos.y += mapSz.y * tileSize.y;
    if (startPos.y >= mapSz.y * tileSize.y) startPos.y -= mapSz.y * tileSize.y;

    sf::RenderWindow window(mode, "Fake Pacman", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60); // Ensure a consistent framerate

    // Trova la posizione di spawn di Pac-Man ('P') o usa il centro
    for (unsigned y = 0; y < mapSz.y; ++y) {
        for (unsigned x = 0; x < mapSz.x; ++x) {
            if (map.getData()[y][x] == 'P') {
                startPos = {
                    x * float(tileSize.x) + tileSize.x/2.f,
                    y * float(tileSize.y) + tileSize.y/2.f
                };
            }
        }
    }

    // Carica il font e inizializza il punteggio
    std::unique_ptr<Score> score;
    try {
        score = std::make_unique<Score>(fontPath.string());
    } catch (const std::exception& e) { showDialog("Errore Pacman", e.what(), true); return EXIT_FAILURE; }

    // Crea il giocatore (Pac-Man)
    Player pac(120.f, startPos, tileSize);

    // Genera tutti i pellet sulle celle libere, ESCLUDENDO tile '2' e la cella di spawn di Pac-Man
    std::vector<Pellet> pellets;
    for (unsigned y = 0; y < mapSz.y; ++y) {
        for (unsigned x = 0; x < mapSz.x; ++x) {
            char tile = map.getData()[y][x];
            // Escludi la cella di spawn di Pac-Man
            sf::Vector2f pos{
                x*float(tileSize.x)+tileSize.x/2.f,
                y*float(tileSize.y)+tileSize.y/2.f
            };
            bool isPacmanSpawn = (std::abs(pos.x - startPos.x) < 1e-2f && std::abs(pos.y - startPos.y) < 1e-2f);
            // Genera pellet solo sui tile '0' (spazi vuoti) e non sui tile '1' (muri) o '2' (spazi vuoti senza pellet)
            if (tile == '0' && !isPacmanSpawn) {
                pellets.emplace_back(pos);
            }
        }
    }

    // Crea i fantasmi mobili - ora usando classi specifiche
    std::vector<std::unique_ptr<Ghost>> ghosts;
    const std::vector<sf::Vector2f> ghostStartPos = {
        sf::Vector2f(10*tileSize.x+tileSize.x/2.f, 9*tileSize.y+tileSize.y/2.f),  // Blinky (centro, riga 10) - ghost house entrance
        sf::Vector2f(10*tileSize.x+tileSize.x/2.f, 10*tileSize.y+tileSize.y/2.f), // Pinky (centro, riga 11) 
        sf::Vector2f(11*tileSize.x+tileSize.x/2.f, 10*tileSize.y+tileSize.y/2.f),  // Inky (sinistra, riga 11)
        sf::Vector2f(9*tileSize.x+tileSize.x/2.f, 10*tileSize.y+tileSize.y/2.f)  // Clyde (destra, riga 11)
    };
    
    ghosts.push_back(std::make_unique<Blinky>(ghostStartPos[0]));
    ghosts.push_back(std::make_unique<Pinky>(ghostStartPos[1]));
    ghosts.push_back(std::make_unique<Inky>(ghostStartPos[2]));
    ghosts.push_back(std::make_unique<Clyde>(ghostStartPos[3]));

    // Initialize all ghosts as unreleased (cascade system will control release)
    for (auto& ghost : ghosts) {
        ghost->setReleased(false);
    }

    // --- TIMER MODALITÀ GHOSTS ---
    enum class GhostMode { Scatter, Chase };
    GhostMode ghostMode = GhostMode::Scatter; // Inizia in modalità SCATTER come nel classico
    float modeTimer = 0.f;
    int modePhase = 0;
    // Tabella classica: scatter/chase (in secondi) - ora SCATTER È LUNGO QUANTO ERA chase E VICEVERSA
    const std::vector<float> scatterChaseTimes = {
        7.f, 20.f, 7.f, 20.f, 5.f, 20.f, 5.f, -1.f // tempi classici Pac-Man
    };
    bool modeJustChanged = false;

    // --- Super Pellet positions ---
    std::vector<sf::Vector2f> superPelletPositions;
    for (unsigned y = 0; y < mapSz.y; ++y) {
        for (unsigned x = 0; x < mapSz.x; ++x) {
            if (map.getData()[y][x] == 'S') {
                superPelletPositions.emplace_back(
                    x * float(tileSize.x) + tileSize.x/2.f,
                    y * float(tileSize.y) + tileSize.y/2.f
                );
            }
        }
    }

    // Funzione di reset centralizzata per pellet e super pellet
    auto resetPelletsAndSuperPellets = [&]() {
        if (!map.load(mapPath.string(), tileSize)) {
            showDialog("Errore Pacman", "Errore caricamento mappa:\n"+mapPath.string(), true);
            exit(EXIT_FAILURE);
        }
        pellets.clear();
        superPelletPositions.clear();
        for (unsigned y = 0; y < mapSz.y; ++y) {
            for (unsigned x = 0; x < mapSz.x; ++x) {
                char tile = map.getData()[y][x];
                sf::Vector2f pos{
                    x*float(tileSize.x)+tileSize.x/2.f,
                    y*float(tileSize.y)+tileSize.y/2.f
                };
                bool isPacmanSpawn = (std::abs(pos.x - startPos.x) < 1e-2f && std::abs(pos.y - startPos.y) < 1e-2f);
                if (tile == '0' && !isPacmanSpawn) {
                    pellets.emplace_back(pos);
                }
                if (tile == 'S') {
                    superPelletPositions.emplace_back(pos);
                }
            }
        }
    };

    // --- GESTIONE MULTI-LIVELLO ---
    std::vector<std::string> mapFiles = {"map1.txt", "map2.txt"};
    int currentLevel = 0;
    bool allMapsCompleted = false;
    int difficultyLevel = 1;
    float ghostBaseSpeed = 90.f;
    float frightenedBaseDuration = 6.0f;
    std::vector<float> ghostReleaseDelays = {0.f, 3.f, 3.f, 3.f};
    const float minFrightened = 1.5f;
    const float minRelease = 0.5f;
    
    // --- GESTIONE RELEASE SEMPLICE E SEQUENZIALE DEI FANTASMI ---
    // Il primo esce subito, poi ogni X secondi il prossimo
    //std::vector<float> ghostReleaseDelays = {0.f, 3.f, 3.f, 3.f}; // solo il primo delay è assoluto, gli altri sono intervalli tra un rilascio e il successivo
    int nextGhostToRelease = 0;
    float ghostReleaseTimer = 0.f;
    
    auto loadLevel = [&](int levelIdx) {
        if (levelIdx >= (int)mapFiles.size()) {
            showDialog("Game Completed", "Hai completato tutti i livelli! Congratulazioni!", false);
            currentLevel = 0;
            levelIdx = 0;
        }
        mapPath = assets / mapFiles[levelIdx];
        if (!map.load(mapPath.string(), tileSize)) {
            showDialog("Errore Pacman", "Mappa non trovata:\n" + mapPath.string(), true);
            exit(EXIT_FAILURE);
        }
        mapSz = map.getSize();
        // Trova spawn Pac-Man
        for (unsigned y = 0; y < mapSz.y; ++y) {
            for (unsigned x = 0; x < mapSz.x; ++x) {
                if (map.getData()[y][x] == 'P') {
                    startPos = {
                        x * float(tileSize.x) + tileSize.x/2.f,
                        y * float(tileSize.y) + tileSize.y/2.f
                    };
                }
            }
        }
        // Reset pellet e super pellet
        pellets.clear();
        superPelletPositions.clear();
        for (unsigned y = 0; y < mapSz.y; ++y) {
            for (unsigned x = 0; x < mapSz.x; ++x) {
                char tile = map.getData()[y][x];
                sf::Vector2f pos{
                    x*float(tileSize.x)+tileSize.x/2.f,
                    y*float(tileSize.y)+tileSize.y/2.f
                };
                bool isPacmanSpawn = (std::abs(pos.x - startPos.x) < 1e-2f && std::abs(pos.y - startPos.y) < 1e-2f);
                if (tile == '0' && !isPacmanSpawn) {
                    pellets.emplace_back(pos);
                }
                if (tile == 'S') {
                    superPelletPositions.emplace_back(pos);
                }
            }
        }
        // Reset fantasmi
        ghosts.clear();
        ghosts.push_back(std::make_unique<Blinky>(ghostStartPos[0]));
        ghosts.push_back(std::make_unique<Pinky>(ghostStartPos[1]));
        ghosts.push_back(std::make_unique<Inky>(ghostStartPos[2]));
        ghosts.push_back(std::make_unique<Clyde>(ghostStartPos[3]));
        // Aggiorna velocità e frightened in base alla difficoltà
        float speed = ghostBaseSpeed;
        float frightenedDuration = frightenedBaseDuration;
        for (auto& g : ghosts) {
            g->setSpeed(speed);
            g->setFrightened(0.f);
            g->setEaten(false);
            g->setReleased(false);
        }
        // Reset ghost release state sequenziale
        nextGhostToRelease = 0;
        ghostReleaseTimer = 0.f;
        pac = Player(120.f, startPos, tileSize);
        score = std::make_unique<Score>(fontPath.string());
    };

    // Game loop principale
    sf::Clock clock;
    bool gameOver = false;
    bool gameStarted = false;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        // --- Modalità ghosts ---
        modeJustChanged = false;
        // Riattiva il cambio automatico scatter/chase
        if (modePhase < (int)scatterChaseTimes.size() && scatterChaseTimes[modePhase] > 0.f) {
            modeTimer += dt;
            if (modeTimer >= scatterChaseTimes[modePhase]) {
                modeTimer = 0.f;
                modePhase++;
                ghostMode = (ghostMode == GhostMode::Scatter) ? GhostMode::Chase : GhostMode::Scatter;
                modeJustChanged = true;
                std::cout << "[DEBUG] Cambio modalità fantasmi: " << (ghostMode == GhostMode::Scatter ? "SCATTER" : "CHASE") << std::endl;
            }
        }
        // Gestione eventi finestra
        while (auto ev = window.pollEvent())
            if (ev->is<sf::Event::Closed>())
                window.close();

        if (!gameOver) {
            // Aggiorna il giocatore
            pac.update(dt, map, tileSize);

            // Forza la posizione di Pac-Man sempre dentro i limiti della mappa dopo ogni update
            sf::Vector2f pacPos = pac.getPosition();
            float minX = tileSize.x / 2.f;
            float minY = tileSize.y / 2.f;
            float maxX = map.getSize().x * tileSize.x - tileSize.x / 2.f;
            float maxY = map.getSize().y * tileSize.y - tileSize.y / 2.f;
            if (pacPos.x < minX) pac.setPosition({minX, pacPos.y});
            if (pacPos.x > maxX) pac.setPosition({maxX, pacPos.y});
            if (pacPos.y < minY) pac.setPosition({pacPos.x, minY});
            if (pacPos.y > maxY) pac.setPosition({pacPos.x, maxY});

            // Wrap-around per Pac-Man: riallinea alla griglia dopo il teletrasporto
            if (pacPos.x < minX) pac.setPosition({mapSz.x * tileSize.x - tileSize.x / 2.f, pacPos.y});
            if (pacPos.x > maxX) pac.setPosition({tileSize.x / 2.f, pacPos.y});
            if (pacPos.y < minY) pac.setPosition({pacPos.x, mapSz.y * tileSize.y - tileSize.y / 2.f});
            if (pacPos.y > maxY) pac.setPosition({pacPos.x, tileSize.y / 2.f});

            // --- GESTIONE RELEASE SEMPLICE E SEQUENZIALE DEI FANTASMI ---
            if (gameStarted && nextGhostToRelease < 4) {
                ghostReleaseTimer += dt;
                if (ghostReleaseTimer >= ghostReleaseDelays[nextGhostToRelease]) {
                    ghosts[nextGhostToRelease]->setReleased(true);
                    std::cout << "[DEBUG] Ghost " << nextGhostToRelease << " (" << Ghost::getTypeName(static_cast<Ghost::Type>(nextGhostToRelease)) << ") released!\n";
                    nextGhostToRelease++;
                    ghostReleaseTimer = 0.f; // resetta il timer SOLO dopo il rilascio
                }
            }

            // Aggiorna i fantasmi con la nuova architettura
            for (size_t i = 0; i < ghosts.size(); ++i) {
                Ghost::Mode m = (ghostMode == GhostMode::Scatter) ? Ghost::Mode::Scatter : Ghost::Mode::Chase;
                if (auto* inky = dynamic_cast<Inky*>(ghosts[i].get())) {
                    if (ghosts.size() > 0) {
                        inky->update(dt, map, tileSize, pac.getPosition(), pac.getDirection(), m, ghosts[0]->getPosition(), gameStarted);
                    }
                } else if (auto* pinky = dynamic_cast<Pinky*>(ghosts[i].get())) {
                    pinky->update(dt, map, tileSize, pac.getPosition(), pac.getDirection(), m, gameStarted);
                } else if (auto* clyde = dynamic_cast<Clyde*>(ghosts[i].get())) {
                    clyde->update(dt, map, tileSize, pac.getPosition(), pac.getDirection(), m, gameStarted);
                } else {
                    ghosts[i]->update(dt, map, tileSize, pac.getPosition(), pac.getDirection(), m, gameStarted);
                }
            }

            // Controlla collisione con i pellet
            for (auto it = pellets.begin(); it != pellets.end(); )
                if (it->eaten(pac.getPosition())) {
                    score->add(10);
                    it = pellets.erase(it);
                } else ++it;

            // --- Raccolta Super Pellet ---
            unsigned pacTileX = static_cast<unsigned>(pac.getPosition().x / tileSize.x);
            unsigned pacTileY = static_cast<unsigned>(pac.getPosition().y / tileSize.y);
            // Se la posizione è ancora in superPelletPositions, la raccogli
            auto it = std::find_if(superPelletPositions.begin(), superPelletPositions.end(),
                [&](const sf::Vector2f& pos) {
                    return (std::abs(pos.x - (pacTileX * tileSize.x + tileSize.x/2.f)) < 1e-2f &&
                            std::abs(pos.y - (pacTileY * tileSize.y + tileSize.y/2.f)) < 1e-2f);
                });
            if (it != superPelletPositions.end()) {
                superPelletPositions.erase(it);
                std::cout << "[DEBUG] Super Pellet raccolto a (" << pacTileX << ", " << pacTileY << ")\n";
                // TODO: Attiva modalità Frightened per tutti i fantasmi
                // for (auto& g : ghosts) g->setFrightened(...);
                // Attiva modalità Frightened per tutti i fantasmi
                for (auto& g : ghosts) g->setFrightened(frightenedBaseDuration);
            }

            // Se tutti i pellet sono stati raccolti, mostra messaggio e passa al livello successivo
            if (pellets.empty()) {
                currentLevel++;
                if (currentLevel >= (int)mapFiles.size()) {
                    // Tutte le mappe completate!
                    showMessage(window, "CONGRATULATIONS!\n\nALL LEVELS COMPLETED!\n\nDIFFICULTY INCREASED!", fontPath.string());
                    difficultyLevel++;
                    currentLevel = 0;
                    // Aumenta la velocità dei fantasmi per la nuova difficoltà
                    ghostBaseSpeed = 90.f * (1.0f + 0.1f * (difficultyLevel-1));
                    // Diminuisci frightened (minimo 1.5s)
                    frightenedBaseDuration = std::max<float>(minFrightened, 6.0f * std::pow(0.9f, float(difficultyLevel-1)));
                    // Diminuisci tempi di rilascio (minimo 0.5s, il primo resta 0)
                    for (size_t i = 1; i < ghostReleaseDelays.size(); ++i) {
                        ghostReleaseDelays[i] = std::max<float>(minRelease, 3.f * std::pow(0.9f, float(difficultyLevel-1)));
                    }
                } else {
                    showMessage(window, "LEVEL COMPLETE!\n\nWELL DONE!", fontPath.string());
                }
                loadLevel(currentLevel);
                gameOver = true;
            }

            // Collisione Pac-Man / Fantasmi
            for (const auto& ghost : ghosts) {
                float dist = (pac.getPosition() - ghost->getPosition()).x * (pac.getPosition() - ghost->getPosition()).x +
                             (pac.getPosition() - ghost->getPosition()).y * (pac.getPosition() - ghost->getPosition()).y;
                float minDist = 24.f * 24.f; // raggio Pac-Man + raggio Ghost (approssimato)
                if (dist < minDist) {
                    if (ghost->isFrightened() && !ghost->isEaten()) {
                        ghost->setEaten(true);
                        score->add(200);
                        continue;
                    } else if (!ghost->isEaten() && !ghost->isReturningToHouse()) {
                        showMessage(window, "GAME OVER!\n\nPAC-MAN WAS CAUGHT!\n\nTRY AGAIN!", fontPath.string());
                        // Ricarica il livello corrente invece di chiudere il gioco
                        loadLevel(currentLevel);
                        gameOver = true;
                    }
                }
            }
        }
        // Dopo il reset, attendi che il giocatore prema una freccia per ripartire
        if (gameOver) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down)) {
                gameOver = false;
                gameStarted = false;
            }
        }
        // Avvia il gioco solo dopo la prima mossa di Pac-Man
        if (!gameStarted && (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left) ||
                             sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right) ||
                             sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up) ||
                             sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down))) {
            gameStarted = true;
        }

        // Rendering
        window.clear();
        window.draw(map);
        // Prima i pellet, poi i Super Pellet grandi, poi i fantasmi, poi Pac-Man sopra tutto
        for (auto& p : pellets) window.draw(p);
        // Disegna i Super Pellet come cerchi grandi
        for (const auto& pos : superPelletPositions) {
            sf::CircleShape superPellet(12.f); // raggio 12px (triplo del pellet normale)
            superPellet.setOrigin(sf::Vector2f(12.f, 12.f));
            superPellet.setPosition(pos);
            superPellet.setFillColor(sf::Color(255, 192, 203)); // rosa chiaro
            window.draw(superPellet);
        }
        for (auto& g : ghosts) window.draw(*g);
        window.draw(pac);
        score->draw(window);
        window.display();
    }

    return 0;
}

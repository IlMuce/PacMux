#include <SFML/Graphics.hpp>
#include <Windows.h>          // Per gestione path e messaggi di errore
#include <filesystem>
#include <vector>
#include <memory>
#include <string>
#include <iostream>          // Per il log di debug

#include "TileMap.hpp"
#include "Player.hpp"
#include "Pellet.hpp"
#include "Score.hpp"
#include "Blinky.hpp"
#include "Pinky.hpp"
#include "Inky.hpp"
#include "Clyde.hpp"

int main()
{
    namespace fs = std::filesystem;

    // Recupera la cartella dell'eseguibile e degli asset
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    fs::path exeDir = fs::path(buf).parent_path();
    fs::path assets = exeDir / "assets";
    fs::path mapPath = assets / "map1.txt";
    fs::path fontPath = assets / "arial.ttf";

    // Verifica la presenza degli asset fondamentali
    if (!fs::exists(mapPath)) {
        MessageBoxA(NULL, ("Mappa non trovata:\n" + mapPath.string()).c_str(),
                    "Errore Pacman", MB_OK|MB_ICONERROR);
        return EXIT_FAILURE;
    }
    if (!fs::exists(fontPath)) {
        MessageBoxA(NULL, ("Font non trovato:\n" + fontPath.string()).c_str(),
                    "Errore Pacman", MB_OK|MB_ICONERROR);
        return EXIT_FAILURE;
    }

    // Configura la finestra di gioco
    const sf::Vector2u tileSize{32,32};
    TileMap map;
    if (!map.load(mapPath.string(), tileSize)) {
        MessageBoxA(NULL, ("Errore caricamento mappa:\n"+mapPath.string()).c_str(),
                    "Errore Pacman", MB_OK|MB_ICONERROR);
        return EXIT_FAILURE;
    }
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

    sf::RenderWindow window(mode, "Pacman Release 1");
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
    } catch (const std::exception& e) {
        MessageBoxA(NULL, e.what(), "Errore Pacman", MB_OK|MB_ICONERROR);
        return EXIT_FAILURE;
    }

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
            MessageBoxA(NULL, ("Errore caricamento mappa:\n"+mapPath.string()).c_str(),
                        "Errore Pacman", MB_OK|MB_ICONERROR);
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
    std::vector<std::string> mapFiles = {"map1.txt", "map2.txt"}; // Aggiungi qui altre mappe
    int currentLevel = 0;
    float ghostBaseSpeed = 90.f;
    float frightenedBaseDuration = 6.0f;
    
    // --- GESTIONE RELEASE SEMPLICE E SEQUENZIALE DEI FANTASMI ---
    // Il primo esce subito, poi ogni X secondi il prossimo
    std::vector<float> ghostReleaseDelays = {0.f, 3.f, 3.f, 3.f}; // solo il primo delay è assoluto, gli altri sono intervalli tra un rilascio e il successivo
    int nextGhostToRelease = 0;
    float ghostReleaseTimer = 0.f;
    
    auto loadLevel = [&](int levelIdx) {
        if (levelIdx >= (int)mapFiles.size()) {
            MessageBoxA(NULL, "Hai completato tutti i livelli! Congratulazioni!", "Game Completed", MB_OK|MB_ICONINFORMATION);
            currentLevel = 0;
            levelIdx = 0;
        }
        mapPath = assets / mapFiles[levelIdx];
        if (!map.load(mapPath.string(), tileSize)) {
            MessageBoxA(NULL, ("Mappa non trovata:\n" + mapPath.string()).c_str(), "Errore Pacman", MB_OK|MB_ICONERROR);
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
        // Aggiorna velocità e frightened in base al livello
        float speed = ghostBaseSpeed + 10.f * levelIdx;
        float frightenedDuration = std::max(2.0f, frightenedBaseDuration - 0.5f * levelIdx);
        for (auto& g : ghosts) {
            g->setSpeed(speed);
            g->setFrightened(0.f);
            g->setEaten(false);
            g->setReleased(false); // Ensure all ghosts start unreleased
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
                for (auto& g : ghosts) g->setFrightened(std::max(2.0f, frightenedBaseDuration - 0.5f * currentLevel));
            }

            // Se tutti i pellet sono stati raccolti, mostra messaggio e passa al livello successivo
            if (pellets.empty()) {
                MessageBoxA(NULL, "Hai raccolto tutti i pellet! Avanti al prossimo livello.", "Level Complete", MB_OK|MB_ICONINFORMATION);
                currentLevel++;
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
                        // Pac-Man mangia il fantasma
                        ghost->setEaten(true);
                        // Aumenta il punteggio (es: 200 punti per fantasma)
                        score->add(200);
                        continue;
                    } else if (!ghost->isEaten() && !ghost->isReturningToHouse()) {
                        MessageBoxA(NULL, "Game Over! Pac-Man ha incontrato un fantasma. Premi OK per ripartire.", "Game Over", MB_OK|MB_ICONERROR);
                        pac = Player(120.f, startPos, tileSize);
                        score = std::make_unique<Score>(fontPath.string());
                        // Reset posizione fantasmi
                        for (size_t i = 0; i < ghosts.size(); ++i) {
                            ghosts[i]->setPosition(ghostStartPos[i]);
                            ghosts[i]->setFrightened(0.f); // Reset frightened state
                            ghosts[i]->setEaten(false);    // Reset eaten state
                            ghosts[i]->setReleased(false); // Reset released state
                        }
                        // Reset ghost release state sequenziale
                        nextGhostToRelease = 0;
                        ghostReleaseTimer = 0.f;
                        gameOver = true;
                        break;
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

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
        sf::Vector2f(10*tileSize.x+tileSize.x/2.f, 8*tileSize.y+tileSize.y/2.f),  // Blinky (centro, riga 9)
        sf::Vector2f(10*tileSize.x+tileSize.x/2.f, 10*tileSize.y+tileSize.y/2.f), // Pinky (centro, riga 11) 
        sf::Vector2f(11*tileSize.x+tileSize.x/2.f, 10*tileSize.y+tileSize.y/2.f),  // Inky (sinistra, riga 11)
        sf::Vector2f(9*tileSize.x+tileSize.x/2.f, 10*tileSize.y+tileSize.y/2.f)  // Clyde (destra, riga 11)
    };
    
    ghosts.push_back(std::make_unique<Blinky>(ghostStartPos[0]));
    ghosts.push_back(std::make_unique<Pinky>(ghostStartPos[1]));
    ghosts.push_back(std::make_unique<Inky>(ghostStartPos[2]));
    ghosts.push_back(std::make_unique<Clyde>(ghostStartPos[3]));

    // --- TIMER MODALITÀ GHOSTS ---
    enum class GhostMode { Scatter, Chase };
    GhostMode ghostMode = GhostMode::Chase; // INIZIA DIRETTAMENTE IN CHASE PER TEST
    float modeTimer = 0.f;
    int modePhase = 0;
    // Tabella classica: scatter/chase (in secondi) - DISABILITATA PER TEST
    const std::vector<float> scatterChaseTimes = {
        7.f, 20.f, 7.f, 20.f, 5.f, 20.f, 5.f, -1.f // -1 = chase infinito
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

    // Game loop principale
    sf::Clock clock;
    bool gameOver = false;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        // --- Modalità ghosts --- DISABILITATO PER TEST CHASE MODE
        modeJustChanged = false;
        // Commentiamo temporaneamente il cambio automatico di modalità
        /*
        if (modePhase < (int)scatterChaseTimes.size() && scatterChaseTimes[modePhase] > 0.f) {
            modeTimer += dt;
            if (modeTimer >= scatterChaseTimes[modePhase]) {
                modeTimer = 0.f;
                modePhase++;
                ghostMode = (ghostMode == GhostMode::Scatter) ? GhostMode::Chase : GhostMode::Scatter;
                modeJustChanged = true;
            }
        }
        */
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

            // Aggiorna i fantasmi con la nuova architettura
            for (size_t i = 0; i < ghosts.size(); ++i) {
                Ghost::Mode m = (ghostMode == GhostMode::Scatter) ? Ghost::Mode::Scatter : Ghost::Mode::Chase;
                // Blinky = 0, Pinky = 1, Inky = 2, Clyde = 3
                if (auto* inky = dynamic_cast<Inky*>(ghosts[i].get())) {
                    // Passa la posizione di Blinky a Inky
                    if (ghosts.size() > 0) {
                        inky->update(dt, map, tileSize, pac.getPosition(), pac.getDirection(), m, ghosts[0]->getPosition());
                    }
                } else {
                    ghosts[i]->update(dt, map, tileSize, pac.getPosition(), pac.getDirection(), m);
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
                for (auto& g : ghosts) g->setFrightened(6.0f); // 6 secondi di frightened
            }

            // Se tutti i pellet sono stati raccolti, mostra messaggio e resetta
            if (pellets.empty()) {
                MessageBoxA(NULL, "Hai raccolto tutti i pellet! Premi OK per ripartire.", "You Win!", MB_OK|MB_ICONINFORMATION);
                pac = Player(120.f, startPos, tileSize);
                score = std::make_unique<Score>(fontPath.string());
                resetPelletsAndSuperPellets();
                // Reset posizione fantasmi
                for (size_t i = 0; i < ghosts.size(); ++i) {
                    ghosts[i]->setPosition(ghostStartPos[i]);
                }
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
                        resetPelletsAndSuperPellets();
                        // Reset posizione fantasmi
                        for (size_t i = 0; i < ghosts.size(); ++i) {
                            ghosts[i]->setPosition(ghostStartPos[i]);
                            ghosts[i]->setFrightened(0.f); // Reset frightened state
                            ghosts[i]->setEaten(false);    // Reset eaten state
                        }
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
            }
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

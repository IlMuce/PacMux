#include <SFML/Graphics.hpp>
#include <filesystem>
#include <vector>
#include <memory>
#include <string>
#include <iostream>          // Per il log di debug
#include <array>
#include <cstdint>
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
            for (auto& g : ghosts) {
                Ghost::Mode m = (ghostMode == GhostMode::Scatter) ? Ghost::Mode::Scatter : Ghost::Mode::Chase;
                g->update(dt, map, tileSize, pac.getPosition(), pac.getDirection(), m);
                // TODO: Implementare reverse al cambio modalità se necessario
                // if (modeJustChanged) {
                //     g->setDirection(-g->getDirection());
                // }
            }

            // Controlla collisione con i pellet
            for (auto it = pellets.begin(); it != pellets.end(); )
                if (it->eaten(pac.getPosition())) {
                    score->add(10);
                    it = pellets.erase(it);
                } else ++it;

            // Se tutti i pellet sono stati raccolti, mostra messaggio e resetta
            if (pellets.empty()) {
                showDialog("You Win!", "Hai raccolto tutti i pellet! Premi OK per ripartire.", false);
                pac = Player(120.f, startPos, tileSize);
                score = std::make_unique<Score>(fontPath.string());
                pellets.clear();
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
                    }
                }
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
                    showDialog("Game Over", "Game Over! Pac-Man ha incontrato un fantasma. Premi OK per ripartire.", true);
                    pac = Player(120.f, startPos, tileSize);
                    score = std::make_unique<Score>(fontPath.string());
                    pellets.clear();
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
                        }
                    }
                    // Reset posizione fantasmi
                    for (size_t i = 0; i < ghosts.size(); ++i) {
                        ghosts[i]->setPosition(ghostStartPos[i]);
                    }
                    gameOver = true;
                    break;
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
        // Prima i pellet, poi i fantasmi, poi Pac-Man sopra tutto
        for (auto& p : pellets) window.draw(p);
        for (auto& g : ghosts) window.draw(*g);
        window.draw(pac);
        score->draw(window);
        window.display();
    }

    return 0;
}

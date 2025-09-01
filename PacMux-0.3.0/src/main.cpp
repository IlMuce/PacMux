#include <SFML/Graphics.hpp>
#include <filesystem>
#include <vector>
#include <memory>
#include <string>
#include <iostream>
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
#include "Ghost.hpp"

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
    sf::VideoMode mode(sf::Vector2u{800,600}, 32);
    sf::RenderWindow window(mode, "Pacman Release 1");
    window.setFramerateLimit(60);

    // Carica la mappa dal file
    TileMap map;
    if (!map.load(mapPath.string(), tileSize)) { showDialog("Errore Pacman", "Errore caricamento mappa:\n"+mapPath.string(), true); return EXIT_FAILURE; }
    auto mapSz = map.getSize();

    // Trova la posizione di spawn di Pac-Man ('P') o usa il centro
    sf::Vector2f startPos{
        (mapSz.x * tileSize.x) / 2.f,
        (mapSz.y * tileSize.y) / 2.f
    };
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

    // Genera tutti i pellet sulle celle libere, ESCLUDENDO la ghost house centrale (righe 8-10, colonne 8-10)
    // e la cella di spawn di Pac-Man
    std::vector<Pellet> pellets;
    for (unsigned y = 0; y < mapSz.y; ++y) {
        for (unsigned x = 0; x < mapSz.x; ++x) {
            // Escludi la ghost house centrale
            bool inGhostHouse = (y >= 8 && y <= 10 && x >= 8 && x <= 10);
            // Escludi la cella di spawn di Pac-Man
            sf::Vector2f pos{
                x*float(tileSize.x)+tileSize.x/2.f,
                y*float(tileSize.y)+tileSize.y/2.f
            };
            bool isPacmanSpawn = (std::abs(pos.x - startPos.x) < 1e-2f && std::abs(pos.y - startPos.y) < 1e-2f);
            if (!map.isWall(x,y) && !inGhostHouse && !isPacmanSpawn) {
                pellets.emplace_back(pos);
            }
        }
    }

    // Crea i fantasmi statici (posizionati nella ghost house)
    std::vector<Ghost> ghosts;
    ghosts.emplace_back(sf::Vector2f(9*tileSize.x+tileSize.x/2.f, 10*tileSize.y+tileSize.y/2.f), sf::Color::Red);
    ghosts.emplace_back(sf::Vector2f(8*tileSize.x+tileSize.x/2.f, 9*tileSize.y+tileSize.y/2.f), sf::Color::Cyan);
    ghosts.emplace_back(sf::Vector2f(10*tileSize.x+tileSize.x/2.f, 9*tileSize.y+tileSize.y/2.f), sf::Color(255,184,255));
    ghosts.emplace_back(sf::Vector2f(9*tileSize.x+tileSize.x/2.f, 8*tileSize.y+tileSize.y/2.f), sf::Color(255,184,82));

    // Game loop principale
    sf::Clock clock;
    bool gameOver = false;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        // Gestione eventi finestra
        while (auto ev = window.pollEvent())
            if (ev->is<sf::Event::Closed>())
                window.close();

        if (!gameOver) {
            // Aggiorna il giocatore
            pac.update(dt, map, tileSize);

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
                        bool inGhostHouse = (y >= 8 && y <= 10 && x >= 8 && x <= 10);
                        sf::Vector2f pos{
                            x*float(tileSize.x)+tileSize.x/2.f,
                            y*float(tileSize.y)+tileSize.y/2.f
                        };
                        bool isPacmanSpawn = (std::abs(pos.x - startPos.x) < 1e-2f && std::abs(pos.y - startPos.y) < 1e-2f);
                        if (!map.isWall(x,y) && !inGhostHouse && !isPacmanSpawn) {
                            pellets.emplace_back(pos);
                        }
                    }
                }
                gameOver = true;
            }

            // Collisione Pac-Man / Fantasmi
            for (const auto& ghost : ghosts) {
                float dist = (pac.getPosition() - ghost.getPosition()).x * (pac.getPosition() - ghost.getPosition()).x +
                             (pac.getPosition() - ghost.getPosition()).y * (pac.getPosition() - ghost.getPosition()).y;
                float minDist = 24.f * 24.f; // raggio Pac-Man + raggio Ghost (approssimato)
                if (dist < minDist) {
                    showDialog("Game Over", "Game Over! Pac-Man ha incontrato un fantasma. Premi OK per ripartire.", true);
                    // Reset: riposiziona Pac-Man allo spawn, resetta punteggio e pellet
                    pac = Player(120.f, startPos, tileSize);
                    score = std::make_unique<Score>(fontPath.string());
                    pellets.clear();
                    for (unsigned y = 0; y < mapSz.y; ++y) {
                        for (unsigned x = 0; x < mapSz.x; ++x) {
                            bool inGhostHouse = (y >= 8 && y <= 10 && x >= 8 && x <= 10);
                            sf::Vector2f pos{
                                x*float(tileSize.x)+tileSize.x/2.f,
                                y*float(tileSize.y)+tileSize.y/2.f
                            };
                            bool isPacmanSpawn = (std::abs(pos.x - startPos.x) < 1e-2f && std::abs(pos.y - startPos.y) < 1e-2f);
                            if (!map.isWall(x,y) && !inGhostHouse && !isPacmanSpawn) {
                                pellets.emplace_back(pos);
                            }
                        }
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
        window.draw(pac);
        for (auto& g : ghosts) window.draw(g);
        for (auto& p : pellets) window.draw(p);
        score->draw(window);
        window.display();
    }

    return 0;
}

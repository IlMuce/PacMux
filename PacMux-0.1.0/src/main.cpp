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

namespace {
    namespace fs = std::filesystem;

    static fs::path getExecutableDir() {
#if defined(_WIN32)
        // Use WinAPI only when building on Windows
        char buf[32768];
        unsigned long len = GetModuleFileNameA(nullptr, buf, static_cast<unsigned long>(sizeof(buf)));
        if (len == 0 || len >= sizeof(buf)) {
            return fs::current_path();
        }
        return fs::path(buf).parent_path();
#elif defined(__APPLE__)
        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);
        std::string tmp(size, '\0');
        if (_NSGetExecutablePath(tmp.data(), &size) == 0) {
            return fs::path(tmp.c_str()).parent_path();
        }
        return fs::current_path();
#elif defined(__linux__)
        std::array<char, 4096> buf{};
        ssize_t count = readlink("/proc/self/exe", buf.data(), buf.size() - 1);
        if (count > 0) {
            buf[static_cast<size_t>(count)] = '\0';
            return fs::path(buf.data()).parent_path();
        }
        return fs::current_path();
#else
        return fs::current_path();
#endif
    }

    static void showDialog(const std::string& title, const std::string& message, bool errorIcon = true) {
#if defined(_WIN32)
        MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | (errorIcon ? MB_ICONERROR : MB_ICONINFORMATION));
#else
        std::cerr << title << ": " << message << std::endl;
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
    if (!fs::exists(mapPath)) {
        showDialog("Errore Pacman", "Mappa non trovata:\n" + mapPath.string(), true);
        return EXIT_FAILURE;
    }
    if (!fs::exists(fontPath)) {
        showDialog("Errore Pacman", "Font non trovato:\n" + fontPath.string(), true);
        return EXIT_FAILURE;
    }

    // Configura la finestra di gioco
    const sf::Vector2u tileSize{32,32};
    sf::VideoMode mode(sf::Vector2u{800,600}, 32);
    sf::RenderWindow window(mode, "Pacman Release 1");
    window.setFramerateLimit(60);

    // Carica la mappa dal file
    TileMap map;
    if (!map.load(mapPath.string(), tileSize)) {
        showDialog("Errore Pacman", "Errore caricamento mappa:\n" + mapPath.string(), true);
        return EXIT_FAILURE;
    }
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
    } catch (const std::exception& e) {
        showDialog("Errore Pacman", e.what(), true);
        return EXIT_FAILURE;
    }

    // Crea il giocatore (Pac-Man)
    Player pac(120.f, startPos, tileSize);

    // Genera tutti i pellet sulle celle libere
    std::vector<Pellet> pellets;
    for (unsigned y = 0; y < mapSz.y; ++y) {
        for (unsigned x = 0; x < mapSz.x; ++x) {
            if (!map.isWall(x,y)) {
                sf::Vector2f pos{
                    x*float(tileSize.x)+tileSize.x/2.f,
                    y*float(tileSize.y)+tileSize.y/2.f
                };
                pellets.emplace_back(pos);
            }
        }
    }

    // Game loop principale
    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        // Gestione eventi finestra
        while (auto ev = window.pollEvent())
            if (ev->is<sf::Event::Closed>())
                window.close();

        // Aggiorna il giocatore
        pac.update(dt, map, tileSize);

        // Controlla collisione con i pellet
        for (auto it = pellets.begin(); it != pellets.end(); )
            if (it->eaten(pac.getPosition())) {
                score->add(10);
                it = pellets.erase(it);
            } else ++it;

        // Rendering
        window.clear();
        window.draw(map);
        window.draw(pac);
        for (auto& p : pellets) window.draw(p);
        score->draw(window);
        window.display();
    }

    return 0;
}

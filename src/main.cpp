// src/main.cpp
#include <SFML/Graphics.hpp>
#include <Windows.h>          // GetModuleFileNameA, MessageBoxA
#include <filesystem>         // C++17
#include <vector>
#include <memory>
#include <string>

#include "TileMap.hpp"
#include "Player.hpp"
#include "Pellet.hpp"
#include "Score.hpp"

int main()
{
    namespace fs = std::filesystem;

    // 1) Trova la directory dell'eseguibile
    char exePathBuf[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        MessageBoxA(NULL,
            "Impossibile determinare il percorso dell'eseguibile.",
            "Errore Pacman",
            MB_OK | MB_ICONERROR
        );
        return EXIT_FAILURE;
    }
    fs::path exePath(exePathBuf);
    fs::path exeDir = exePath.parent_path();

    // 2) Costruisci il path assets sempre relativo all'EXE
    fs::path assetsDir = exeDir / "assets";
    fs::path mapPath   = assetsDir / "map1.txt";
    fs::path fontPath  = assetsDir / "arial.ttf";

    // 3) Verifica asset
    if (!fs::exists(mapPath)) {
        MessageBoxA(NULL,
            ("Mappa non trovata:\n" + mapPath.string()).c_str(),
            "Errore Pacman", MB_OK | MB_ICONERROR
        );
        return EXIT_FAILURE;
    }
    if (!fs::exists(fontPath)) {
        MessageBoxA(NULL,
            ("Font non trovato:\n" + fontPath.string()).c_str(),
            "Errore Pacman", MB_OK | MB_ICONERROR
        );
        return EXIT_FAILURE;
    }

    // 4) Setup finestra
    const sf::Vector2u tileSize{32, 32};
    sf::VideoMode mode(sf::Vector2u{800, 600}, 32);
    sf::RenderWindow window(mode, "Pacman Release 1");
    window.setFramerateLimit(60);

    // 5) Carica labirinto
    TileMap map;
    if (!map.load(mapPath.string(), tileSize)) {
        MessageBoxA(NULL,
            ("Errore caricamento mappa:\n" + mapPath.string()).c_str(),
            "Errore Pacman", MB_OK | MB_ICONERROR
        );
        return EXIT_FAILURE;
    }

    // 6) Inizializza punteggio
    std::unique_ptr<Score> score;
    try {
        score = std::make_unique<Score>(fontPath.string());
    } catch (const std::exception& e) {
        MessageBoxA(NULL,
            e.what(),
            "Errore Pacman", MB_OK | MB_ICONERROR
        );
        return EXIT_FAILURE;
    }

    // 7) Crea player e pellet
    Player pac(120.f, {400.f, 300.f}, tileSize);
    std::vector<Pellet> pellets;
    auto mapSz = map.getSize();
    for (unsigned y = 0; y < mapSz.y; ++y)
        for (unsigned x = 0; x < mapSz.x; ++x)
            if (!map.isWall(x, y))
                pellets.emplace_back(
                    sf::Vector2f(
                        x * float(tileSize.x) + tileSize.x / 2.f,
                        y * float(tileSize.y) + tileSize.y / 2.f
                    )
                );

    // 8) Game loop
    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        // Eventi SFML 3
        while (auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>())
                window.close();
        }

        // Update e collisioni
        pac.update(dt, map, tileSize);
        for (auto it = pellets.begin(); it != pellets.end(); ) {
            if (it->eaten(pac.getPosition())) {
                score->add(10);
                it = pellets.erase(it);
            } else ++it;
        }

        // Render
        window.clear();
        window.draw(map);
        window.draw(pac);
        for (auto& p : pellets) window.draw(p);
        score->draw(window);
        window.display();
    }

    return 0;
}

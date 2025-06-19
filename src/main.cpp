#include <SFML/Graphics.hpp>
#include <Windows.h>          // Per gestione path e messaggi di errore
#include <filesystem>
#include <vector>
#include <memory>
#include <string>

#include "TileMap.hpp"
#include "Player.hpp"
#include "Pellet.hpp"
#include "Score.hpp"
#include "Ghost.hpp"

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
    sf::VideoMode mode(sf::Vector2u{800,600}, 32);
    sf::RenderWindow window(mode, "Pacman Release 1");
    window.setFramerateLimit(60);

    // Carica la mappa dal file
    TileMap map;
    if (!map.load(mapPath.string(), tileSize)) {
        MessageBoxA(NULL, ("Errore caricamento mappa:\n"+mapPath.string()).c_str(),
                    "Errore Pacman", MB_OK|MB_ICONERROR);
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
        MessageBoxA(NULL, e.what(), "Errore Pacman", MB_OK|MB_ICONERROR);
        return EXIT_FAILURE;
    }

    // Crea il giocatore (Pac-Man)
    Player pac(120.f, startPos, tileSize);

    // Genera tutti i pellet sulle celle libere, ESCLUDENDO la ghost house centrale (righe 8-10, colonne 8-10)
    std::vector<Pellet> pellets;
    for (unsigned y = 0; y < mapSz.y; ++y) {
        for (unsigned x = 0; x < mapSz.x; ++x) {
            // Escludi la ghost house centrale
            bool inGhostHouse = (y >= 8 && y <= 10 && x >= 8 && x <= 10);
            if (!map.isWall(x,y) && !inGhostHouse) {
                sf::Vector2f pos{
                    x*float(tileSize.x)+tileSize.x/2.f,
                    y*float(tileSize.y)+tileSize.y/2.f
                };
                pellets.emplace_back(pos);
            }
        }
    }

    // Crea i fantasmi statici (posizionati nella ghost house)
    std::vector<Ghost> ghosts;
    ghosts.emplace_back(sf::Vector2f(9*tileSize.x+tileSize.x/2.f, 9*tileSize.y+tileSize.y/2.f), sf::Color::Red);
    ghosts.emplace_back(sf::Vector2f(8*tileSize.x+tileSize.x/2.f, 9*tileSize.y+tileSize.y/2.f), sf::Color::Cyan);
    ghosts.emplace_back(sf::Vector2f(10*tileSize.x+tileSize.x/2.f, 9*tileSize.y+tileSize.y/2.f), sf::Color(255,184,255));
    ghosts.emplace_back(sf::Vector2f(9*tileSize.x+tileSize.x/2.f, 8*tileSize.y+tileSize.y/2.f), sf::Color(255,184,82));

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

        // Collisione Pac-Man / Fantasmi
        for (const auto& ghost : ghosts) {
            float dist = (pac.getPosition() - ghost.getPosition()).x * (pac.getPosition() - ghost.getPosition()).x +
                         (pac.getPosition() - ghost.getPosition()).y * (pac.getPosition() - ghost.getPosition()).y;
            float minDist = 24.f * 24.f; // raggio Pac-Man + raggio Ghost (approssimato)
            if (dist < minDist) {
                MessageBoxA(NULL, "Game Over! Pac-Man ha incontrato un fantasma.", "Game Over", MB_OK|MB_ICONERROR);
                window.close();
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

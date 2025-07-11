#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <Windows.h>          // Per gestione path e messaggi di errore
#include <filesystem>
#include <vector>
#include <memory>
#include <string>
#include <iostream>          // Per il log di debug
#include <cstdint>           // Per std::uint32_t
#include <cctype>            // Per std::isalnum, std::toupper

#include "TileMap.hpp"
#include "Player.hpp"
#include "Pellet.hpp"
#include "Score.hpp"
#include "HighScore.hpp"
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
    
    // Clock per il lampeggiamento - resettato a ogni chiamata per visibilità immediata
    sf::Clock blinkClock;
    
    bool waiting = true;
    while (waiting) {
        window.clear(sf::Color::Black);
        window.draw(background);
        window.draw(text);
        
        // Aggiungi "PRESS ENTER" lampeggiante - ora sempre visibile all'inizio
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

// Funzione per inserire il nome del giocatore per un nuovo record
std::string inputPlayerName(sf::RenderWindow& window, const std::string& fontPath, unsigned int finalScore) {
    sf::Font font(fontPath);
    std::string playerName;
    sf::Clock blinkClock;
    bool showCursor = true;
    
    while (true) { // Ciclo infinito controllato internamente
        window.clear(sf::Color::Black);
        
        // Titolo
        sf::Text titleText(font, "NUOVO RECORD!", 36);
        titleText.setFillColor(sf::Color::Yellow);
        titleText.setOutlineColor(sf::Color::Red);
        titleText.setOutlineThickness(2);
        sf::FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setPosition(sf::Vector2f((window.getSize().x - titleBounds.size.x) / 2.0f, window.getSize().y * 0.2f));
        window.draw(titleText);
        
        // Punteggio
        sf::Text scoreText(font, "Punteggio: " + std::to_string(finalScore), 24);
        scoreText.setFillColor(sf::Color::Cyan);
        sf::FloatRect scoreBounds = scoreText.getLocalBounds();
        scoreText.setPosition(sf::Vector2f((window.getSize().x - scoreBounds.size.x) / 2.0f, window.getSize().y * 0.35f));
        window.draw(scoreText);
        
        // Prompt
        sf::Text promptText(font, "Inserisci il tuo nome:", 20);
        promptText.setFillColor(sf::Color::White);
        sf::FloatRect promptBounds = promptText.getLocalBounds();
        promptText.setPosition(sf::Vector2f((window.getSize().x - promptBounds.size.x) / 2.0f, window.getSize().y * 0.5f));
        window.draw(promptText);
        
        // Nome corrente + cursore lampeggiante
        std::string displayName = playerName;
        if (showCursor && blinkClock.getElapsedTime().asSeconds() < 0.5f) {
            displayName += "_";
        } else if (blinkClock.getElapsedTime().asSeconds() >= 1.0f) {
            blinkClock.restart();
            showCursor = !showCursor;
        }
        
        sf::Text nameText(font, displayName, 24);
        nameText.setFillColor(sf::Color::Green);
        sf::FloatRect nameBounds = nameText.getLocalBounds();
        nameText.setPosition(sf::Vector2f((window.getSize().x - nameBounds.size.x) / 2.0f, window.getSize().y * 0.6f));
        window.draw(nameText);
        
        // Istruzioni
        sf::Text instructText(font, "Premi INVIO per confermare", 16);
        instructText.setFillColor(sf::Color(128, 128, 128));
        sf::FloatRect instructBounds = instructText.getLocalBounds();
        instructText.setPosition(sf::Vector2f((window.getSize().x - instructBounds.size.x) / 2.0f, window.getSize().y * 0.8f));
        window.draw(instructText);
        
        window.display();
        
        auto event = window.waitEvent();
        if (!event) continue;
        
        if (event->is<sf::Event::KeyPressed>()) {
            if (auto keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->code == sf::Keyboard::Key::Enter) {
                    if (playerName.empty()) {
                        playerName = "PLAYER"; // Nome predefinito se vuoto
                    }
                    return playerName;
                } else if (keyEvent->code == sf::Keyboard::Key::Backspace) {
                    if (!playerName.empty()) {
                        playerName.pop_back();
                    }
                }
            }
        } else if (event->is<sf::Event::TextEntered>()) {
            if (auto textEvent = event->getIf<sf::Event::TextEntered>()) {
                std::uint32_t unicode = textEvent->unicode;
                // Accetta solo caratteri alfanumerici e spazi (max 10 caratteri)
                if (unicode >= 32 && unicode < 127 && unicode != 127 && playerName.length() < 10) { // ASCII stampabili escluso DEL
                    char character = static_cast<char>(unicode);
                    if (std::isalnum(character) || character == ' ') {
                        playerName += std::toupper(character); // Converti in maiuscolo
                    }
                }
            }
        } else if (event->is<sf::Event::Closed>()) {
            window.close();
            return "PLAYER";
        }
    }
    
    return playerName.empty() ? "PLAYER" : playerName;
}

int main()
{
    namespace fs = std::filesystem;

    // Recupera la cartella dell'eseguibile e degli asset
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    fs::path exeDir = fs::path(buf).parent_path();
    fs::path assets = exeDir / "assets";
    fs::path mapPath = assets / "map1.txt";
    fs::path fontPath = assets / "pacman.ttf";
    fs::path audioDir = assets / "audio";

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
    
    // Assicurati che la finestra sia abbastanza grande per i menu e l'HUD
    const unsigned minWidth = 800;
    const unsigned minHeight = 700;
    if (mode.size.x < minWidth) {
        mode.size.x = minWidth;
    }
    if (mode.size.y < minHeight) {
        mode.size.y = minHeight;
    }

    // Wrap-around per Pac-Man: correggi posizione se esce dai bordi
    // RIMOSSO: i controlli di wrap-around ora sono gestiti nella classe Player
    
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
    std::unique_ptr<HighScore> highScore;
    
    // Determina il percorso del file highscore (nella stessa directory dell'eseguibile)
    fs::path highscorePath = exeDir / "highscores.json";
    
    try {
        score = std::make_unique<Score>(fontPath.string());
        highScore = std::make_unique<HighScore>(fontPath.string());
        
        // Carica i record esistenti dal percorso corretto
        highScore->loadFromFile(highscorePath.string());
        
    } catch (const std::exception& e) {
        MessageBoxA(NULL, e.what(), "Errore Pacman", MB_OK|MB_ICONERROR);
        return EXIT_FAILURE;
    }

    // --- AUDIO: Caricamento effetti e musica ---
    sf::Music music;
    if (!music.openFromFile((audioDir / "pacman_beginning.wav").string())) {
        std::cerr << "[AUDIO] Errore caricamento musica di sottofondo!\n";
    }
    music.setLooping(false); // SFML 3: musica suona una volta sola
    // NON avviare la musica qui - sarà avviata quando inizia il gameplay

    sf::SoundBuffer bufChomp, bufChompMenu, bufEatGhost, bufDeath, bufMenu, bufGhostBlue, bufGhostReturn, bufGhostNormal;
    if (!bufChomp.loadFromFile((audioDir / "PacmanChomp.mp3").string())) {
        std::cerr << "[AUDIO] Errore caricamento effetto chomp!\n";
    }
    if (!bufChompMenu.loadFromFile((audioDir / "pacman_chomp.wav").string())) {
        std::cerr << "[AUDIO] Errore caricamento effetto chomp menu!\n";
    }
    if (!bufEatGhost.loadFromFile((audioDir / "pacman_eatghost.wav").string())) {
        std::cerr << "[AUDIO] Errore caricamento effetto eat ghost!\n";
    }
    if (!bufDeath.loadFromFile((audioDir / "pacman_death.wav").string())) {
        std::cerr << "[AUDIO] Errore caricamento effetto death!\n";
    }
    if (!bufMenu.loadFromFile((audioDir / "pacman_menupausa.wav").string())) {
        std::cerr << "[AUDIO] Errore caricamento effetto menu!\n";
    }
    if (!bufGhostBlue.loadFromFile((audioDir / "GhostTurntoBlue.mp3").string())) {
        std::cerr << "[AUDIO] Errore caricamento effetto ghost blue!\n";
    }
    if (!bufGhostReturn.loadFromFile((audioDir / "GhostReturntoHome.mp3").string())) {
        std::cerr << "[AUDIO] Errore caricamento effetto ghost return!\n";
    }
    if (!bufGhostNormal.loadFromFile((audioDir / "GhostNormalMove.mp3").string())) {
        std::cerr << "[AUDIO] Errore caricamento effetto ghost normal!\n";
    }

    // === RIEPILOGO DEGLI 8 SUONI IMPLEMENTATI ===
    // 1. pacman_beginning.wav - Musica di sottofondo (una volta per partita)
    // 2. PacmanChomp.mp3 - Suono "wakawakawaka" durante il gioco (loop continuo)
    // 3. pacman_chomp.wav - Suono per navigazione menu (breve, non in loop)
    // 4. pacman_eatghost.wav - Mangiare fantasmi + conferma selezioni menu
    // 5. pacman_death.wav - Morte di Pac-Man
    // 6. pacman_menupausa.wav - Suono pausa + ritorno al menu da Game Over
    // 7. GhostTurntoBlue.mp3 - Quando i fantasmi diventano blu (Super Pellet)
    // 8. GhostReturntoHome.mp3 - Fantasmi che tornano alla casa (dopo essere stati mangiati)
    // 9. GhostNormalMove.mp3 - Movimento normale dei fantasmi (loop continuo)

    sf::Sound sfxChomp(bufChomp), sfxChompMenu(bufChompMenu), sfxEatGhost(bufEatGhost), sfxDeath(bufDeath), sfxMenu(bufMenu), sfxGhostBlue(bufGhostBlue), sfxGhostReturn(bufGhostReturn), sfxGhostNormal(bufGhostNormal);

    // Imposta il chomp in loop per suono continuo come l'arcade originale
    sfxChomp.setLooping(true); // Il chomp sarà un loop continuo
    
    // Il suono menu chomp NON è in loop (perfetto per navigazione)
    // sfxChompMenu è già configurato per navigazione menu
    sfxGhostNormal.setLooping(true); // SFML 3: setLoop() → setLooping()
    sfxGhostReturn.setLooping(true); // Anche il suono di ritorno alla casa in loop
    
    // === CONTROLLO VOLUME AUDIO ===
    sfxChomp.setVolume(60.f); // Riduci volume chomp (0-100)
    sfxChompMenu.setVolume(60.f); // Volume per navigazione menu
    sfxGhostNormal.setVolume(25.f); // Volume molto ridotto fantasmi
    sfxGhostReturn.setVolume(35.f); // Volume ridotto per ritorno
    sfxGhostBlue.setVolume(80.f); // Volume normale per effetti speciali
    sfxEatGhost.setVolume(85.f);
    sfxDeath.setVolume(90.f);
    sfxMenu.setVolume(70.f); // Volume musica di sottofondo

    // Flag per controllare se la musica è già stata avviata
    bool musicStarted = false;
    
    // Flag per controllare se i fantasmi stanno suonando
    bool ghostSoundPlaying = false;
    bool canPlayGhostSounds = false; // I fantasmi possono suonare solo dopo la musica di inizio
    
    // Timer per gestire i diversi stati audio dei fantasmi
    sf::Clock ghostModeTimer;
    
    // Sistema audio avanzato - tracciamento degli stati
    static bool wasInGameOverState = false;
    static bool wasInMenuState = true;
    sf::Clock menuSoundCooldown; // Previene spam di suoni nel menu
    
    // Timer per il suono chomp - VERSIONE CON CONTROLLO VOLUME
    bool chompActive = false; // Indica se Pac-Man sta mangiando
    sf::Clock lastPelletTimer; // Timer globale per tracciare l'ultimo pellet mangiato
    bool chompSoundStarted = false; // Flag per sapere se il chomp è mai stato avviato

    // Crea il giocatore (Pac-Man)
    Player pac(120.f, startPos, tileSize);

    // Genera tutti i pellet sulle celle libere, ESCLUDENDO tile '2' e la cella di spawn di Pac-Man
    std::vector<Pellet> pellets;
    // --- Super Pellet positions ---
    std::vector<sf::Vector2f> superPelletPositions;
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
            if (tile == 'S') {
                superPelletPositions.emplace_back(pos);
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

    // --- GESTIONE STATI DI GIOCO ---
    enum class GameState { MENU, PLAYING, GAME_OVER, HIGHSCORE, PAUSED };
    GameState gameState = GameState::MENU;

    // --- GESTIONE MENU PRINCIPALE ---
    enum class MenuOption { PLAY = 0, HIGHSCORE = 1, EXIT = 2 };
    MenuOption selectedMenuOption = MenuOption::PLAY;
    const int NUM_MENU_OPTIONS = 3;

    // --- GESTIONE MENU PAUSA ---
    enum class PauseOption { RESUME = 0, BACK_TO_MENU = 1 };
    PauseOption selectedPauseOption = PauseOption::RESUME;
    const int NUM_PAUSE_OPTIONS = 2;

    // --- GESTIONE MULTI-LIVELLO ---
    std::vector<std::string> mapFiles = {"map1.txt", "map2.txt", "map3.txt"};
    int currentLevel = 0;
    bool allMapsCompleted = false;
    int difficultyLevel = 1;
    float ghostBaseSpeed = 90.f;
    float frightenedBaseDuration = 6.0f;
    std::vector<float> ghostReleaseDelays = {0.f, 3.f, 3.f, 3.f};
    const float minFrightened = 1.5f;
    const float minRelease = 0.5f;

    // --- GESTIONE RELEASE SEMPLICE E SEQUENZIALE DEI FANTASMI ---
    int nextGhostToRelease = 0;
    float ghostReleaseTimer = 0.f;
    
    auto loadLevel = [&](int levelIdx, bool resetPellets = true) {
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
        // Reset pellet e super pellet solo se richiesto
        if (resetPellets) {
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
        
        // Salva le vite attuali prima di creare un nuovo Player
        int currentLives = pac.getLives();
        pac = Player(120.f, startPos, tileSize);
        pac.setLives(currentLives); // Ripristina le vite salvate
        
        // NON resettare score qui - mantieni il punteggio tra i livelli
        // score = std::make_unique<Score>(fontPath.string());
    };

    // Game loop principale
    sf::Clock clock;
    bool gameOver = false;
    bool gameStarted = false;
    bool recordChecked = false; // Flag per controllare se il record è già stato verificato
    // --- VARIABILI PER PAUSA DOPO MANGIATO FANTASMA (combo classica) ---
    bool isGhostEatPause = false;
    sf::Clock ghostEatPauseClock;
    int ghostEatCombo = 0;
    int ghostEatScore = 0;
    constexpr float GHOST_EAT_PAUSE = 1.0f;
    sf::Vector2f pacmanDirBeforePause;
    std::vector<sf::Vector2f> ghostsDirBeforePause(4);

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        // --- Gestione stati di gioco ---
        if (gameState == GameState::GAME_OVER) {
            // Controlla se è stato raggiunto un nuovo record
            if (!recordChecked) {
                unsigned int finalScore = score->getScore();
                if (highScore->isHighScore(finalScore)) {
                    // Nuovo record! Chiedi il nome del giocatore
                    std::string playerName = inputPlayerName(window, fontPath.string(), finalScore);
                    highScore->addScore(playerName, finalScore);
                }
                recordChecked = true;
            }
            
            // Mostra schermata Game Over
            window.clear(sf::Color::Black);
            sf::Font font(fontPath.string());
            
            sf::Text gameOverText(font, "GAME OVER", 48);
            gameOverText.setFillColor(sf::Color::Red);
            gameOverText.setOutlineColor(sf::Color::White);
            gameOverText.setOutlineThickness(2);
            gameOverText.setPosition(sf::Vector2f(window.getSize().x * 0.18f, window.getSize().y * 0.15f));
            window.draw(gameOverText);
            
            unsigned int finalScore = score->getScore();
            sf::Text scoreText(font, "Punteggio finale: " + std::to_string(finalScore), 24);
            scoreText.setFillColor(sf::Color::Yellow);
            scoreText.setPosition(sf::Vector2f(window.getSize().x * 0.2f, window.getSize().y * 0.35f));
            window.draw(scoreText);
            
            // Mostra se è stato raggiunto un nuovo record
            if (highScore->isHighScore(finalScore) || finalScore == highScore->getTopScore()) {
                sf::Text recordText(font, "NUOVO RECORD!", 28);
                recordText.setFillColor(sf::Color(255, 215, 0)); // Oro
                recordText.setOutlineColor(sf::Color::Red);
                recordText.setOutlineThickness(2);
                sf::FloatRect recordBounds = recordText.getLocalBounds();
                recordText.setPosition(sf::Vector2f((window.getSize().x - recordBounds.size.x) / 2.0f, window.getSize().y * 0.42f));
                window.draw(recordText);
            }
            
            sf::Text restartText(font, "Premi INVIO per ricominciare", 20);
            restartText.setFillColor(sf::Color::White);
            restartText.setPosition(sf::Vector2f(window.getSize().x * 0.12f, window.getSize().y * 0.55f));
            window.draw(restartText);
            
            sf::Text menuText(font, "Premi M o ESC per tornare al menu", 20);
            menuText.setFillColor(sf::Color::Cyan);
            menuText.setPosition(sf::Vector2f(window.getSize().x * 0.08f, window.getSize().y * 0.65f));
            window.draw(menuText);
            
            sf::Text highscoreText(font, "Premi H per vedere i record", 20);
            highscoreText.setFillColor(sf::Color::Magenta);
            highscoreText.setPosition(sf::Vector2f(window.getSize().x * 0.12f, window.getSize().y * 0.75f));
            window.draw(highscoreText);
            
            window.display();
            
            // Gestione input Game Over
            auto event = window.waitEvent();
            if (event && event->is<sf::Event::KeyPressed>()) {
                if (auto keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                    if (keyEvent->code == sf::Keyboard::Key::Enter) {
                        sfxEatGhost.play(); // Suono di conferma restart
                        // Riavvia il gioco - RESETTA tutto
                        gameState = GameState::PLAYING;

                        // Avvia la musica di sottofondo solo se non è già stata avviata
                        if (!musicStarted) {
                            music.play();
                            musicStarted = true;
                        }
                        // Reset flag audio e suoni
                        canPlayGhostSounds = false;
                        ghostSoundPlaying = false;
                        chompActive = false;
                        chompSoundStarted = false;
                        sfxChomp.stop();
                        sfxChomp.setVolume(60.f); // Reset volume per il prossimo uso
                        sfxGhostNormal.stop();
                        sfxGhostReturn.stop();

                        gameOver = false;
                        gameStarted = false;
                        currentLevel = 0;
                        score->resetScore();
                        pac.setLives(3);
                        recordChecked = false; // Reset del flag per il prossimo game over
                        loadLevel(0);
                        continue;
                    } else if (keyEvent->code == sf::Keyboard::Key::M || keyEvent->code == sf::Keyboard::Key::Escape) {
                        sfxMenu.play(); // Suono di ritorno al menu
                        // Ferma tutti i suoni quando si torna al menu
                        sfxGhostNormal.stop();
                        sfxGhostReturn.stop();
                        ghostSoundPlaying = false;
                        canPlayGhostSounds = false;
                        chompActive = false;
                        sfxChomp.stop(); // Ferma completamente il chomp quando si torna al menu
                        chompSoundStarted = false; // Reset del flag
                        // Torna al menu - MANTIENI il punteggio per ora
                        recordChecked = false; // Reset del flag
                        gameState = GameState::MENU;
                        continue;
                    } else if (keyEvent->code == sf::Keyboard::Key::H) {
                        sfxEatGhost.play(); // Suono di navigazione
                        gameState = GameState::HIGHSCORE;
                        continue;
                    }
                }
            }
            if (event && event->is<sf::Event::Closed>()) {
                window.close();
                break;
            }
            continue;
        }

        if (gameState == GameState::MENU) {
            // Mostra menu principale
            window.clear(sf::Color::Black);
            sf::Font font(fontPath.string());
            
            // Titolo del gioco
            sf::Text title(font, "PACMAN", 48);
            title.setFillColor(sf::Color::Yellow);
            title.setOutlineColor(sf::Color::Blue);
            title.setOutlineThickness(3);
            title.setPosition(sf::Vector2f(window.getSize().x * 0.32f, window.getSize().y * 0.15f));
            window.draw(title);
            
            // Opzioni del menu
            std::vector<std::string> menuItems = {"GIOCA", "RECORD", "ESCI"};
            std::vector<sf::Color> menuColors = {sf::Color::White, sf::Color::White, sf::Color::White};
            
            // Evidenzia l'opzione selezionata
            menuColors[static_cast<int>(selectedMenuOption)] = sf::Color::Yellow;
            
            for (int i = 0; i < NUM_MENU_OPTIONS; ++i) {
                sf::Text menuText(font, menuItems[i], 24);
                menuText.setFillColor(menuColors[i]);
                if (i == static_cast<int>(selectedMenuOption)) {
                    menuText.setOutlineColor(sf::Color::Red);
                    menuText.setOutlineThickness(2);
                }
                
                float yPos = window.getSize().y * 0.4f + (i * 60.f);
                menuText.setPosition(sf::Vector2f(window.getSize().x * 0.4f, yPos));
                window.draw(menuText);
                
                // Freccia per l'opzione selezionata
                if (i == static_cast<int>(selectedMenuOption)) {
                    sf::Text arrow(font, ">", 24);
                    arrow.setFillColor(sf::Color::Red);
                    arrow.setPosition(sf::Vector2f(window.getSize().x * 0.35f, yPos));
                    window.draw(arrow);
                }
            }
            
            // Istruzioni - sezione separata e ben organizzata
            sf::Text instructionsTitle(font, "CONTROLLI:", 18);
            instructionsTitle.setFillColor(sf::Color::Cyan);
            instructionsTitle.setPosition(sf::Vector2f(window.getSize().x * 0.15f, window.getSize().y * 0.72f));
            window.draw(instructionsTitle);
            
            sf::Text navigationText(font, "Frecce SU/GIU - naviga menu", 14);
            navigationText.setFillColor(sf::Color::White);
            navigationText.setPosition(sf::Vector2f(window.getSize().x * 0.15f, window.getSize().y * 0.76f));
            window.draw(navigationText);
            
            sf::Text selectText(font, "INVIO - seleziona", 14);
            selectText.setFillColor(sf::Color::White);
            selectText.setPosition(sf::Vector2f(window.getSize().x * 0.15f, window.getSize().y * 0.79f));
            window.draw(selectText);
            
            sf::Text moveText(font, "Frecce - muovi Pac-Man durante il gioco", 14);
            moveText.setFillColor(sf::Color::White);
            moveText.setPosition(sf::Vector2f(window.getSize().x * 0.15f, window.getSize().y * 0.82f));
            window.draw(moveText);
            
            sf::Text pauseText(font, "P - pausa (solo quando Pac-Man e' fermo)", 14);
            pauseText.setFillColor(sf::Color::White);
            pauseText.setPosition(sf::Vector2f(window.getSize().x * 0.15f, window.getSize().y * 0.85f));
            window.draw(pauseText);
            
            window.display();
            
            // Gestione input menu
            auto event = window.waitEvent();
            if (event && event->is<sf::Event::KeyPressed>()) {
                if (auto keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                    if (keyEvent->code == sf::Keyboard::Key::Up) {
                        if (menuSoundCooldown.getElapsedTime().asMilliseconds() > 100) {
                            sfxChompMenu.play(); // Suono di navigazione menu (non in loop)
                            menuSoundCooldown.restart();
                        }
                        selectedMenuOption = static_cast<MenuOption>((static_cast<int>(selectedMenuOption) - 1 + NUM_MENU_OPTIONS) % NUM_MENU_OPTIONS);
                    } else if (keyEvent->code == sf::Keyboard::Key::Down) {
                        if (menuSoundCooldown.getElapsedTime().asMilliseconds() > 100) {
                            sfxChompMenu.play(); // Suono di navigazione menu (non in loop)
                            menuSoundCooldown.restart();
                        }
                        selectedMenuOption = static_cast<MenuOption>((static_cast<int>(selectedMenuOption) + 1) % NUM_MENU_OPTIONS);
                    } else if (keyEvent->code == sf::Keyboard::Key::Enter) {
                        sfxEatGhost.play(); // Suono di conferma selezione
                        switch (selectedMenuOption) {
                            case MenuOption::PLAY:
                                // Inizia una nuova partita
                                gameState = GameState::PLAYING;
                                
                                // Reset del suono chomp per nuova partita
                                chompActive = false;
                                sfxChomp.stop();
                                chompSoundStarted = false;
                                
                                // Avvia la musica di sottofondo solo se non è già stata avviata
                                if (!musicStarted) {
                                    music.play();
                                    musicStarted = true;
                                    canPlayGhostSounds = false; // Reset audio fantasmi quando ricomincia la musica
                                }
                                
                                gameOver = false;
                                gameStarted = false;
                                currentLevel = 0;
                                score->resetScore();
                                pac.setLives(3);
                                recordChecked = false; // Reset del flag per il prossimo game over
                                loadLevel(0);
                                break;
                            case MenuOption::HIGHSCORE:
                                // Vai alla schermata dei record
                                gameState = GameState::HIGHSCORE;
                                break;
                            case MenuOption::EXIT:
                                // Esci dal gioco
                                window.close();
                                break;
                        }
                        continue;
                    }
                }
            }
            if (event && event->is<sf::Event::Closed>()) {
                window.close();
                break;
            }
            continue;
        }

        if (gameState == GameState::HIGHSCORE) {
            // Mostra schermata dei record usando la classe HighScore
            window.clear(sf::Color::Black);
            
            // Disegna la schermata dei record
            highScore->draw(window, window.getSize());
            
            window.display();
            
            // Gestione input schermata record
            auto event = window.waitEvent();
            if (event && event->is<sf::Event::KeyPressed>()) {
                if (auto keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                    if (keyEvent->code == sf::Keyboard::Key::Escape) {
                        sfxEatGhost.play(); // Suono di ritorno al menu
                        // Ferma tutti i suoni quando si torna al menu da highscore
                        sfxGhostNormal.stop();
                        sfxGhostReturn.stop();
                        ghostSoundPlaying = false;
                        canPlayGhostSounds = false;
                        chompActive = false;
                        sfxChomp.stop(); // Ferma completamente il chomp quando si torna al menu
                        chompSoundStarted = false; // Reset del flag
                        gameState = GameState::MENU;
                        continue;
                    }
                }
            }
            if (event && event->is<sf::Event::Closed>()) {
                window.close();
                break;
            }
            continue;
        }

        if (gameState == GameState::PAUSED) {
            // Mostra menu di pausa
            window.clear(sf::Color::Black);
            sf::Font font(fontPath.string());
            
            sf::Text titleText(font, "PAUSA", 48);
            titleText.setFillColor(sf::Color::Yellow);
            titleText.setOutlineColor(sf::Color::Blue);
            titleText.setOutlineThickness(3);
            titleText.setPosition(sf::Vector2f(window.getSize().x * 0.35f, window.getSize().y * 0.15f));
            window.draw(titleText);
            
            // Opzioni del menu pausa
            std::vector<std::string> pauseItems = {"RIPRENDI", "TORNA AL MENU"};
            std::vector<sf::Color> pauseColors = {sf::Color::White, sf::Color::White};
            
            // Evidenzia l'opzione selezionata
            pauseColors[static_cast<int>(selectedPauseOption)] = sf::Color::Yellow;
            
            for (int i = 0; i < NUM_PAUSE_OPTIONS; ++i) {
                sf::Text pauseText(font, pauseItems[i], 24);
                pauseText.setFillColor(pauseColors[i]);
                if (i == static_cast<int>(selectedPauseOption)) {
                    pauseText.setOutlineColor(sf::Color::Red);
                    pauseText.setOutlineThickness(2);
                }
                
                float yPos = window.getSize().y * 0.4f + (i * 60.f);
                pauseText.setPosition(sf::Vector2f(window.getSize().x * 0.35f, yPos));
                window.draw(pauseText);
                
                // Freccia per l'opzione selezionata
                if (i == static_cast<int>(selectedPauseOption)) {
                    sf::Text arrow(font, ">", 24);
                    arrow.setFillColor(sf::Color::Red);
                    arrow.setPosition(sf::Vector2f(window.getSize().x * 0.3f, yPos));
                    window.draw(arrow);
                }
            }
            
            // Istruzioni
            sf::Text instructionsText(font, "Usa le frecce SU/GIU per navigare", 16);
            instructionsText.setFillColor(sf::Color::Cyan);
            instructionsText.setPosition(sf::Vector2f(window.getSize().x * 0.15f, window.getSize().y * 0.65f));
            window.draw(instructionsText);
            
            sf::Text selectText(font, "INVIO per selezionare - P per riprendere", 16);
            selectText.setFillColor(sf::Color::Cyan);
            selectText.setPosition(sf::Vector2f(window.getSize().x * 0.12f, window.getSize().y * 0.68f));
            window.draw(selectText);
            
            window.display();
            
            // Gestione input menu pausa
            auto event = window.waitEvent();
            if (event && event->is<sf::Event::KeyPressed>()) {
                if (auto keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                    if (keyEvent->code == sf::Keyboard::Key::Up) {
                        if (menuSoundCooldown.getElapsedTime().asMilliseconds() > 100) {
                            sfxChompMenu.play(); // Suono di navigazione menu pausa (non in loop)
                            menuSoundCooldown.restart();
                        }
                        selectedPauseOption = static_cast<PauseOption>((static_cast<int>(selectedPauseOption) - 1 + NUM_PAUSE_OPTIONS) % NUM_PAUSE_OPTIONS);
                    } else if (keyEvent->code == sf::Keyboard::Key::Down) {
                        if (menuSoundCooldown.getElapsedTime().asMilliseconds() > 100) {
                            sfxChompMenu.play(); // Suono di navigazione menu pausa (non in loop)
                            menuSoundCooldown.restart();
                        }
                        selectedPauseOption = static_cast<PauseOption>((static_cast<int>(selectedPauseOption) + 1) % NUM_PAUSE_OPTIONS);
                    } else if (keyEvent->code == sf::Keyboard::Key::Enter) {
                        sfxEatGhost.play(); // Suono di conferma selezione pausa
                        switch (selectedPauseOption) {
                            case PauseOption::RESUME:
                                // Riprendi il gioco
                                gameState = GameState::PLAYING;
                                break;
                            case PauseOption::BACK_TO_MENU:
                                // Ferma tutti i suoni quando si torna al menu dal pause
                                sfxGhostNormal.stop();
                                sfxGhostReturn.stop();
                                ghostSoundPlaying = false;
                                canPlayGhostSounds = false;
                                chompActive = false;
                                sfxChomp.stop(); // Ferma completamente il chomp quando si torna al menu
                                chompSoundStarted = false; // Reset del flag
                                // Torna al menu principale
                                gameState = GameState::MENU;
                                break;
                        }
                        continue;
                    } else if (keyEvent->code == sf::Keyboard::Key::P) {
                        // Shortcut per riprendere con P
                        gameState = GameState::PLAYING;
                        continue;
                    }
                }
            }
            if (event && event->is<sf::Event::Closed>()) {
                window.close();
                break;
            }
            continue;
        }

        // --- Modalità ghosts ---
        modeJustChanged = false;
        // Riattiva il cambio automatico scatter/chase SOLO durante il gameplay
        if (gameState == GameState::PLAYING && modePhase < (int)scatterChaseTimes.size() && scatterChaseTimes[modePhase] > 0.f) {
            modeTimer += dt;
            if (modeTimer >= scatterChaseTimes[modePhase]) {
                modeTimer = 0.f;
                modePhase++;
                ghostMode = (ghostMode == GhostMode::Scatter) ? GhostMode::Chase : GhostMode::Scatter;
                modeJustChanged = true;
                // std::cout << "[DEBUG] Cambio modalità fantasmi: " << (ghostMode == GhostMode::Scatter ? "SCATTER" : "CHASE") << std::endl;
            }
        }
        // Gestione eventi finestra
        while (auto ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) {
                window.close();
            } else if (ev->is<sf::Event::KeyPressed>()) {
                if (auto keyEvent = ev->getIf<sf::Event::KeyPressed>()) {
                    // Pausa durante il gameplay - SOLO se Pac-Man è davvero fermo
                    if (gameState == GameState::PLAYING && keyEvent->code == sf::Keyboard::Key::P) {
                        // Controlla se Pac-Man è effettivamente fermo (direzione = 0,0)
                        sf::Vector2f pacDirection = pac.getDirection();
                        bool isPacmanStopped = (pacDirection.x == 0.0f && pacDirection.y == 0.0f);
                        
                        // Consenti pausa solo se Pac-Man è fermo
                        if (isPacmanStopped) {
                            gameState = GameState::PAUSED;
                            sfxGhostNormal.stop(); // Ferma il suono fantasmi quando il gioco va in pausa
                            sfxGhostReturn.stop(); // Ferma anche il suono di ritorno
                            ghostSoundPlaying = false; // Reset flag suono fantasmi
                            chompActive = false; // Ferma anche il chomp ritmico
                            sfxChomp.setVolume(0.f); // Silenzia il chomp
                            sfxMenu.play(); // Suono del menu di pausa
                            selectedPauseOption = PauseOption::RESUME; // Reset selezione pausa
                        }
                    }
                }
            }
        }

        // Solo se il gioco è in stato PLAYING, aggiorna la logica di gioco
        if (gameState == GameState::PLAYING && !gameOver) {
            // Blocca movimento di Pac-Man e fantasmi finché la musica iniziale non è finita
            bool introMusicPlaying = (musicStarted && music.getStatus() == sf::Music::Status::Playing);
            
            // Blocca tutto il gioco finché la musica iniziale è in riproduzione
            if (introMusicPlaying) {
                // Ferma anche il suono chomp se per qualche motivo è partito
                chompActive = false;
                sfxChomp.stop();
                sfxChomp.setVolume(0.f);
                // Blocca anche i fantasmi
                // Non aggiornare Pac-Man o fantasmi
                // Salta direttamente al rendering
                goto render_section;
            }
            // Aggiorna il giocatore
            pac.update(dt, map, tileSize);
            
            // --- GESTIONE SUONO FANTASMI IN MOVIMENTO ---
            // Consenti suoni fantasmi SOLO dopo che Pac-Man ha iniziato a muoversi
            if (musicStarted && !canPlayGhostSounds) {
                if (music.getStatus() != sf::Music::Status::Playing && gameStarted) {
                    canPlayGhostSounds = true;
                    ghostSoundPlaying = false; // Forza il reset per evitare suoni prematuri
                    sfxGhostNormal.stop();
                    sfxGhostReturn.stop();
                }
            }
            // Solo se i fantasmi possono suonare, gestisci gli audio
            if (canPlayGhostSounds) {
                // Determina quale suono dei fantasmi dovrebbe essere attivo
                bool anyFrightened = false;
                bool anyReturning = false;
                for (const auto& ghost : ghosts) {
                    if (ghost->isFrightened() && !ghost->isEaten()) {
                        anyFrightened = true;
                    }
                    if (ghost->isEaten() && ghost->isReturningToHouse()) {
                        anyReturning = true;
                    }
                }
                
                // Gestisci i suoni in base allo stato dei fantasmi (evita sovrapposizioni)
                static bool lastAnyReturning = false;
                static bool lastAnyFrightened = false;
                static sf::Clock ghostSoundCooldown; // Previene cambio troppo frequente
                
                // Cambia audio solo se lo stato è cambiato E è passato abbastanza tempo
                if ((anyReturning != lastAnyReturning || anyFrightened != lastAnyFrightened) && 
                    ghostSoundCooldown.getElapsedTime().asMilliseconds() > 300) {
                    
                    // Stato cambiato, ferma tutti i suoni dei fantasmi prima
                    sfxGhostNormal.stop();
                    sfxGhostReturn.stop();
                    ghostSoundPlaying = false;
                    
                    if (anyReturning) {
                        // Priorità: suono ritorno alla casa
                        sfxGhostReturn.play();
                        ghostSoundPlaying = true;
                        // std::cout << "[AUDIO] Attivato suono ritorno fantasmi\n";
                    } else if (anyFrightened) {
                        // Suono quando i fantasmi sono frightened (blu) - silenzio
                        ghostSoundPlaying = false;
                        // std::cout << "[AUDIO] Fantasmi frightened, silenzio\n";
                    } else {
                        // Suono normale dei fantasmi
                        sfxGhostNormal.play();
                        ghostSoundPlaying = true;
                        // std::cout << "[AUDIO] Attivato suono normale fantasmi\n";
                    }
                    
                    lastAnyReturning = anyReturning;
                    lastAnyFrightened = anyFrightened;
                    ghostSoundCooldown.restart();
                }
                
                // Mantieni i suoni attivi se non sono cambiati gli stati (controllo meno frequente)
                static sf::Clock maintainSoundCheck;
                if (maintainSoundCheck.getElapsedTime().asSeconds() > 1.0f) // Controlla ogni secondo
                {
                    if (anyReturning && sfxGhostReturn.getStatus() != sf::Sound::Status::Playing) {
                        sfxGhostReturn.play();
                    } else if (!anyFrightened && !anyReturning && sfxGhostNormal.getStatus() != sf::Sound::Status::Playing) {
                        sfxGhostNormal.play();
                    }
                    maintainSoundCheck.restart();
                }
            }

            // --- GESTIONE RELEASE SEMPLICE E SEQUENZIALE DEI FANTASMI ---
            if (gameStarted && nextGhostToRelease < 4) {
                ghostReleaseTimer += dt;
                if (ghostReleaseTimer >= ghostReleaseDelays[nextGhostToRelease]) {
                    ghosts[nextGhostToRelease]->setReleased(true);
                    // std::cout << "[DEBUG] Ghost " << nextGhostToRelease << " (" << Ghost::getTypeName(static_cast<Ghost::Type>(nextGhostToRelease)) << ") released!\n";
                    nextGhostToRelease++;
                    ghostReleaseTimer = 0.f; // resetta il timer SOLO dopo il rilascio
                }
            }

            // Aggiorna i fantasmi con la nuova architettura SOLO se la musica iniziale  e8 finita
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
                
                // WORKAROUND: Evita che i fantasmi attraversino i bordi laterali (teleport)
                sf::Vector2f ghostPos = ghosts[i]->getPosition();
                unsigned ghostTileX = static_cast<unsigned>(ghostPos.x / tileSize.x);
                // Se il fantasma è troppo vicino ai bordi laterali, riposizionalo e forza la direzione verso l'interno
                if (ghostTileX <= 1) {
                    sf::Vector2f newPos = ghostPos;
                    newPos.x = 2.2f * tileSize.x;
                    ghosts[i]->setPosition(newPos);
                    // Forza la direzione verso destra (interno)
                    sf::Vector2f dir = ghosts[i]->getDirection();
                    dir.x = 1.f; dir.y = 0.f;
                    ghosts[i]->setDirection(dir);
                    // std::cout << "[DEBUG] Ghost " << i << " troppo a sinistra, riposizionato e direzione forzata a destra\n";
                } else if (ghostTileX >= mapSz.x - 2) {
                    sf::Vector2f newPos = ghostPos;
                    newPos.x = (mapSz.x - 2.2f) * tileSize.x;
                    ghosts[i]->setPosition(newPos);
                    // Forza la direzione verso sinistra (interno)
                    sf::Vector2f dir = ghosts[i]->getDirection();
                    dir.x = -1.f; dir.y = 0.f;
                    ghosts[i]->setDirection(dir);
                    // std::cout << "[DEBUG] Ghost " << i << " troppo a destra, riposizionato e direzione forzata a sinistra\n";
                }
            }

            // Controlla collisione con i pellet
            bool pelletEaten = false;
            for (auto it = pellets.begin(); it != pellets.end(); )
                if (it->eaten(pac.getPosition())) {
                    score->add(10);
                    pelletEaten = true;
                    it = pellets.erase(it);
                } else ++it;
            
            // Gestione suono chomp continuo - CONTROLLO VOLUME INVECE DI STOP/PLAY
            if (pelletEaten) {
                // Avvia il loop continuo del chomp solo la prima volta
                if (!chompSoundStarted) {
                    chompSoundStarted = true;
                    sfxChomp.stop(); // Forza il reset
                    sfxChomp.setVolume(60.f);
                    sfxChomp.play(); // Inizia il loop continuo una volta sola
                }
                // Rendi il chomp udibile
                if (!chompActive) {
                    chompActive = true;
                    sfxChomp.setVolume(60.f); // Volume normale quando si mangiano pellet
                }
                lastPelletTimer.restart(); // Reset timer quando si mangia un pellet
            }
            // Silenzia il chomp se non si mangiano pellet da un po' (ma non fermarlo!)
            if (chompActive && lastPelletTimer.getElapsedTime().asMilliseconds() > 300) {
                chompActive = false;
                sfxChomp.setVolume(0.f); // Silenzia invece di fermare
            }

            // Controlla se è stata raggiunta una vita extra
            if (score->checkExtraLife()) {
                // Ferma tutti i suoni durante il messaggio di vita extra
                sfxGhostNormal.stop();
                sfxGhostReturn.stop();
                ghostSoundPlaying = false;
                chompActive = false;
                sfxChomp.setVolume(0.f); // Silenzia il chomp
                
                // Ferma Pac-Man per evitare che esca dalla mappa durante il messaggio
                pac.stopMovement();
                pac.setLives(pac.getLives() + 1);
                showMessage(window, "VITA EXTRA!\n\nHai raggiunto 10.000 punti!\n\nVite: " + std::to_string(pac.getLives()), fontPath.string());
            }

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
                sfxGhostBlue.play();
                // Attiva frightened SOLO per fantasmi già usciti
                for (auto& g : ghosts) {
                    if (g->isReleased()) g->setFrightened(frightenedBaseDuration);
                }
            }

            // Se tutti i pellet sono stati raccolti, mostra messaggio e passa al livello successivo
            if (pellets.empty()) {
                // Ferma tutti i suoni durante le schermate di vittoria
                sfxGhostNormal.stop();
                sfxGhostReturn.stop();
                ghostSoundPlaying = false;
                chompActive = false;
                sfxChomp.setVolume(0.f); // Silenzia il chomp
                
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

            // --- BLOCCO PAUSA DOPO MANGIATO FANTASMA (combo classica) ---
            if (isGhostEatPause) {
                // Blocca movimento Pac-Man e fantasmi SOLO durante la pausa
                pac.setDirection({0.f, 0.f});
                for (size_t i = 0; i < ghosts.size(); ++i) ghosts[i]->setDirection({0.f, 0.f});
                // Mostra punteggio sopra Pac-Man
                sf::Vector2f mapOffset;
                mapOffset.x = (window.getSize().x - map.getSize().x * tileSize.x) / 2.f;
                mapOffset.y = (window.getSize().y - map.getSize().y * tileSize.y) / 2.f;
                // Mostra punteggio sopra Pac-Man
                sf::Font font(fontPath.string());
                sf::Text ghostScoreText(font, std::to_string(ghostEatScore), 18);
                ghostScoreText.setFillColor(sf::Color(0, 191, 255)); // Blu frightened
                ghostScoreText.setOutlineColor(sf::Color::Black);
                ghostScoreText.setOutlineThickness(4);
                ghostScoreText.setStyle(sf::Text::Bold);
                auto textRect = ghostScoreText.getLocalBounds();
                ghostScoreText.setOrigin({textRect.position.x + textRect.size.x / 2.f, textRect.position.y + textRect.size.y / 2.f});
                ghostScoreText.setPosition(sf::Vector2f(pac.getPosition().x + mapOffset.x, pac.getPosition().y + mapOffset.y - 40));
                window.clear();
                // Disegna la mappa e gli oggetti normalmente
                sf::Transform mapTransform;
                mapTransform.translate(mapOffset);
                window.draw(map, mapTransform);
                for (auto& p : pellets) {
                    sf::Transform pelletTransform;
                    pelletTransform.translate(mapOffset);
                    window.draw(p, pelletTransform);
                }
                for (const auto& pos : superPelletPositions) {
                    sf::CircleShape superPellet(9.f);
                    superPellet.setOrigin(sf::Vector2f(9.f, 9.f));
                    superPellet.setPosition(pos + mapOffset);
                    superPellet.setFillColor(sf::Color(255, 209, 128));
                    window.draw(superPellet);
                }
                for (auto& g : ghosts) {
                    sf::Transform ghostTransform;
                    ghostTransform.translate(mapOffset);
                    window.draw(*g, ghostTransform);
                }
                sf::Transform pacTransform;
                pacTransform.translate(mapOffset);
                window.draw(pac, pacTransform);
                score->draw(window);
                // HUD
                sf::Text livesText(font, "Vite: " + std::to_string(pac.getLives()), 20);
                livesText.setFillColor(sf::Color::White);
                livesText.setPosition(sf::Vector2f(window.getSize().x - 140.f, 10.f));
                window.draw(livesText);
                sf::Text levelText(font, "Livello: " + std::to_string(currentLevel + 1), 20);
                levelText.setFillColor(sf::Color::Cyan);
                levelText.setPosition(sf::Vector2f(10.f, window.getSize().y - 30.f));
                window.draw(levelText);
                window.draw(ghostScoreText);
                window.display();
                if (ghostEatPauseClock.getElapsedTime().asSeconds() >= GHOST_EAT_PAUSE) {
                    isGhostEatPause = false;
                    // Ripristina la direzione di Pac-Man e dei fantasmi
                    pac.setDirection(pacmanDirBeforePause);
                    for (size_t i = 0; i < ghosts.size(); ++i) ghosts[i]->setDirection(ghostsDirBeforePause[i]);
                }
                continue;
            }
            // Collisione Pac-Man / Fantasmi
            for (size_t i = 0; i < ghosts.size(); ++i) {
                const auto& ghost = ghosts[i];
                float dist = (pac.getPosition() - ghost->getPosition()).x * (pac.getPosition() - ghost->getPosition()).x +
                             (pac.getPosition() - ghost->getPosition()).y * (pac.getPosition() - ghost->getPosition()).y;
                float minDist = 24.f * 24.f; // raggio Pac-Man + raggio Ghost (approssimato)
                if (dist < minDist) {
                    if (ghost->isFrightened() && !ghost->isEaten()) {
                        ghost->setEaten(true);
                        sfxEatGhost.play();
                        // Combo: 200, 400, 800, 1600
                        static const int ghostScores[] = {200, 400, 800, 1600};
                        ghostEatScore = ghostScores[std::min(ghostEatCombo, 3)];
                        score->add(ghostEatScore);
                        ghostEatCombo++;
                        // Controlla se è stata raggiunta una vita extra dopo aver mangiato un fantasma
                        if (score->checkExtraLife()) {
                            // Ferma tutti i suoni durante il messaggio di vita extra
                            sfxGhostNormal.stop();
                            sfxGhostReturn.stop();
                            ghostSoundPlaying = false;
                            chompActive = false;
                            sfxChomp.setVolume(0.f); // Silenzia il chomp
                            pac.setLives(pac.getLives() + 1);
                            showMessage(window, "VITA EXTRA!\n\nHai raggiunto 10.000 punti!\n\nVite: " + std::to_string(pac.getLives()), fontPath.string());
                        }
                        // Salva la direzione di Pac-Man e dei fantasmi prima della pausa
                        pacmanDirBeforePause = pac.getDirection();
                        for (size_t j = 0; j < ghosts.size(); ++j) ghostsDirBeforePause[j] = ghosts[j]->getDirection();
                        isGhostEatPause = true;
                        ghostEatPauseClock.restart();
                        continue;
                    } else if (!ghost->isEaten() && !ghost->isReturningToHouse()) {
                        // Avvia animazione morte Pac-Man
                        if (!pac.isDying()) {
                            pac.startDeathAnimation();
                            sfxGhostNormal.stop(); // Ferma il suono fantasmi quando Pac-Man muore
                            sfxGhostReturn.stop(); // Ferma anche il suono di ritorno
                            ghostSoundPlaying = false; // Reset flag suono fantasmi
                            // SILENZIA IMMEDIATAMENTE IL CHOMP
                            chompActive = false;
                            sfxChomp.stop();
                            sfxChomp.setVolume(0.f);
                            sfxDeath.play();
                        }
                    }
                }
            }
            // --- BLOCCO GIOCO DURANTE ANIMAZIONE MORTE PAC-MAN ---
            if (pac.isDying()) {
                // Aggiorna solo Pac-Man per animazione morte
                pac.update(dt, map, tileSize);
                // Rendering
                goto render_section;
            }
            // --- FINE BLOCCO ANIMAZIONE MORTE ---
            // Dopo che l'animazione di morte è finita, decrementa vite o Game Over
            if (pac.isDeathAnimationFinished()) {
                pac.resetDeathAnimation();
                pac.loseLife();
                if (pac.getLives() <= 0) {
                    // Ferma tutti i suoni quando si va in Game Over
                    chompActive = false;
                    sfxChomp.stop();
                    sfxChomp.setVolume(0.f);
                    // Game Over - passa alla schermata Game Over
                    gameState = GameState::GAME_OVER;
                } else {
                    showMessage(window, "VITA PERSA!\n\nVite rimaste: " + std::to_string(pac.getLives()) + "\n\nRiprova!", fontPath.string());
                    // Ricarica il livello corrente SENZA resettare i pellet e le vite
                    int currentLives = pac.getLives();
                    loadLevel(currentLevel, false); // NON resettare i pellet
                    pac.setLives(currentLives); // Ripristina le vite corrette
                    ghostSoundPlaying = false; // Reset flag suono fantasmi dopo morte
                    // --- FIX: reset chomp dopo morte ---
                    chompActive = false;
                    chompSoundStarted = false;
                    sfxChomp.stop();
                    sfxChomp.setVolume(60.f); // Reset volume per il prossimo uso
                    gameOver = true;
                }
            }
            // --- RESET COMBO SOLO SE NESSUN FANTASMA È FRIGHTENED ---
            bool anyFrightened = false;
            for (const auto& ghost : ghosts) {
                if (ghost->isFrightened() && !ghost->isEaten()) {
                    anyFrightened = true;
                    break;
                }
            }
            if (!anyFrightened) ghostEatCombo = 0;
        }
        // Gestione gameOver e gameStarted SOLO durante il gameplay
        if (gameState == GameState::PLAYING) {
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
        }

        render_section:
        // Rendering
        window.clear();
        
        // Disegna l'HUD solo durante il gameplay
        if (gameState == GameState::PLAYING) {
            // Centra la mappa nella finestra
            sf::Vector2f mapOffset;
            mapOffset.x = (window.getSize().x - map.getSize().x * tileSize.x) / 2.f;
            mapOffset.y = (window.getSize().y - map.getSize().y * tileSize.y) / 2.f;
            
            // Applica l'offset alla mappa (se possibile)
            sf::Transform mapTransform;
            mapTransform.translate(mapOffset);
            window.draw(map, mapTransform);
            
            // Prima i pellet, poi i Super Pellet grandi, poi i fantasmi, poi Pac-Man sopra tutto
            for (auto& p : pellets) {
                sf::Transform pelletTransform;
                pelletTransform.translate(mapOffset);
                window.draw(p, pelletTransform);
            }
            // Super Pellet lampeggianti: visibile (peach) o invisibile (trasparente)
            static sf::Clock blinkClock;
            float blink = 1.0f;
            sf::Color pelletColor = sf::Color(255, 209, 128); // sempre visibile di default
            if (gameStarted) {
                blink = std::abs(std::sin(blinkClock.getElapsedTime().asSeconds() * 12)); // lampeggio ~6 volte/sec
                pelletColor = (blink > 0.5f) ? sf::Color(255, 209, 128) : sf::Color(255, 209, 128, 0); // peach o trasparente
            }
            for (const auto& pos : superPelletPositions) {
                sf::CircleShape superPellet(9.f); // raggio 9px
                superPellet.setOrigin(sf::Vector2f(9.f, 9.f));
                superPellet.setPosition(pos + mapOffset);
                superPellet.setFillColor(pelletColor);
                window.draw(superPellet);
            }
            for (auto& g : ghosts) {
                sf::Transform ghostTransform;
                ghostTransform.translate(mapOffset);
                window.draw(*g, ghostTransform);
            }
            sf::Transform pacTransform;
            pacTransform.translate(mapOffset);
            window.draw(pac, pacTransform);
            
            score->draw(window);
            
            // HUD - Visualizza vite del giocatore (angolo in alto a destra)
            sf::Font font(fontPath.string());
            sf::Text livesText(font, "Vite: " + std::to_string(pac.getLives()), 20);
            livesText.setFillColor(sf::Color::White);
            livesText.setPosition(sf::Vector2f(window.getSize().x - 140.f, 10.f)); // Più a sinistra per evitare tagli
            window.draw(livesText);
            
            // HUD - Visualizza livello corrente (angolo in basso a sinistra)
            sf::Text levelText(font, "Livello: " + std::to_string(currentLevel + 1), 20);
            levelText.setFillColor(sf::Color::Cyan);
            levelText.setPosition(sf::Vector2f(10.f, window.getSize().y - 30.f)); // Angolo in basso a sinistra
            window.draw(levelText);
        }
        
        window.display();
    }

    return 0;
}

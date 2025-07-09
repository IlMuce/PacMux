# Pacman-SFML Release 4

Questo repository contiene una versione base funzionante (MVP, Minimum Viable Product) di un clone di Pac-Man realizzato con SFML 3.0 e CMake.

## Struttura del progetto

```
pacman-sfml/
├── assets/            # Risorse di gioco (mappe, font)
│   ├── map1.txt       # Mappa di esempio (ASCII)
│   └── arial.ttf      # Font per il punteggio
├── include/           # Header C++ (tutti gli .hpp)
│   ├── Blinky.hpp     # AI Blinky
│   ├── Pinky.hpp      # AI Pinky
│   ├── Inky.hpp       # AI Inky
│   ├── Clyde.hpp      # AI Clyde
│   ├── Ghost.hpp      # Base class Ghost
│   ├── Player.hpp     # Pac-Man
│   ├── TileMap.hpp    # Mappa
│   ├── Pellet.hpp     # Pellet
│   └── Score.hpp      # Punteggio
├── src/               # Codice sorgente C++
│   ├── main.cpp       # Punto d'ingresso e loop di gioco
│   ├── Blinky.cpp     # AI Blinky
│   ├── Pinky.cpp      # AI Pinky
│   ├── Inky.cpp       # AI Inky
│   ├── Clyde.cpp      # AI Clyde
│   ├── Ghost.cpp      # Base class Ghost
│   ├── Player.cpp     # Pac-Man
│   ├── TileMap.cpp    # Mappa
│   ├── Pellet.cpp     # Pellet
│   └── Score.cpp      # Punteggio
├── CMakeLists.txt     # Configurazione di build
└── README.md          # Questo file
```

## Requisiti

- Windows 10/11 (o Linux/macOS con SFML 3.0)
- Visual Studio Build Tools 2022 / GCC o Clang
- SFML 3.0.0 installato e disponibile via CMake

## Come compilare

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

L’eseguibile verrà generato in `build/Release/PacmanMVP.exe` e dovrà essere eseguito dalla cartella `build/Release`, affinché trovi la cartella `assets` al suo fianco.

## Come si gioca

- **Freccia sinistra/destra/su/giù**: muovi Pac-Man lungo i corridoi a griglia.
- Raccogli tutti i pellet per terminare il livello.
- **Evita i fantasmi**: Se Blinky (rosso) ti tocca, la partita si resetta.
- Il punteggio aumenta di 10 punti per pellet.
- Usa i tunnel laterali per sfuggire ai fantasmi con il wrap-around.

## Funzionalità della Release 1

1. Lettura mappa da file ASCII (`assets/map1.txt`).
2. Movimento tile-based fluido di Pac-Man tramite le frecce direzionali.
3. Raccolta automatica dei pellet e incremento del punteggio a schermo.
4. Finestra di gioco con framerate fisso a 60 FPS.
5. Gestione asset (font e mappa) automatica: basta la cartella `assets` accanto all'eseguibile.

## Funzionalità della Release 2

- Implementato il teletrasporto ai bordi (wrap-around): Pac-Man che esce da un lato o dall'alto/basso riappare dal lato opposto, come nei classici arcade.
- Mappa più grande, simmetrica e ispirata ai vecchi Pac-Man, con tunnel orizzontali e verticali funzionanti.
- Possibilità di testare facilmente il wrap-around grazie a corridoi e tunnel dedicati.
- Codice pronto per l'estensione con nuove feature (fantasmi, power-up, ecc.).

## Funzionalità della Release 3

- Aggiunta dei fantasmi statici, posizionati nella ghost house centrale, ognuno con un colore diverso.
- Gestione collisione tra Pac-Man e i fantasmi: se avviene, viene mostrato un messaggio di Game Over e la partita si resetta (Pac-Man torna allo spawn, punteggio e pellet vengono azzerati).
- Gestione della vittoria: se tutti i pellet vengono raccolti, viene mostrato un messaggio di vittoria e la partita si resetta.
- I pellet non vengono generati nella ghost house né nella cella di spawn di Pac-Man.
- Tutte le transizioni avvengono senza chiudere la finestra di gioco.

## Funzionalità della Release 4

- **Mappa fedele all'originale**: Aggiornata `map1.txt` con dimensioni e layout più vicini al Pac-Man classico.
- **Tile speciali**: Introdotti tile '2' per spazi vuoti senza pellet (es: tunnel, aree speciali).
- **Ghost House migliorata**: I fantasmi spawano nella casa centrale e non possono rientrarvi una volta usciti.
- **Generazione pellet ottimizzata**: I pellet vengono creati solo sui tile '0', escludendo tile '2' e spawn di Pac-Man.
- **Refactoring architetturale**: Tutti gli header sono in `include/`, i sorgenti in `src/`. Ogni fantasma ha il suo file (`Blinky`, `Pinky`, `Inky`, `Clyde`).
- **Ghost base class**: Tutta la logica comune (movimento, pathfinding greedy, tunnel, ghost house, scatter/chase) è in `Ghost.cpp`/`Ghost.hpp`.
- **AI fedele all'originale**: Blinky insegue Pac-Man, Pinky mira 4 celle avanti, entrambi con pathfinding greedy (distanza euclidea, tie-breaking Up>Left>Down>Right, no inversione immediata).
- **Pronto per estensioni**: Struttura pronta per logiche uniche di Inky e Clyde, modalità frightened, animazioni, ecc.

### Funzionalità della Release 5
- I fantasmi entrano in modalità frightened (blu, movimento casuale e rallentato) quando Pac-Man mangia un Super Pellet.
- I fantasmi possono essere mangiati da Pac-Man in modalità frightened: diventano "occhi", tornano alla ghost house, attendono e poi respawnano normalmente.
- Implementato il timer frightened, con effetto lampeggiante prima della fine.
- Tutti i fantasmi (Blinky, Pinky, Inky, Clyde) condividono la stessa logica di frightened/eaten/respawn, senza duplicazione di codice.
- Alternanza classica scatter/chase con timer configurabili e debug log dettagliato.
- Reset completo degli stati dei fantasmi su game over o vittoria.

### Release 6: Levels & Progression
- Sistema multi-livello con caricamento dinamico
- Implementato reset con aumento di difficoltá una volta finiti tutti i livelli a disposizione
- Velocità variabili per Pac-Man e fantasmi
- Nuova mappa basata su Ms. Pacman
- Corrette tempistiche di uscita fantasmi dalla ghost house e diminuito scatter time
- Aggiunte scritte di vittoria e sconfitta con grafica SFML al posto di console windows
- Cambiato font principale 

## Prossime Release (Roadmap)

## Release 7: UI completa
- Menu principale, HUD, punteggio, vite.
- Schermata iniziale, Game Over, visualizzazione vite e punteggio.

## Release 8: Audio
- Musica di sottofondo, effetti (mangiare, morte).
- Integrazione con SFML audio.

## Release 9: Salvataggio highscore + schermata dei record
- File locale (JSON o TXT), visualizzazione dei record.

## Release 10: Polish finale
- Animazioni, sprite, effetti particellari, transizioni, ottimizzazioni, bugfix, refactoring.

## Bug Noti
- Fantasmi si bloccano quando tentano di attraversare il teleport finché pacman non li fa entrare in modalitá frightened.

---

Buon divertimento!
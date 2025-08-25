# PacMux

Questo repository contiene una versione base funzionante (MVP, Minimum Viable Product) di un clone di Pac-Man realizzato con SFML 3.0 e CMake.

## Struttura del progetto

```
pacman-sfml/
├── assets/            # Risorse di gioco (mappe, font, audio)
│   ├── map1.txt       # Livello 1
│   ├── map2.txt       # Livello 2
│   ├── map3.txt       # Livello 3
│   ├── pacman.ttf     # Font del gioco
│   └── audio/         # Tutti i file audio sono ora qui!
│       ├── pacman_beginning.wav      # Musica di sottofondo
│       ├── PacmanChomp.mp3          # Suono "wakawakawaka"
│       ├── pacman_chomp.wav         # Suono menu
│       ├── pacman_eatghost.wav      # Suono mangiare fantasmi
│       ├── pacman_death.wav         # Suono morte Pac-Man
│       ├── pacman_menupausa.wav     # Suono menu/pausa
│       ├── GhostTurntoBlue.mp3      # Suono fantasmi blu (Super Pellet)
│       ├── GhostReturntoHome.mp3    # Suono ritorno fantasmi alla casa
│       └── GhostNormalMove.mp3      # Suono movimento normale fantasmi
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

## Attenzione: file richiesti e posizione

Tutti i file audio sono ora nella cartella `assets/audio/` e **non** più direttamente in `assets/`.

**File audio richiesti in `assets/audio/`:**
- pacman_beginning.wav
- PacmanChomp.mp3
- pacman_chomp.wav
- pacman_eatghost.wav
- pacman_death.wav
- pacman_menupausa.wav
- GhostTurntoBlue.mp3
- GhostReturntoHome.mp3
- GhostNormalMove.mp3

**Se uno di questi file non è presente o è in una posizione diversa, il gioco non funzionerà correttamente!**

**Altri file richiesti:**
- `assets/pacman.ttf` (font)
- `assets/map1.txt`, `assets/map2.txt`, `assets/map3.txt` (mappe)

Se sposti o rinomini questi file, aggiorna anche i percorsi nel codice o ripristina la struttura sopra indicata.

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

L’eseguibile verrà generato in `build/Release/Pacman.exe` e dovrà essere eseguito dalla cartella `build/Release`, affinché trovi la cartella `assets` al suo fianco.

## Come si gioca

- **Freccia sinistra/destra/su/giù** o **W/A/S/D**: muovi Pac-Man lungo i corridoi a griglia.
- Raccogli tutti i pellet per avanzare di livello.
- **Evita i fantasmi**: Se un fantasma ti tocca, perdi una vita. Se perdi tutte le vite é game over.
- Il punteggio aumenta di 10 punti per pellet, 200 punti per fantasma mangiato (con moltiplicatore).
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

### Funzionalità della Release 6
- Sistema multi-livello con caricamento dinamico
- Implementato reset con aumento di difficoltá una volta finiti tutti i livelli a disposizione
- Velocità variabili per Pac-Man e fantasmi
- Nuova mappa basata su Ms. Pacman
- Corrette tempistiche di uscita fantasmi dalla ghost house e diminuito scatter time
- Aggiunte scritte di vittoria e sconfitta con grafica SFML al posto di console windows
- Cambiato font principale 

### Funzionalità della Release 7
- Gestione avanzata stati di gioco.
- Aggiunta menu principale, menu di pausa, schermata Game Over.
- HUD completo con visualizzazione punteggio, vite rimaste e livello attuale.
- Vita extra una volta raggiunti 10k punti.
- Aggiunta possibilitá di compilazione con dll statici (non serve averli inclusi nel PATH di sistema).

### Funzionalità della Release 8
- Integrazione con SFML audio.
- RIEPILOGO DEGLI 8 SUONI IMPLEMENTATI:
    1. pacman_beginning.wav - Musica di sottofondo (una volta per partita)
    2. PacmanChomp.mp3 - Suono "wakawakawaka" + navigazione menu
    3. pacman_eatghost.wav - Mangiare fantasmi + conferma selezioni menu
    4. pacman_death.wav - Morte di Pac-Man
    5. pacman_menupausa.wav - Suono pausa + ritorno al menu da Game Over
    6. GhostTurntoBlue.mp3 - Quando i fantasmi diventano blu (Super Pellet)
    7. GhostReturntoHome.mp3 - Fantasmi che tornano alla casa (dopo essere stati mangiati)
    8. GhostNormalMove.mp3 - Movimento normale dei fantasmi (loop continuo)
- Aggiunta possibilità di modificare livelli audio di ciascun suono.

### Funzionalità della Release 9
- **Sistema Highscore completo**: Top 10 persistente con input nome giocatore e schermata Hall of Fame
- **Mappa 3 con tema arancione**: Stile Ms. Pac-Man con colori arancione chiaro
- **Bug fix critici**: Risolti chomp infinito, highscore dopo 7° record, parser JSON robusto

## Funzionalità della Release 10
- Aggiunte texture e animazioni per Pacman (movimento in ogni direzione e morte).
- Aggiunte texture e animazioni per tutti e 4 i Fantasmini (movimento, modalitá frightened e "occhi" una volta mangiati).
- Modificati dimensione e colore di Pellet e Super Pellet per renderli piú simili all'originale.
- Aggiunta punteggio a vista e moltiplicatore quando Pacman mangia i fantasmini.
- Corretti svariati bug legati a fanatsmi, stati di gioco e audio.

## Funzionalità della Release 11
- Classifica globale online (Top 50): a fine partita puoi caricare il punteggio e vederlo nella leaderboard dal menu.
- Scorrimento e refresh: frecce/PgUp/PgDn/Home/End per scorrere, R per aggiornare.
- Aggiornamenti rapidi e senza blocchi: upload/download asincroni.
- Dati su GitHub in formato JSON, parser robusto e fallback offline.
- Implementazione: HTTP con CPR; lettura via GitHub Raw; scrittura via GitHub Contents API (Base64 + SHA).

## Prossime Release (Roadmap)

## Release da 12 in poi:
- Aggiunta frutta
- Aggiunta livelli
- Grafica migliorata
- Miglioramenti texture del labirinto
- Correzione dei bug noti

## Bug Noti
- Fantasmi si bloccano quando tentano di attraversare il teleport finché pacman non li fa entrare in modalitá frightened.
  Soluzione temporanea: divieto di attraversamento dei teleport ai fantasmi.
- Se Pacman muore ed é toccato per piú frame da un fantasma possono esserci piú animazioni di morte.

---

Buon divertimento!
# Pacman-SFML Release 4

Questo repository contiene l'MVP di un clone di Pac-Man realizzato con SFML 3.0 e CMake.

## Struttura del progetto

```
pacman-sfml/
├── assets/            # Risorse di gioco (mappe, font)
│   ├── map1.txt       # Mappa di esempio (ASCII)
│   └── arial.ttf      # Font per il punteggio
├── src/               # Codice sorgente C++
│   ├── main.cpp       # Punto d'ingresso e loop di gioco
│   ├── Player.hpp/.cpp
│   ├── TileMap.hpp/.cpp
│   ├── Pellet.hpp/.cpp
│   └── Score.hpp/.cpp
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

## Funzionalità della Release 4 (COMPLETATA)

- **Mappa fedele all'originale**: Aggiornata `map1.txt` con dimensioni e layout più vicini al Pac-Man classico.
- **Tile speciali**: Introdotti tile '2' per spazi vuoti senza pellet (es: tunnel, aree speciali).
- **Ghost House migliorata**: I fantasmi spawano nella casa centrale e non possono rientrarvi una volta usciti.
- **Generazione pellet ottimizzata**: I pellet vengono creati solo sui tile '0', escludendo tile '2' e spawn di Pac-Man.
- **AI di Blinky**: Implementato inseguimento diretto con targeting preciso, distanza euclidea e tie-breaking intelligente (Up>Left>Down>Right).
- **Anti-reverse**: I fantasmi non possono invertire immediatamente la direzione per un comportamento più realistico.
- **Wrap-around corretto**: Funzionamento perfetto del teletrasporto per Pac-Man e fantasmi.

## Prossime Release (Roadmap)

### Release 5: AI Complete
- **Blinky**: Reintrodurre modalità scatter con timing corretto
- **Pinky**: AI che punta 4 tile davanti a Pac-Man
- **Inky**: AI collaborativa basata su Blinky e Pac-Man
- **Clyde**: AI che alterna inseguimento e fuga
- **State Machine**: Implementare correttamente Chase/Scatter con timing

### Release 6: Power Pellets & Frightened Mode
- Power Pellet negli angoli della mappa
- Modalità Frightened: fantasmi blu e vulnerabili
- Sistema di punteggi progressivi per fantasmi mangiati
- Suoni di base (chomp, sirene, fantasmi mangiati)

### Release 7: Levels & Progression
- Sistema multi-livello con difficoltà crescente
- Velocità variabili per Pac-Man e fantasmi
- Bonus fruit intermittenti
- Animazioni migliorate e sprite

### Release 8: Polish & Features
- Menu principale e pause
- High score persistente
- Effetti sonori completi
- Animazioni di morte e transizioni

## Aggiungere nuove mappe

1. Copia un file `.txt` in `assets/`, mantenendo convenzione ASCII:
   - `1` = muro, `0` = corridoio, `P` = spawn di Pac-Man.
2. Avvia l’eseguibile e verrà caricata la mappa `map1.txt`.
3. Per cambiare mappa, modifica la stringa in `main.cpp` o aggiungi parametro da linea di comando.

---

Buon divertimento!


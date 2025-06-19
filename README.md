# Pacman-SFML Release 1

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
- Il punteggio aumenta di 10 punti per pellet.

## Funzionalità della Release 1 (MVP)

1. Lettura mappa da file ASCII (`assets/map1.txt`).
2. Movimento “a griglia” con svolte e collisioni corrette.
3. Rimozione dei pellet e conteggio punteggio.
4. Finestra e loop di gioco con 60 FPS.

## Miglioramenti previsti (Release 2+)

- Menu iniziale e schermata "Game Over".
- Salvataggio/lettura degli highscore.
- Aggiunta di fantasmi con IA base.
- Power-up (Super Pellet) e modalità frightened.

## Aggiungere nuove mappe

1. Copia un file `.txt` in `assets/`, mantenendo convenzione ASCII:
   - `1` = muro, `0` = corridoio, `P` = spawn di Pac-Man.
2. Avvia l’eseguibile e verrà caricata la mappa `map1.txt`.
3. Per cambiare mappa, modifica la stringa in `main.cpp` o aggiungi parametro da linea di comando.

---

Buon divertimento!


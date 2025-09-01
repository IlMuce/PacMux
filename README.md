# PacMux
[![CI](https://github.com/IlMuce/PacMux/actions/workflows/ci.yml/badge.svg)](https://github.com/IlMuce/PacMux/actions/workflows/ci.yml)
![Repo Size](https://img.shields.io/github/repo-size/IlMuce/PacMux)
[![SFML](https://img.shields.io/badge/SFML-3.0.1-green?logo=sfml)](https://www.sfml-dev.org/)

Questo repository contiene una versione base funzionante di un clone di Pac-Man realizzato con SFML 3.0 e CMake.

## Questo branch (ConsegnaProgetto) e superbuild CMake

Questo branch è dedicato esclusivamente alla consegna del progetto del corso di Fondamenti di Computer Grafica. Raccoglie TUTTE le release sviluppate (PacMux-0.1.0 → 1.2.0) in cartelle separate per mostrare l'evoluzione del lavoro.

Il CMake di radice non compila un solo gioco, ma funge da superbuild che:
- scarica e configura automaticamente SFML 3.0.1 tramite FetchContent (non serve installare SFML a parte);
- configura/compila ogni cartella PacMux-<version> come progetto indipendente (niente conflitti di target);
- lascia gli eseguibili dentro build/… con la cartella assets copiata accanto all'exe.

Come compilare con il superbuild
- Configurazione unica del workspace:
    - cmake -S . -B build -G "Visual Studio 17 2022" -A x64
- Compilare tutte le release in una volta:
    - cmake --build build --config Release --target ALL_BUILD -j 8
- Compilare una singola release (esempi):
    - cmake --build build --config Release --target PacMux_0_9_0
    - cmake --build build --config Release --target PacMux_1_2_0

Dove trovare gli eseguibili (Release)
- build/PacMux-<versione>-build/PacmanR<versione>.exe

## Requisiti

- Windows 10/11
- Visual Studio Build Tools 2022
- SFML 3.0.0 (viene fetchata automaticamente dal CMake in questo branch; non è necessaria un'installazione manuale)

## Come compilare

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```
L’eseguibile verrà generato in `build/PacmanR12.exe` e dovrà essere eseguito da lì affinché trovi la cartella `assets` al suo fianco.

Oppure da VS Code (Command Palette):
- Ctrl+Shift+P → "CMake: Select a Kit" → scegli Visual Studio 2022 x64.
- Ctrl+Shift+P → "CMake: Configure".
- Barra di stato: seleziona Configurazione "Release".
- Ctrl+Shift+P → "CMake: Build".

## Come si gioca

Obiettivo e punteggi
- Mangia tutti i pellet per completare il livello.
- I Super Pellet mettono i fantasmi in modalità frightened per un breve periodo: puoi mangiarli in combo da 200/400/800/1600 punti.
- I frutti compaiono quando hai mangiato 30 e 70 pellet; rimangono per 10 secondi e danno punti bonus in base al tipo.
- Vita extra a 10.000 punti.
- Usa i tunnel laterali per sfruttare il wrap-around e seminare i fantasmi.

Controlli durante il gioco
- Movimento: Frecce o WASD
- Pausa: P (il gioco si mette in pausa e puoi riprendere con P o dal menu pausa)

Menu principale
- Navigazione: Frecce Su/Giù
- Seleziona voce: Invio

Game Over
- Riprova: Invio
- Torna al menu: M oppure Esc
- Vai ai record locali: H

Schermata Record (Highscore)
- Torna al menu: Esc

Classifica Globale
- Aggiorna: R
- Scorri: Frecce Su/Giù (±1), Pag↑/Pag↓ (±5)
- Inizio/Fine: Home/End
- Torna al menu: Esc

Inserimento nome
- Digita lettere/numeri/"_"/"-" (max 10 caratteri)
- Cancella: Backspace
- Conferma: Invio

## Funzionalità della Release 1

1. Lettura mappa da file ASCII (`assets/map1.txt`).
2. Movimento tile-based fluido di Pac-Man tramite le frecce direzionali.
3. Raccolta automatica dei pellet e incremento del punteggio a schermo.
4. Finestra di gioco con framerate fisso a 60 FPS.
5. Gestione asset (font e mappa) automatica: basta la cartella `assets` accanto all'eseguibile.
 
    ![output](https://github.com/user-attachments/assets/73a952b8-b8e3-45b0-867f-6b1cd62b1418)

## Funzionalità della Release 2

- Implementato il teletrasporto ai bordi (wrap-around): Pac-Man che esce da un lato o dall'alto/basso riappare dal lato opposto, come nei classici arcade.
- Mappa più grande, simmetrica e ispirata ai vecchi Pac-Man, con tunnel orizzontali e verticali funzionanti.
- Possibilità di testare facilmente il wrap-around grazie a corridoi e tunnel dedicati.
- Codice pronto per l'estensione con nuove feature (fantasmi, power-up, ecc.).

    ![output](https://github.com/user-attachments/assets/bdbaf048-43ed-4df1-96fc-2cc385fbd6b3)

## Funzionalità della Release 3

- Aggiunta dei fantasmi statici, posizionati nella ghost house centrale, ognuno con un colore diverso.
- Gestione collisione tra Pac-Man e i fantasmi: se avviene, viene mostrato un messaggio di Game Over e la partita si resetta (Pac-Man torna allo spawn, punteggio e pellet vengono azzerati).

    ![output1](https://github.com/user-attachments/assets/f2788ea2-e5e6-4750-bb9f-d4a722b0cff0)
  
- Gestione della vittoria: se tutti i pellet vengono raccolti, viene mostrato un messaggio di vittoria e la partita si resetta.
- I pellet non vengono generati nella ghost house né nella cella di spawn di Pac-Man.
- Tutte le transizioni avvengono senza chiudere la finestra di gioco.

    ![output2](https://github.com/user-attachments/assets/7863630d-c67f-4165-add6-0d7051ad4c79)

## Funzionalità della Release 4

- **Mappa fedele all'originale**: Aggiornata `map1.txt` con dimensioni e layout più vicini al Pac-Man classico.
- **Tile speciali**: Introdotti tile '2' per spazi vuoti senza pellet (es: tunnel, aree speciali).
- **Ghost House migliorata**: I fantasmi spawano nella casa centrale e non possono rientrarvi una volta usciti.
- **Generazione pellet ottimizzata**: I pellet vengono creati solo sui tile '0', escludendo tile '2' e spawn di Pac-Man.
- **Refactoring architetturale**: Tutti gli header sono in `include/`, i sorgenti in `src/`. Ogni fantasma ha il suo file (`Blinky`, `Pinky`, `Inky`, `Clyde`).
- **Ghost base class**: Tutta la logica comune (movimento, pathfinding greedy, tunnel, ghost house, scatter/chase) è in `Ghost.cpp`/`Ghost.hpp`.
- **AI fedele all'originale**: Blinky insegue Pac-Man, Pinky mira 4 celle avanti, entrambi con pathfinding greedy (distanza euclidea, tie-breaking Up>Left>Down>Right, no inversione immediata).
- **Pronto per estensioni**: Struttura pronta per logiche uniche di Inky e Clyde, modalità frightened, animazioni, ecc.

    ![output](https://github.com/user-attachments/assets/a40a07b6-cbf2-4b1e-b2a6-267aff7d239d)

### Funzionalità della Release 5
- I fantasmi entrano in modalità frightened (blu, movimento casuale e rallentato) quando Pac-Man mangia un Super Pellet.
- I fantasmi possono essere mangiati da Pac-Man in modalità frightened: diventano "occhi", tornano alla ghost house, attendono e poi respawnano normalmente.
- Implementato il timer frightened, con effetto lampeggiante prima della fine.
- Tutti i fantasmi (Blinky, Pinky, Inky, Clyde) condividono la stessa logica di frightened/eaten/respawn, senza duplicazione di codice.
- Alternanza classica scatter/chase con timer configurabili e debug log dettagliato.
- Reset completo degli stati dei fantasmi su game over o vittoria.

    ![output](https://github.com/user-attachments/assets/f0772c55-40f5-4752-9252-bf1107a24173)

### Funzionalità della Release 6
- Sistema multi-livello con caricamento dinamico
- Velocità variabili per Pac-Man e fantasmi
- Nuova mappa basata su Ms. Pacman

    ![output2](https://github.com/user-attachments/assets/5824a764-4568-4465-b211-ad4bacca37c8)

- Implementato reset con aumento di difficoltá una volta finiti tutti i livelli a disposizione
- Corrette tempistiche di uscita fantasmi dalla ghost house e diminuito scatter time
- Aggiunte scritte di vittoria e sconfitta con grafica SFML al posto di console windows
- Cambiato font principale 

    ![output1](https://github.com/user-attachments/assets/5744841d-4a1a-40da-aea4-a8bc251b9b22)

### Funzionalità della Release 7
- Gestione avanzata stati di gioco.
- Aggiunta menu principale, menu di pausa, schermata Game Over.
- HUD completo con visualizzazione punteggio, vite rimaste e livello attuale.
- Vita extra una volta raggiunti 10k punti.
- Aggiunta possibilitá di compilazione con dll statici (non serve averli inclusi nel PATH di sistema).

    ![output](https://github.com/user-attachments/assets/c11444c4-b509-42ef-a28d-0e3cfaf667fa)

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

    ![output1](https://github.com/user-attachments/assets/4ca9445a-46a5-4d38-a022-7cc1ebda4f32)

- **Mappa 3 con tema arancione**: Stile Ms. Pac-Man con colori arancione chiaro
- **Bug fix critici**: Risolti chomp infinito, highscore dopo 7° record, parser JSON robusto

    ![output2](https://github.com/user-attachments/assets/a49ae4c4-a41d-4b60-b6e2-51136e71c5e0)

## Funzionalità della Release 10
- Aggiunte texture e animazioni per Pacman (movimento in ogni direzione e morte).
- Aggiunte texture e animazioni per tutti e 4 i Fantasmini (movimento, modalitá frightened e "occhi" una volta mangiati).
- Modificati dimensione e colore di Pellet e Super Pellet per renderli piú simili all'originale.
- Aggiunta punteggio a vista e moltiplicatore quando Pacman mangia i fantasmini.
- Corretti svariati bug legati a fantasmi, stati di gioco e audio.

    ![output](https://github.com/user-attachments/assets/a0aa3e91-802a-4df3-9622-abc6cbecbcc6)

## Funzionalità della Release 11
- Classifica globale online (Top 50): a fine partita puoi caricare il punteggio e vederlo nella leaderboard dal menu.
- Scorrimento e refresh: frecce/PgUp/PgDn/Home/End per scorrere, R per aggiornare.
- Aggiornamenti rapidi e senza blocchi: upload/download asincroni.
- Dati su GitHub in formato JSON, parser robusto e fallback offline.
- Implementazione: HTTP con CPR; lettura via GitHub Raw; scrittura via GitHub Contents API (Base64 + SHA).

    ![output](https://github.com/user-attachments/assets/454bd35f-59a3-4d2c-88ad-615c1b1660b8)

## Funzionalità della Release 12
- Cibo di gioco (4 tipi): Mela, Banana, Fungo, Uovo.
  - Spawn casuale al raggiungimento di 30 e 70 pellet mangiati.
  - Posizionamento su una posizione di pellet rimanenti per garantire visibilità e accessibilità.
  - Niente duplicati: i due frutti del livello sono sempre di tipo diverso.
  - Despawn automatico dopo 10 secondi se non raccolti.
  - Punteggi e probabilità di comparsa:
    - Mela: 100 pt — 40%
    - Banana: 300 pt — 30%
    - Fungo: 500 pt — 20%
    - Uovo: 700 pt — 10%
- Bilanciamento difficoltà dinamico per livello:
  - Velocità dei fantasmi +20% a ogni aumento di difficoltà.
  - Durata frightened −20% per livello (con minimo 1.5s).
  - Ritardi di uscita dalla ghost house −20% per livello (minimo 0.5s).
  - Tempo di attesa in ghost house dopo essere stati mangiati −20% per livello.
- Pulizia sprite sheet:
  - Eliminati i bordi neri laterali in ogni texture usata per evitare artefatti ai bordi durante il rendering.

    ![output](https://github.com/user-attachments/assets/12a54653-afa6-442a-93af-eb46e3b99d7d)

## Prossime Release (Roadmap)

## Release da 13 in poi:
- Aggiunta livelli
- Grafica migliorata
- Miglioramenti texture del labirinto
- Correzione dei bug noti

## Bug Noti
- Fantasmi si bloccano quando tentano di attraversare il teleport finché pacman non li fa entrare in modalitá frightened.
  Soluzione temporanea: divieto di attraversamento dei teleport ai fantasmi.
- Problemi grafici con alcune gpu integrate AMD. 

## Struttura del progetto

```
pacman-sfml/
├── assets/            # Risorse di gioco (mappe, font, sprite, audio)
│   ├── map1.txt
│   ├── map2.txt
│   ├── map3.txt
│   ├── pacman.ttf
│   ├── pacman.png
│   └── audio/
│       ├── pacman_beginning.wav
│       ├── PacmanChomp.mp3
│       ├── pacman_chomp.wav
│       ├── pacman_eatghost.wav
│       ├── pacman_death.wav
│       ├── pacman_menupausa.wav
│       ├── GhostTurntoBlue.mp3
│       ├── GhostReturntoHome.mp3
│       └── GhostNormalMove.mp3
├── include/           # Header C++
│   ├── Blinky.hpp
│   ├── Clyde.hpp
│   ├── Fruit.hpp
│   ├── Ghost.hpp
│   ├── GlobalLeaderboard.hpp
│   ├── HighScore.hpp
│   ├── Inky.hpp
│   ├── Pellet.hpp
│   ├── Pinky.hpp
│   ├── Player.hpp
│   ├── Score.hpp
│   └── TileMap.hpp
├── src/               # Codice sorgente C++
│   ├── Blinky.cpp
│   ├── Clyde.cpp
│   ├── Fruit.cpp
│   ├── Ghost.cpp
│   ├── GlobalLeaderboard.cpp
│   ├── HighScore.cpp
│   ├── Inky.cpp
│   ├── main.cpp
│   ├── Pellet.cpp
│   ├── Pinky.cpp
│   ├── Player.cpp
│   ├── Score.cpp
│   └── TileMap.cpp
├── CMakeLists.txt     # Configurazione di build
└── README.md
```

## Attenzione: file richiesti e posizione

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

---

Buon divertimento!

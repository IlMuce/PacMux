cmake_minimum_required(VERSION 3.24)
project(PacmanR12 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Nota: evitare di forzare globalmente la build statica per non influenzare le dipendenze FetchContent
# (in particolare cpr/curl su MSYS2). Se vuoi eseguibile statico, gestiscilo in modo mirato in un secondo momento.
# set(BUILD_SHARED_LIBS OFF)

include(FetchContent)

# Sempre usa FetchContent per SFML 3 per garantire coerenza cross‑toolchain (Windows/MSVC, MinGW/Clang, macOS/Clang)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

# Aggiungi CPR per HTTP requests (usato da GlobalLeaderboard)
# Abilita l'uso di libcurl di sistema solo su MinGW/MSYS per evitare estrazione/rename falliti.
if (MINGW OR MSYS)
    set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "Use system-provided libcurl with cpr" FORCE)
else()
    # Su MSVC lasciare OFF: verrà buildata la dipendenza curl tramite FetchContent
    set(CPR_USE_SYSTEM_CURL OFF CACHE BOOL "Use system-provided libcurl with cpr" FORCE)
endif()

FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 3b15fa82ea74739b574d705fea44959b58142eb8) # Versione 1.10.x
FetchContent_MakeAvailable(cpr)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/TileMap.cpp
    src/Player.cpp
    src/Pellet.cpp
    src/Fruit.cpp
    src/Score.cpp
    src/HighScore.cpp
    src/GlobalLeaderboard.cpp
    src/Ghost.cpp
    src/Blinky.cpp
    src/Pinky.cpp
    src/Inky.cpp
    src/Clyde.cpp
)

# RPATH: rendi l'eseguibile eseguibile direttamente da build e dopo install su Linux/macOS
if (APPLE)
    # Usa @loader_path per cercare le dylib accanto all'eseguibile
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH "@loader_path")
elseif(UNIX)
    # Usa $ORIGIN per cercare le .so accanto all'eseguibile
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

# Copia ricorsiva della cartella assets accanto all'exe
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

# Su Windows copia automaticamente anche le DLL dipendenti accanto all'exe
# Esegui solo per build condivise; con build statica la lista è vuota e il comando fallirebbe.
if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
    )
endif()

# Hide console window on Windows; set bundle on macOS for a proper app
# if (WIN32)
#     set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
# elseif(APPLE)
#     set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
# endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE src include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        cpr::cpr
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
)

# Linka il wrapper WinMain solo su Windows per usare int main() con sottosistema GUI
if (WIN32 AND TARGET SFML::Main)
    target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Main)
endif()

# Regole di installazione generiche (bin + assets)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE  DESTINATION .
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION bin)
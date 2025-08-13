cmake_minimum_required(VERSION 3.24)
project(PacmanR10 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Costruisci SFML staticamente, non condivisa
#set(BUILD_SHARED_LIBS OFF)

# Opzionale: forza definizione per find_package
#set(SFML_STATIC_LIBRARIES TRUE)

# Trova SFML 3, richiedendo i componenti Graphics, Window, System e Audio
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)

# Aggiungi CPR per HTTP requests (usato da GlobalLeaderboard)
include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 3b15fa82ea74739b574d705fea44959b58142eb8) # Versione 1.10.x
FetchContent_MakeAvailable(cpr)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/TileMap.cpp
    src/Player.cpp
    src/Pellet.cpp
    src/Score.cpp
    src/HighScore.cpp
    src/GlobalLeaderboard.cpp
    src/Ghost.cpp
    src/Blinky.cpp
    src/Pinky.cpp
    src/Inky.cpp
    src/Clyde.cpp
)

# Copia ricorsiva della cartella assets accanto all'exe
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

target_include_directories(${PROJECT_NAME}
    PRIVATE src include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio cpr::cpr
)
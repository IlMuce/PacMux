cmake_minimum_required(VERSION 3.24)
project(PacmanR9 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Costruisci SFML staticamente, non condivisa
#set(BUILD_SHARED_LIBS OFF)

# Opzionale: forza definizione per find_package
#set(SFML_STATIC_LIBRARIES TRUE)

# Trova SFML 3, richiedendo i componenti Graphics, Window, System e Audio
if(NOT TARGET SFML::Graphics)
    find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)
endif()

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/TileMap.cpp
    src/Player.cpp
    src/Pellet.cpp
    src/Score.cpp
    src/HighScore.cpp
    src/Ghost.cpp
    src/Blinky.cpp
    src/Pinky.cpp
    src/Inky.cpp
    src/Clyde.cpp
)

# RPATH per macOS / Linux
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_RPATH "@loader_path;$<TARGET_FILE_DIR:SFML::Graphics>"
    INSTALL_RPATH "@loader_path"
    BUILD_WITH_INSTALL_RPATH ON
    INSTALL_RPATH_USE_LINK_PATH OFF
    MACOSX_RPATH ON
    )
elseif(UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUILD_RPATH "\$ORIGIN"
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Copia ricorsiva della cartella assets accanto all'exe
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

if(APPLE AND BUILD_SHARED_LIBS)
    set(_sfml_libs
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::System>
    )
    if(TARGET SFML::Audio)
        list(APPEND _sfml_libs $<TARGET_FILE:SFML::Audio>)
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${_sfml_libs}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND /usr/bin/install_name_tool -change $<TARGET_FILE:SFML::Graphics> @loader_path/$<TARGET_FILE_NAME:SFML::Graphics> "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMAND /usr/bin/install_name_tool -change @rpath/libsfml-graphics.3.0.dylib @loader_path/$<TARGET_FILE_NAME:SFML::Graphics> "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMAND /usr/bin/install_name_tool -change @rpath/libsfml-graphics.3.dylib   @loader_path/$<TARGET_FILE_NAME:SFML::Graphics> "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMAND /usr/bin/install_name_tool -change $<TARGET_FILE:SFML::Window>   @loader_path/$<TARGET_FILE_NAME:SFML::Window>   "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMAND /usr/bin/install_name_tool -change @rpath/libsfml-window.3.0.dylib   @loader_path/$<TARGET_FILE_NAME:SFML::Window>   "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMAND /usr/bin/install_name_tool -change @rpath/libsfml-window.3.dylib     @loader_path/$<TARGET_FILE_NAME:SFML::Window>   "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMAND /usr/bin/install_name_tool -change $<TARGET_FILE:SFML::System>   @loader_path/$<TARGET_FILE_NAME:SFML::System>   "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMAND /usr/bin/install_name_tool -change @rpath/libsfml-system.3.0.dylib   @loader_path/$<TARGET_FILE_NAME:SFML::System>   "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMAND /usr/bin/install_name_tool -change @rpath/libsfml-system.3.dylib     @loader_path/$<TARGET_FILE_NAME:SFML::System>   "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMAND_EXPAND_LISTS
    )
    if(TARGET SFML::Audio)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND /usr/bin/install_name_tool -change $<TARGET_FILE:SFML::Audio> @loader_path/$<TARGET_FILE_NAME:SFML::Audio> "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMAND /usr/bin/install_name_tool -change @rpath/libsfml-audio.3.0.dylib   @loader_path/$<TARGET_FILE_NAME:SFML::Audio> "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMAND /usr/bin/install_name_tool -change @rpath/libsfml-audio.3.dylib     @loader_path/$<TARGET_FILE_NAME:SFML::Audio> "$<TARGET_FILE:${PROJECT_NAME}>"
        )
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND /usr/bin/install_name_tool -id @loader_path/$<TARGET_FILE_NAME:SFML::Graphics> "$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_NAME:SFML::Graphics>"
        COMMAND /usr/bin/install_name_tool -id @loader_path/$<TARGET_FILE_NAME:SFML::Window>   "$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_NAME:SFML::Window>"
        COMMAND /usr/bin/install_name_tool -id @loader_path/$<TARGET_FILE_NAME:SFML::System>   "$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_NAME:SFML::System>"
        COMMAND_EXPAND_LISTS
    )
    if(TARGET SFML::Audio)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND /usr/bin/install_name_tool -id @loader_path/$<TARGET_FILE_NAME:SFML::Audio> "$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_NAME:SFML::Audio>"
        )
    endif()
endif()

# Su Windows, copia automaticamente le DLL dipendenti accanto all'exe in build condivisa
if (WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
    )
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE src include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio
)

# Linka il wrapper WinMain solo su Windows per usare int main() con sottosistema GUI
if (WIN32 AND TARGET SFML::Main)
    target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Main)
endif()
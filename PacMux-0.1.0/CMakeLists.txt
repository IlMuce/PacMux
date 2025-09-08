cmake_minimum_required(VERSION 3.24)
project(PacmanR1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET SFML::Graphics)
    find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)
endif()

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/TileMap.cpp
    src/Player.cpp
    src/Pellet.cpp
    src/Score.cpp
)

# RPATH per macOS / Linux per trovare le librerie SFML condivise accanto all'eseguibile
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUILD_RPATH "@loader_path"
        INSTALL_RPATH "@loader_path"
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

# Copia le dylib SFML accanto all'eseguibile (macOS) per esecuzione standalone
if(APPLE AND BUILD_SHARED_LIBS)
    get_target_property(_sfml_graphics_lib SFML::Graphics LOCATION)
    if(_sfml_graphics_lib)
        get_filename_component(_sfml_lib_dir "${_sfml_graphics_lib}" DIRECTORY)
        set(_needed_libs
            "${_sfml_lib_dir}/libsfml-graphics.3.dylib"
            "${_sfml_lib_dir}/libsfml-window.3.dylib"
            "${_sfml_lib_dir}/libsfml-system.3.dylib"
        )
        if(TARGET SFML::Audio)
            list(APPEND _needed_libs "${_sfml_lib_dir}/libsfml-audio.3.dylib")
        endif()
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_needed_libs} $<TARGET_FILE_DIR:${PROJECT_NAME}>
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

# Dì a CMake che vogliamo un EXE di tipo GUI (subsystem=windows)
# set_target_properties(${PROJECT_NAME}
#    PROPERTIES
#        WIN32_EXECUTABLE TRUE
# )

target_include_directories(${PROJECT_NAME}
    PRIVATE src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE SFML::Graphics SFML::Window SFML::System
)

# Linka il wrapper WinMain solo su Windows per usare int main() con sottosistema GUI
if (WIN32 AND TARGET SFML::Main)
    target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Main)
endif()

cmake_minimum_required(VERSION 3.24)
project(PacMuxSuite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Consenti ai subprogetti legacy di configurarsi con CMake 4.x
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Costruiamo staticamente di default (ogni subprogetto può sovrascrivere)
set(BUILD_SHARED_LIBS OFF)

# Porta SFML via FetchContent (usato da TUTTE le release)
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
)
FetchContent_MakeAvailable(SFML)

include(ExternalProject)

function(add_release name dir)
    ExternalProject_Add(${name}
        SOURCE_DIR "${CMAKE_SOURCE_DIR}/${dir}"
        BINARY_DIR "${CMAKE_BINARY_DIR}/${dir}-build"
        CMAKE_ARGS
            -DCMAKE_CXX_STANDARD=20
            -DCMAKE_CXX_STANDARD_REQUIRED=ON
            -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/_install/${dir}
            -DSFML_DIR=${sfml_BINARY_DIR}/lib/cmake/SFML
        INSTALL_COMMAND ""
    )
endfunction()

# Aggiungi tutte le release come ExternalProject (niente collisioni di target)
add_release(PacMux_0_1_0 PacMux-0.1.0)
add_release(PacMux_0_2_0 PacMux-0.2.0)
add_release(PacMux_0_3_0 PacMux-0.3.0)
add_release(PacMux_0_4_0 PacMux-0.4.0)
add_release(PacMux_0_5_0 PacMux-0.5.0)
add_release(PacMux_0_6_0 PacMux-0.6.0)
add_release(PacMux_0_7_0 PacMux-0.7.0)
add_release(PacMux_0_8_0 PacMux-0.8.0)
add_release(PacMux_0_9_0 PacMux-0.9.0)
add_release(PacMux_1_0_0 PacMux-1.0.0)
add_release(PacMux_1_1_0 PacMux-1.1.0)
add_release(PacMux_1_2_0 PacMux-1.2.0)
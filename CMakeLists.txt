cmake_minimum_required(VERSION 3.24)
project(PacMuxSuite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Consenti ai subprogetti legacy di configurarsi con CMake 4.x
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Non forzare BUILD_SHARED_LIBS a livello globale; lascia ai subprogetti o alla toolchain

# Prepara SFML come ExternalProject e installalo in un prefix locale
include(ExternalProject)

# Determina shared/static per SFML in base a BUILD_SHARED_LIBS della superbuild
if (DEFINED BUILD_SHARED_LIBS)
    if (BUILD_SHARED_LIBS)
        set(_sfml_build_shared ON)
    else()
        set(_sfml_build_shared OFF)
    endif()
else()
    set(_sfml_build_shared ON)
endif()

set(SFML_INSTALL_DIR ${CMAKE_BINARY_DIR}/_sfml_install)

ExternalProject_Add(SFML_ext
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.1
    PREFIX ${CMAKE_BINARY_DIR}/_deps/sfml
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${SFML_INSTALL_DIR}
        -DCMAKE_CXX_STANDARD=20
        -DCMAKE_CXX_STANDARD_REQUIRED=ON
        -DBUILD_SHARED_LIBS=${_sfml_build_shared}
    INSTALL_DIR ${SFML_INSTALL_DIR}
)

function(add_release name dir)
    # Raccogli argomenti comuni da propagare ai subprogetti
    set(_extra_args)
    if (DEFINED CMAKE_C_COMPILER)
        list(APPEND _extra_args -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER})
    endif()
    if (DEFINED CMAKE_CXX_COMPILER)
        list(APPEND _extra_args -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER})
    endif()

    # Allinea la scelta static/shared con SFML: se BUILD_SHARED_LIBS=OFF, chiedi statiche esplicitamente
    if (DEFINED BUILD_SHARED_LIBS)
        if (BUILD_SHARED_LIBS)
            set(_build_shared_val ON)
            set(_sfml_static_val OFF)
        else()
            set(_build_shared_val OFF)
            set(_sfml_static_val ON)
        endif()
    else()
        # Default: shared
        set(_build_shared_val ON)
        set(_sfml_static_val OFF)
    endif()

    ExternalProject_Add(${name}
        SOURCE_DIR "${CMAKE_SOURCE_DIR}/${dir}"
        BINARY_DIR "${CMAKE_BINARY_DIR}/${dir}-build"
        CMAKE_ARGS
            -DCMAKE_CXX_STANDARD=20
            -DCMAKE_CXX_STANDARD_REQUIRED=ON
            -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/_install/${dir}
            -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON
            -DCMAKE_PREFIX_PATH=${SFML_INSTALL_DIR}
            ${_extra_args}
        CMAKE_CACHE_ARGS
            -DSFML_DIR:PATH=${SFML_INSTALL_DIR}/lib/cmake/SFML
            -DBUILD_SHARED_LIBS:BOOL=${_build_shared_val}
            -DSFML_STATIC_LIBRARIES:BOOL=${_sfml_static_val}
        DEPENDS SFML_ext
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
cmake_minimum_required(VERSION 3.10)
project(BreakOut VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configuration de GLFW comme sous-module
# Désactiver les composants de GLFW que nous n'utilisons pas
set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW documentation" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW tests" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW examples" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Generate GLFW installation target" FORCE)

# Ajouter le sous-module GLFW au projet
add_subdirectory(external/glfw)

# Add executable
add_executable(BreakOut breakout.cpp
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl2.cpp
)

# Include directories
target_include_directories(BreakOut PRIVATE
        imgui
        imgui/backends
        ${CMAKE_SOURCE_DIR}/imgui
        ${CMAKE_SOURCE_DIR}/imgui/backends
        ${CMAKE_SOURCE_DIR}/external/glfw/include  # Inclure les headers de GLFW
)

# Options spécifiques à la plateforme
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(BreakOut PRIVATE
            glfw
            "-framework OpenGL"
            ${COCOA_LIBRARY}
            ${IOKIT_LIBRARY}
            ${COREVIDEO_LIBRARY}
    )
    target_compile_definitions(BreakOut PRIVATE GL_SILENCE_DEPRECATION)
elseif(WIN32)
    find_package(OpenGL REQUIRED)

    # Lier avec la cible glfw déjà configurée par le sous-module
    target_link_libraries(BreakOut PRIVATE
            glfw
            OpenGL::GL
    )

    # Ajouter les bibliothèques système Windows
    if(MINGW)
        target_link_libraries(BreakOut PRIVATE gdi32 user32 shell32)

        # Optimisations pour MinGW en mode Release
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            # Linking statique des bibliothèques standard
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
            # Utiliser le sous-système Windows pour GUI (pas de console)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
        endif()
    else()
        target_link_libraries(BreakOut PRIVATE gdi32 user32 shell32)

        # Configuration pour Visual Studio
        if(MSVC)
            set_target_properties(BreakOut PROPERTIES WIN32_EXECUTABLE TRUE)
        endif()
    endif()
elseif(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)

    target_link_libraries(BreakOut PRIVATE
            glfw
            OpenGL::GL
            ${X11_LIBRARIES}
            ${CMAKE_THREAD_LIBS_INIT}
            ${CMAKE_DL_LIBS}
    )
endif()

# Définir le répertoire de sortie
set_target_properties(BreakOut PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
cmake_minimum_required(VERSION 3.10)
project(BreakOut VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
)

# Link GLFW and OpenGL libraries
if(APPLE)
    target_link_libraries(BreakOut PRIVATE glfw "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
elseif(WIN32)
    # Windows-specific configuration
    find_package(OpenGL REQUIRED)
    target_link_libraries(BreakOut PRIVATE glfw OpenGL::GL)

    # Ajout des bibliothèques système Windows si nécessaire
    target_link_libraries(BreakOut PRIVATE gdi32 user32 shell32)

    # Configuration pour empêcher la création d'une console
    if(MSVC)
        set_target_properties(BreakOut PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
elseif(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(BreakOut PRIVATE glfw OpenGL::GL X11 pthread dl)
endif()

# Add compile options
if(APPLE)
    target_compile_definitions(BreakOut PRIVATE GL_SILENCE_DEPRECATION)
endif()

# Find and link GLFW
find_package(glfw3 REQUIRED)
target_link_libraries(BreakOut PRIVATE glfw)

# Instructions pour Windows si glfw3 n'est pas trouvé par find_package
if(WIN32 AND NOT glfw3_FOUND)
    message(STATUS "GLFW3 not found via find_package, checking for manual paths")
    # Permettre à l'utilisateur de spécifier le chemin vers GLFW3 manuellement
    set(GLFW3_ROOT_DIR "" CACHE PATH "Root directory of GLFW3 installation")

    if(GLFW3_ROOT_DIR)
        target_include_directories(BreakOut PRIVATE ${GLFW3_ROOT_DIR}/include)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # 64-bit
            target_link_directories(BreakOut PRIVATE ${GLFW3_ROOT_DIR}/lib-vc2019-64)
        else()
            # 32-bit
            target_link_directories(BreakOut PRIVATE ${GLFW3_ROOT_DIR}/lib-vc2019-32)
        endif()
    endif()
endif()
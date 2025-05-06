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
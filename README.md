# BreakOut Game

A simple Breakout clone built with C++, GLFW, and ImGui.

## Dependencies

- C++ compiler with C++14 support
    - Windows: MinGW-w64, MSVC
    - macOS: Clang (via Xcode command line tools)
    - Linux: GCC or Clang
- CMake (3.10 or higher)
- Git
- OpenGL development libraries
    - Already included on macOS
    - Windows: Included with compiler or graphics drivers
    - Linux: Usually package `libgl1-mesa-dev` or equivalent

## Installation Guide

### Required Tools

<details>
<summary><b>Windows</b></summary>

#### Installing MinGW (for Windows without Visual Studio)

1. Download and install MSYS2 from https://www.msys2.org/
2. Open MSYS2 terminal and run:
   ```bash
   pacman -Syu
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake git make
   ```
3. Add MinGW to your system PATH:
   ```
   C:\msys64\mingw64\bin
   ```

#### Installing Git
1. Download and install Git from https://git-scm.com/download/win
2. Choose the default options during installation

#### Installing CMake
1. Download and install CMake from https://cmake.org/download/
2. During installation, select "Add CMake to system PATH"

</details>

<details>
<summary><b>macOS</b></summary>

1. Install Xcode Command Line Tools:
   ```bash
   xcode-select --install
   ```

2. Install Homebrew (if not already installed) from https://brew.sh/

3. Install required dependencies:
   ```bash
   brew install cmake git
   ```

</details>

<details>
<summary><b>Linux (Ubuntu/Debian)</b></summary>

1. Install required packages:
   ```bash
   sudo apt update
   sudo apt install build-essential cmake git libgl1-mesa-dev xorg-dev libxinerama-dev libxcursor-dev libxi-dev
   ```

For other distributions, use the appropriate package manager and equivalent packages.

</details>

## Project Setup

### 1. Clone the repository with submodules

```bash
# Clone the repository
git clone https://github.com/your-username/breakout-game.git

# Navigate to the project directory
cd breakout-game

# Initialize and update submodules
git submodule update --init --recursive
```

### 2. Build the project

<details>
<summary><b>Windows (MinGW)</b></summary>

```bash
# Create a build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "MinGW Makefiles"

# Build the project
cmake --build . --config Release

# The executable will be in build/bin/
```

</details>

<details>
<summary><b>Windows (Visual Studio)</b></summary>

```bash
# Create a build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build . --config Release

# The executable will be in build/Release/ or build/bin/
```

You can also open the generated .sln file with Visual Studio and build from there.

</details>

<details>
<summary><b>macOS</b></summary>

```bash
# Create a build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build . --config Release

# The executable will be in build/bin/
```

</details>

<details>
<summary><b>Linux</b></summary>

```bash
# Create a build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build . --config Release

# The executable will be in build/bin/
```

</details>

### 3. Run the game

<details>
<summary><b>Windows</b></summary>

Navigate to the bin directory and run the executable:
```bash
cd bin
BreakOut.exe
```

</details>

<details>
<summary><b>macOS/Linux</b></summary>

Navigate to the bin directory and run the executable:
```bash
cd bin
./BreakOut
```

</details>

## Project Structure

```
breakout-game/
│
├── CMakeLists.txt          # Main CMake configuration
│
├── breakout.cpp            # Main source code
│
├── imgui/                  # ImGui library files
│   ├── imgui.cpp
│   ├── imgui_draw.cpp
│   ├── imgui_tables.cpp
│   ├── imgui_widgets.cpp
│   └── backends/
│       ├── imgui_impl_glfw.cpp
│       └── imgui_impl_opengl2.cpp
│
└── external/               # External dependencies
    └── glfw/               # GLFW submodule
```

## Troubleshooting

### Common Issues

#### Windows

- **"Cannot find -lglfw3"**: Make sure you cloned the GLFW submodule correctly.
- **Missing DLLs**: If running the executable gives a DLL error, you may need to copy MinGW DLLs to the executable directory.

#### macOS

- **"Framework not found"**: Make sure you have Xcode Command Line Tools installed.
- **OpenGL deprecation warnings**: These can be safely ignored as we've added the `GL_SILENCE_DEPRECATION` flag.

#### Linux

- **Missing X11 libraries**: Install the X11 development packages mentioned in the Linux installation section.
- **OpenGL issues**: Make sure your system has proper graphics drivers installed.

### Still having issues?

1. Make sure all submodules are properly initialized
2. Verify you're using the recommended versions of all tools
3. Check that your graphics drivers are up to date

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- GLFW: https://www.glfw.org/
- ImGui: https://github.com/ocornut/imgui
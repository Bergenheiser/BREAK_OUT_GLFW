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

## Acknowledgments

- GLFW: https://www.glfw.org/
- ImGui: https://github.com/ocornut/imgui

# Authors Note
## Mécaniques de jeu

### Contrôle et physique
- **Mouvement de la raquette par souris** avec déplacement progressif :
    - La position de la souris est convertie en coordonnées du monde de jeu
    - Mouvement lissé avec delta time (`dt`) pour un déplacement fluide et cohérent
    - Vitesse de la raquette liée à celle de la balle pour maintenir un équilibre de difficulté

### Système de la balle
- **Vitesse de base** : `INITIAL_BALL_SPEED = 1.0f`
- **Système d'accélération progressive** :
    - Augmentation par compteur de coups : après 4 et 12 coups
    - Accélération lors du premier contact avec les briques rouges et orange
    - Facteur d'accélération unifié : `BALL_SPEED_INCREMENT = 1.19f` (+19%)
    - Normalisation de la vitesse après chaque accélération pour maintenir un vecteur directionnel cohérent

### Architecture des briques
- **Distribution organisée** en 8 rangées de 14 briques
- **Système de points** dégressif selon la hauteur :
    - Rouge (haut) : 7 points
    - Orange : 5 points
    - Vert : 3 points
    - Jaune (bas) : 1 point

### Briques spéciales
- **Murs indestructibles** aux extrémités supérieures (gris)
- **Murs à rétroréflexion** (blancs) qui inversent la direction complète de la balle
- **Briques à compteur** (une par rangée, position aléatoire mais fixe) nécessitant 2 coups pour être détruites.

### Système de bonus/malus
- **Distribution** : un bloc bonus par rangée à position aléatoire
- **Types de bonus** (8 variantes) :
    - Vie supplémentaire (orange)
    - Retrait d'une vie (rouge)
    - Élargissement de la raquette (+25%)
    - Rétrécissement de la raquette (-25%)
    - Ralentissement de la balle (-20%)
    - Accélération de la balle (+20%)
    - Redressement de trajectoire (composante horizontale réduite)
    - Inclinaison de la trajectoire (composante horizontale augmentée)
- **Mécanique de chute** : les bonus tombent à une vitesse constante (traverse l'écran en 2 secondes)

### Événements spéciaux
- **Rétrécissement de la raquette** lors du premier contact avec le plafond
- **Augmentation incrémentale** de la difficulté par niveau
- **Rebond optimisé** sur la raquette : l'angle dépend de la position d'impact

### Adaptation à la résolution
- **Système d'adaptation dynamique** du jeu à la taille de la fenêtre
- **Mise à l'échelle** des vitesses en fonction des dimensions du monde

## TODO
- [ ] Editeur de niveaux
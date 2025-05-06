#define GL_SILENCE_DEPRECATION // For macOS compatibility if needed
#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <string>

// --- Dear ImGui Headers ---
#include <algorithm>

#include "imgui/imgui.h"                       // Main ImGui header
#include "imgui/backends/imgui_impl_glfw.h"    // GLFW backend
#include "imgui/backends/imgui_impl_opengl2.h" // OpenGL 2 backend

// === Compilation Commands ===
// MACOSX:
// g++ -std=c++11 breakout.cpp imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl2.cpp -o breakout -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
//
// LINUX:
// g++ -std=c++11 breakout.cpp imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl2.cpp -o breakout -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm
// (Make sure necessary -dev packages like libglfw3-dev, libgl1-mesa-dev, xorg-dev are installed)

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
constexpr int WINDOW_WIDTH = 960;
constexpr int WINDOW_HEIGHT = 540;
constexpr char *WINDOW_TITLE = "Breakout C++";

// --- Game Constants ---
constexpr int BRICK_ROWS = 8;
constexpr int BRICKS_PER_ROW = 14;
constexpr float BRICK_GRID_WIDTH = 1.85f; // Slightly reduce grid width for margins
constexpr float BRICK_START_Y = 0.85f;   // Start bricks a bit lower
constexpr float BRICK_HEIGHT = 0.06f;
constexpr float BRICK_GAP = 0.01f;

constexpr float PADDLE_WIDTH = 0.25f;
constexpr float PADDLE_HEIGHT = 0.04f;
constexpr float PADDLE_Y_POSITION = -0.9f;
constexpr float PADDLE_SPEED = 1.5f;

constexpr float BALL_RADIUS = 0.02f;
constexpr float INITIAL_BALL_SPEED = 1.0f;;
constexpr float BALL_SPEED_INCREMENT = 1.19f;

constexpr float REFERENCE_WIDTH = 960.0f;
constexpr float REFERENCE_HEIGHT = 540.0f;
constexpr float BASE_SPEED = 1.0f;

//-----------------------------------------------------------------------------
// Helper Structs
//-----------------------------------------------------------------------------
struct Vec2
{
    Vec2() = default;
    Vec2(float x_val, float y_val) : x(x_val), y(y_val) {};
    float x = 0.0f;
    float y = 0.0f;
};

struct Color
{
    explicit Color(float r_val = 1.0f, float g_val = 1.0f, float b_val = 1.0f, float a_val = 1.0f)
        : r(r_val), g(g_val), b(b_val), a(a_val) {}
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};
// --- Définition des couleurs du jeu ---
enum class BrickColor {
    RED,           // Rouge - lignes 0-1
    ORANGE,        // Orange - lignes 2-3
    GREEN,         // Vert - lignes 4-5
    YELLOW,        // Jaune - lignes 6-7
    GRAY,          // Gris - murs indestructibles
    WHITE,         // Blanc - murs réfléchissants
    PADDLE,        // Couleur de la raquette
    BALL           // Couleur de la balle
};

// Fonction qui retourne une structure Color à partir d'une énumération BrickColor
Color getColorFromEnum(BrickColor colorType, bool isDarker = false, float alpha = 1.0f) {
    Color result;

    switch (colorType) {
        case BrickColor::RED:
            result = Color{1.0f, 0.2f, 0.2f, alpha};
        break;
        case BrickColor::ORANGE:
            result = Color{1.0f, 0.6f, 0.2f, alpha};
        break;
        case BrickColor::GREEN:
            result = Color{0.2f, 1.0f, 0.2f, alpha};
        break;
        case BrickColor::YELLOW:
            result = Color{1.0f, 1.0f, 0.2f, alpha};
        break;
        case BrickColor::GRAY:
            result = Color{0.5f, 0.5f, 0.5f, alpha};
        break;
        case BrickColor::WHITE:
            result = Color{1.0f, 1.0f, 1.0f, alpha};
        break;
        case BrickColor::PADDLE:
            result = Color{0.8f, 0.8f, 0.8f, alpha};
        break;
        case BrickColor::BALL:
            result = Color{1.0f, 1.0f, 1.0f, alpha};
        break;
    }

    // Appliquer l'effet "plus sombre" si demandé (pour les briques à compteur)
    if (isDarker) {
        result.r *= 0.7f;
        result.g *= 0.7f;
        result.b *= 0.7f;
    }

    return result;
}

// Base structure for game objects with position and size
struct GameObject
{
    Vec2 position;
    Vec2 size;
    Color color;
    BrickColor colorType;
};

//-----------------------------------------------------------------------------
// Game Object Structs
//-----------------------------------------------------------------------------
struct Paddle : public GameObject
{
};

struct Ball : public GameObject
{
    Vec2 velocity = Vec2{0.0f, 0.0f};
    float speedMagnitude = INITIAL_BALL_SPEED;
    bool stuckToPaddle = true;
    int hitCount = 0;
};

struct Block : public GameObject
{
    bool active = true;
    int points = 0;
    int hitCounter = 0;     // Compteur de coups nécessaires pour détruire la brique
    bool isWall = false;    // Pour les briques indestructibles
    bool isReflective = false; // Pour les briques à rétroréflexion
    bool isBonus = false;   // Pour les briques bonus
    int bonusType = 0;      // Type de bonus
};

//-----------------------------------------------------------------------------
// Game State Enum
//-----------------------------------------------------------------------------
enum class GameState
{
    MENU,
    PLAYING,
    GAME_OVER
};
// Constantes pour les types de bonus
enum BonusType {
    LIFE_ADD = 0,
    LIFE_REMOVE = 1,
    PADDLE_WIDEN = 2,
    PADDLE_SHRINK = 3,
    BALL_SLOW = 4,
    BALL_FAST = 5,
    BALL_STRAIGHTEN = 6,
    BALL_ANGLE = 7
};

// Structure pour un bonus qui tombe
struct FallingBonus {
    Vec2 position;
    Vec2 size;
    Color color;
    int type{};
    float fallSpeed{};
    bool active = true;
};


//-----------------------------------------------------------------------------
// Game Class
//-----------------------------------------------------------------------------
class Game
{
public:
    Game(int width, int height, const char *title)
        : windowWidth(width), windowHeight(height) // game objects use default constructors
    {
        if (!initGLFW(width, height, title))
        {
            throw std::runtime_error("Failed to initialize GLFW or create window");
        }

        // --- Initialize Dear ImGui ---
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        // Optional: Enable keyboard nav if desired
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark(); // Set default dark style

        // Initialize ImGui Backends
        ImGui_ImplGlfw_InitForOpenGL(window, true); // Installs callbacks
        ImGui_ImplOpenGL2_Init();

        // --- Initialize Game ---
        srand(static_cast<unsigned int>(time(nullptr)));
        glfwSetWindowUserPointer(window, this); // Link GLFW window to this Game instance
        setupCallbacks();                       // Setup non-ImGui callbacks (only framebuffer size needed now)
        updateProjectionMatrix(width, height);  // Initial projection setup

        // Start in the menu state
        currentState = GameState::MENU;
    }

    ~Game()
    {
        // --- Shutdown ImGui ---
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // --- Shutdown GLFW ---
        if (window)
        {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }

    // Main game loop runner
    void run()
    {
        lastTime = glfwGetTime();
        while (!glfwWindowShouldClose(window))
        {
            // --- Timing ---
            double currentTime = glfwGetTime();
            auto deltaTime = static_cast<float>(currentTime - lastTime);
            lastTime = currentTime;

            // --- Start ImGui Frame ---
            ImGui_ImplOpenGL2_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // --- Input & Update ---
            processInput(deltaTime); // Handle keyboard input for game
            update(deltaTime);       // Update game state / simulation

            // --- Rendering ---
            render(); // Render game world and ImGui UI

            // --- Event Handling ---
            glfwPollEvents(); // Process window events
        }
    }

private:
    // --- Window & Graphics ---
    GLFWwindow *window = nullptr;
    int windowWidth;
    int windowHeight;
    float gameBoundX = 1.0f; // World coordinate boundaries (-1.0f to 1.0f)
    float gameBoundY = 1.0f;

    // --- State ---
    GameState currentState = GameState::MENU;

    // --- Game Objects ---
    Paddle playerPaddle;
    Ball gameBall;
    std::vector<Block> blocks;
    int score = 0;
    int lives = 3;
    int currentLevel = 1;

    //--- Bonus Objects ---
    //--- Bonus Objects ---
    std::vector<FallingBonus> fallingBonuses;
    float bonusFallSpeed = 1.0f; // Vitesse pour tomber en 2 secondes
    // --- Timing ---
    double lastTime = 0.0;
    //--- Game Logic ---
    bool firstContactRed;
    bool firstContactOrange;
    bool paddleShrunk;

    // --- Initialization Functions ---
    bool initGLFW(const int& width, const int& height, const char *title)
    {
        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        // Request OpenGL 2.1 context (compatible with ImGui OpenGL2 backend)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

        window = glfwCreateWindow(width, height, title, NULL, NULL);
        updateProjectionMatrix(width, height);
        if (!window)
        {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable V-Sync
        return true;
    }

    // Setup GLFW callbacks NOT handled by ImGui
    void setupCallbacks()
    {
        // ImGui_ImplGlfw_InitForOpenGL installs its own handlers for most inputs.
        // We only need to keep the framebuffer size callback.
        glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
        // glfwSetKeyCallback(window, keyCallback); // Can be removed if only polling keys
    }

    void initGame()
    {
        score = 0;
        lives = 3;
        currentLevel = 1;
        firstContactRed=true;
        firstContactOrange=true;
        paddleShrunk=false;
        // currentState is set to PLAYING *before* calling this
        initBlocks();
        resetPlayerAndBall();
    }

    void spawnBonus(const Block &block)
    {
            FallingBonus bonus;
            bonus.position = block.position;
            bonus.size = Vec2{gameBall.size.x,gameBall.size.y}; // Plus petit que la brique
            bonus.type = block.bonusType;
            bonus.fallSpeed = bonusFallSpeed;
            bonus.active = true;

        switch (bonus.type) {
            case LIFE_ADD: bonus.color = Color{1.0f, 0.5f, 0.0f, 1.0f}; break; // Orange
            case LIFE_REMOVE: bonus.color = Color{1.0f, 0.0f, 0.0f, 1.0f}; break; // Rouge
            case PADDLE_WIDEN: bonus.color = Color{1.0f, 1.0f, 0.0f, 1.0f}; break; // Jaune
            case PADDLE_SHRINK: bonus.color = Color{0.0f, 1.0f, 0.0f, 1.0f}; break; // Vert
            case BALL_SLOW: bonus.color = Color{0.0f, 1.0f, 1.0f, 1.0f}; break; // Cyan
            case BALL_FAST: bonus.color = Color{0.0f, 0.0f, 1.0f, 1.0f}; break; // Bleu
            case BALL_STRAIGHTEN: bonus.color = Color{1.0f, 1.0f, 1.0f, 1.0f}; break; // Blanc
            case BALL_ANGLE: bonus.color = Color{0.5f, 0.5f, 0.5f, 1.0f}; break; // Gris
            default: bonus.color = Color{1.0f, 1.0f, 1.0f, 1.0f}; // Blanc par défaut
        }

            fallingBonuses.push_back(bonus);
    }
    void applyBonus(const FallingBonus &bonus)
{
    switch (bonus.type) {
        case LIFE_ADD:
            lives = std::min(lives + 1, 5); // Maximum 5 vies
            break;
        case LIFE_REMOVE:
            lives = std::max(lives - 1, 1); // Minimum 1 vie
            break;
        case PADDLE_WIDEN:
            playerPaddle.size.x *= 1.25f; // 25% plus large
            playerPaddle.size.x = std::min(playerPaddle.size.x, gameBoundX * 0.75f); // Limiter la taille
            break;
        case PADDLE_SHRINK:
            playerPaddle.size.x *= 0.75f; // 25% plus petit
            playerPaddle.size.x = std::max(playerPaddle.size.x, PADDLE_WIDTH * 0.5f); // Taille minimale
            break;
        case BALL_SLOW:
            gameBall.speedMagnitude *= 0.8f; // 20% plus lente
            normalizeVelocity();
            break;
        case BALL_FAST:
            gameBall.speedMagnitude *= 1.2f; // 20% plus rapide
            normalizeVelocity();
            break;
        case BALL_STRAIGHTEN:
            // Redresser la trajectoire
            if (std::abs(gameBall.velocity.x) > 0.1f) {
                float sign = gameBall.velocity.x > 0 ? 1.0f : -1.0f;
                gameBall.velocity.x = sign * gameBall.speedMagnitude * 0.2f; // Réduit la composante horizontale
                gameBall.velocity.y = gameBall.velocity.y > 0 ?
                    std::sqrt(gameBall.speedMagnitude * gameBall.speedMagnitude - gameBall.velocity.x * gameBall.velocity.x) :
                    -std::sqrt(gameBall.speedMagnitude * gameBall.speedMagnitude - gameBall.velocity.x * gameBall.velocity.x);
            }
            break;
        case BALL_ANGLE:
            // Incliner davantage la trajectoire
            if (std::abs(gameBall.velocity.y) > 0.1f) {
                float sign = gameBall.velocity.x > 0 ? 1.0f : -1.0f;
                gameBall.velocity.x = sign * gameBall.speedMagnitude * 0.8f; // Augmente la composante horizontale (80% de la vitesse normalisée est horizontale
                gameBall.velocity.y = gameBall.velocity.y > 0 ?
                    std::sqrt(gameBall.speedMagnitude * gameBall.speedMagnitude - gameBall.velocity.x * gameBall.velocity.x) :
                    -std::sqrt(gameBall.speedMagnitude * gameBall.speedMagnitude - gameBall.velocity.x * gameBall.velocity.x);
            }
            break;
        default: break;
    }
}
void initBlocks() {
    blocks.clear();
    float totalGridWidth = 2*gameBoundX;
    float totalGapWidth = (BRICKS_PER_ROW - 1) * BRICK_GAP;
    float brickWidth = (totalGridWidth - totalGapWidth) / BRICKS_PER_ROW;
    float startX = -gameBoundX;

    // Positions fixes pour les briques bonus et compteur dans chaque rangée
    std::srand(42);  // Seed fixe pour la reproductibilité
    int bonusPositions[BRICK_ROWS];
    int counterPositions[BRICK_ROWS];

    for (int i = 0; i < BRICK_ROWS; i++) {
        bonusPositions[i] = 1 + std::rand() % (BRICKS_PER_ROW - 2);
        do {
            counterPositions[i] = 1 + std::rand() % (BRICKS_PER_ROW - 2);
        } while (counterPositions[i] == bonusPositions[i]);
    }

    for (int i = 0; i < BRICK_ROWS; ++i) {
        // Définir la couleur et les points une seule fois par ligne
        // Pour obtenir la couleur de base selon la ligne
        BrickColor baseColorType;
        int points;
        if (i < 2) {
            baseColorType = BrickColor::RED;
            points = 7;
        } else if (i < 4) {
            baseColorType = BrickColor::ORANGE;
            points = 5;
        } else if (i < 6) {
            baseColorType = BrickColor::GREEN;
            points = 3;
        } else {
            baseColorType = BrickColor::YELLOW;
            points = 1;
        }

        for (int j = 0; j < BRICKS_PER_ROW; ++j) {
            Block block;
            block.size = Vec2{brickWidth, BRICK_HEIGHT};
            block.position = Vec2{startX + j * (brickWidth + BRICK_GAP),
                                  BRICK_START_Y - i * (BRICK_HEIGHT + BRICK_GAP)};
            block.active = true;
            block.hitCounter = 1;
            block.points = points;
            // Par défaut, utiliser la couleur de base de la ligne
            block.color = getColorFromEnum(baseColorType);
            block.colorType=baseColorType;
            // Cas spéciaux par type de brique
            if (i == 0 && (j == 0 || j == BRICKS_PER_ROW-1)) {
                // Murs indestructibles
                block.isWall = true;
                block.isReflective = false;
                block.color = getColorFromEnum(BrickColor::GRAY);
                block.colorType=BrickColor::GRAY;
                block.hitCounter = -1;
            } else if (i == 0 && (j == 1 || j == BRICKS_PER_ROW-2)) {
                // Murs réfléchissants
                block.isWall = true;
                block.isReflective = true;
                block.color = getColorFromEnum(BrickColor::WHITE);
                block.colorType=BrickColor::WHITE;
                block.hitCounter = -1;
            } else if (j == counterPositions[i]) {
                // Briques à compteur - version plus sombre de la couleur de base
                block.hitCounter = 2;
                block.color = getColorFromEnum(baseColorType, true);
            } else if (j == bonusPositions[i]) {
                // Briques bonus
                block.isBonus = true;
                block.bonusType = rand() % 7;
            }
            blocks.push_back(block);
        }
    }
    // Réinitialisation du générateur aléatoire pour le reste du jeu
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}
    void updateBlockPositions()
    {
        // Calculer la nouvelle taille des briques basée sur les limites du jeu actuelles
        float totalGridWidth = 2*gameBoundX;
        float totalGapWidth = (BRICKS_PER_ROW - 1) * BRICK_GAP;
        float brickWidth = (totalGridWidth - totalGapWidth) / BRICKS_PER_ROW;
        float startX = -gameBoundX;

        // Parcourir toutes les briques existantes et ajuster leurs positions et tailles
        int i = 0;
        for (int row = 0; row < BRICK_ROWS; ++row) {
            for (int col = 0; col < BRICKS_PER_ROW; ++col) {
                if (i < blocks.size()) {
                    // Conserver l'état actif/inactif et autres propriétés
                    Block& block = blocks[i];

                    // Mettre à jour uniquement la position et la taille
                    block.size.x = brickWidth;
                    block.size.y = BRICK_HEIGHT;
                    block.position.x = startX + col * (brickWidth + BRICK_GAP);
                    block.position.y = BRICK_START_Y - row * (BRICK_HEIGHT + BRICK_GAP);

                    i++;
                }
            }
        }
    }
    bool checkBonusPaddleCollision(const FallingBonus &bonus) const {
        return bonus.position.x < playerPaddle.position.x + playerPaddle.size.x &&
               bonus.position.x + bonus.size.x > playerPaddle.position.x &&
               bonus.position.y < playerPaddle.position.y + playerPaddle.size.y &&
               bonus.position.y + bonus.size.y > playerPaddle.position.y;
    }

    // Reset paddle and ball to starting positions
    void resetPlayerAndBall()
    {
        paddleShrunk? playerPaddle.size = Vec2{PADDLE_WIDTH*0.5, PADDLE_HEIGHT} : playerPaddle.size = Vec2{PADDLE_WIDTH, PADDLE_HEIGHT};
        playerPaddle.position = Vec2{0.0f - PADDLE_WIDTH / 2.0f, PADDLE_Y_POSITION};
        playerPaddle.color = Color{0.8f, 0.8f, 0.8f, 1.0f};

        gameBall.size = Vec2{BALL_RADIUS * 2.0f, BALL_RADIUS * 2.0f};
        gameBall.position = Vec2{playerPaddle.position.x + playerPaddle.size.x / 2.0f - BALL_RADIUS,
                                 playerPaddle.position.y + playerPaddle.size.y};
        gameBall.color = Color{1.0f, 1.0f, 1.0f, 1.0f};
        gameBall.velocity = Vec2{0.0f, 0.0f};
        gameBall.speedMagnitude = INITIAL_BALL_SPEED;
        gameBall.stuckToPaddle = true;
        gameBall.hitCount = 0; // Reset hits
    }

    // --- Game Loop Functions ---

    void processInput(float dt)
    {
        // Check ImGui IO to see if UI wants mouse/keyboard, if needed for complex interaction
        // ImGuiIO& io = ImGui::GetIO();
        // if (io.WantCaptureKeyboard) return; // Example: Skip game input if typing in ImGui window

        // --- Game Input (only when playing) ---
        if (currentState == GameState::PLAYING)
        {
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            {
                playerPaddle.position.x -= PADDLE_SPEED * dt;
                playerPaddle.position.x = std::max(playerPaddle.position.x, -gameBoundX); // Clamp left
            }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            {
                playerPaddle.position.x += PADDLE_SPEED * dt;
                // Clamp right
                playerPaddle.position.x = std::min(playerPaddle.position.x, gameBoundX - playerPaddle.size.x);
            }
            // Launch Ball
            if (gameBall.stuckToPaddle && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            {
                gameBall.stuckToPaddle = false;
                const float direction = rand() % 2*2-1;
                const float velocityX = direction * gameBall.speedMagnitude;
                const float velocityY = gameBall.speedMagnitude;

                gameBall.velocity = Vec2{velocityX, velocityY};
            }
        }
        // --- Game Over Input ---
        else if (currentState == GameState::GAME_OVER)
        {
            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
            {
                currentState = GameState::MENU; // Return to menu
                                                // No need to call initMenu - renderMenu handles drawing
            }
        }

        // --- Global Input ---
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }
    }

    void update(float dt)
{
    // Only update game logic if playing
    if (currentState == GameState::PLAYING) {
        // --- Update Ball Position ---
        if (gameBall.stuckToPaddle)
        {
            gameBall.position = Vec2{playerPaddle.position.x + playerPaddle.size.x / 2.0f - BALL_RADIUS,
                                     playerPaddle.position.y + playerPaddle.size.y};
        }
        else
        {
            gameBall.position.x += gameBall.velocity.x * dt;
            gameBall.position.y += gameBall.velocity.y * dt;

            // --- Handle Collisions ---
            handleCollisions();

            // --- Check Lose Condition ---
            if (gameBall.position.y + gameBall.size.y < -gameBoundY)
            { // Ball below bottom edge
                lives--;
                if (lives <= 0)
                {
                    currentState = GameState::GAME_OVER;
                }
                else
                {
                    resetPlayerAndBall(); // Reset ball/paddle for next life
                }
            }
        }

        // Mettre à jour les bonus qui tombent
        for (auto &bonus : fallingBonuses) {
            if (bonus.active) {
                // Faire descendre le bonus
                bonus.position.y -= bonus.fallSpeed * dt;

                // Vérifier si le bonus a atteint le bas de l'écran
                if (bonus.position.y < -gameBoundY) {
                    bonus.active = false;
                    continue;
                }

                // Vérifier la collision avec la raquette
                if (checkBonusPaddleCollision(bonus)) {
                    applyBonus(bonus);
                    bonus.active = false;
                }
            }
        }

        // Nettoyage des bonus inactifs
        fallingBonuses.erase(
            std::remove_if(fallingBonuses.begin(), fallingBonuses.end(),
                [](const FallingBonus &b) { return !b.active; }),
            fallingBonuses.end()
        );

        // --- Check Win Condition ---
        bool allBlocksInactive = true;
        for (const auto &block : blocks)
        {
            if (block.active && !block.isWall)
            {
                allBlocksInactive = false;
                break;
            }
        }
        if (allBlocksInactive)
        {
            if (lives > 0)  // S'il reste des vies, passer au niveau suivant
            {
                currentLevel++;
                firstContactOrange=true;
                firstContactRed=true;
                gameBall.hitCount=0;
                initBlocks();  // Générer un nouveau niveau de briques
                resetPlayerAndBall(); // Réinitialiser la position de la balle et de la raquette
                // La score est préservé car nous ne le réinitialisons pas
            }
            else
            {
                currentState = GameState::GAME_OVER;
            }
        }
    }
}

    // --- Rendering Functions ---

    void render()
    {
        // --- Clear Screen ---
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f); // Dark background
        glClear(GL_COLOR_BUFFER_BIT);

        // --- Render Game World Elements (if applicable) ---
        if (currentState == GameState::PLAYING || currentState == GameState::GAME_OVER)
        {
            // Bricks
            for (const auto &block : blocks)
            {
                if (block.active) {
                    renderGameObject(block);
                }
            }


            // Bonus en train de tomber
            for (const auto &bonus : fallingBonuses) {
                if (bonus.active) {
                    renderFallingBonus(bonus);
                }
            }

            // Paddle
            renderGameObject(playerPaddle);
            // Ball (render if playing, or if game over but ball wasn't stuck/lost yet)
            if (currentState == GameState::PLAYING || (lives > 0 && !gameBall.stuckToPaddle))
            {
                renderGameObject(gameBall);
            }
        }

        // --- Render UI using Dear ImGui ---
        renderUI(); // Separate function for ImGui elements

        // --- Finalize ImGui Frame and Render it ---
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        // --- Swap Buffers ---
        glfwSwapBuffers(window);
    }

    // Renders all ImGui elements based on current state
    void renderUI()
    {
        int currentWindowWidth, currentWindowHeight;
        glfwGetWindowSize(window, &currentWindowWidth, &currentWindowHeight);
        if (currentState == GameState::MENU)
        {
            renderMenuUI(currentWindowWidth, currentWindowHeight);
        }
        else if (currentState == GameState::PLAYING || currentState == GameState::GAME_OVER)
        {
            renderGameUI(currentWindowWidth,currentWindowHeight); // Score, Lives
            if (currentState == GameState::GAME_OVER)
            {
                renderGameOverUI(currentWindowWidth,currentWindowHeight); // Game Over message
            }
        }
    }

    // Renders the main menu using ImGui widgets
    void renderMenuUI(const int& wW ,const int& wH)
    {
        // Create a fullscreen, transparent window for button layout
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(wW, wH));
        ImGui::Begin("MainMenu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);

        // --- Title ---
        const char *title = "BREAKOUT";
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImVec2 titleSize = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX((wW - titleSize.x) / 2.0f);
        ImGui::SetCursorPosY(wH * 0.3f);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", title);
        ImGui::PopFont();

        // --- Buttons ---
        constexpr float buttonWidth = 200;
        constexpr float buttonHeight = 50;
        const float buttonPosX = (wW - buttonWidth) / 2.0f;
        const float buttonPosY_Play = wH / 2.0f - buttonHeight;
        const float buttonPosY_Exit = wH / 2.0f + 10;

        // Play Button
        ImGui::SetCursorPos(ImVec2(buttonPosX, buttonPosY_Play));
        if (ImGui::Button("PLAY", ImVec2(buttonWidth, buttonHeight)))
        {
            currentState = GameState::PLAYING;
            initGame();
        }

        // Exit Button
        ImGui::SetCursorPos(ImVec2(buttonPosX, buttonPosY_Exit));
        if (ImGui::Button("EXIT", ImVec2(buttonWidth, buttonHeight)))
        {
            glfwSetWindowShouldClose(window, true);
        }

        ImGui::End();
    }

    void renderGameUI(const float& wW, const float& wH) const
    {
        ImDrawList *drawList = ImGui::GetForegroundDrawList(); // Draw on top of game

        // Score Display (Top-Left)
        std::string scoreText = "SCORE: " + std::to_string(score);
        drawList->AddText(ImVec2(15.0f, 10.0f), IM_COL32(255, 255, 255, 255), scoreText.c_str());

        // Level Display (Top-Middle)
        std::string levelText = "LEVEL: " + std::to_string(currentLevel);
        ImVec2 levelTextSize = ImGui::CalcTextSize(levelText.c_str());
        drawList->AddText(ImVec2((wW - levelTextSize.x) / 2.0f, 10.0f), IM_COL32(255, 255, 255, 255), levelText.c_str());

        // Lives Display (Top-Right)
        std::string livesText = "LIVES: " + std::to_string(lives);
        ImVec2 livesTextSize = ImGui::CalcTextSize(livesText.c_str());
        drawList->AddText(ImVec2(wW - livesTextSize.x - 15.0f / 2, 10.0f), IM_COL32(255, 255, 255, 255), livesText.c_str());
    }

    static void renderGameOverUI(const float& wW,const float& wH)
    {
        ImDrawList *drawList = ImGui::GetForegroundDrawList();

        // --- "GAME OVER" Text ---
        const char *gameOverMsg = "GAME OVER";
        float goFontSize = ImGui::GetFontSize() * 2.0f; // Scale default font size
        ImVec2 goTextSize = ImGui::CalcTextSize(gameOverMsg);
        // Calculate centered position using scaled size
        ImVec2 goTextPos = ImVec2((wW - goTextSize.x * 2.0f) / 2.0f, wH * 0.4f);
        drawList->AddText(nullptr, goFontSize, goTextPos, IM_COL32(255, 50, 50, 255), gameOverMsg);

        // --- "Press Enter" Text ---
        const char *restartMsg = "Press ENTER to Return to Menu";
        ImVec2 restartTextSize = ImGui::CalcTextSize(restartMsg);
        ImVec2 restartTextPos = ImVec2((wW - restartTextSize.x) / 2.0f, wH * 0.6f);
        drawList->AddText(restartTextPos, IM_COL32(255, 255, 255, 255), restartMsg);
    }
    static void renderFallingBonus(const FallingBonus &bonus)
{
    glColor4f(bonus.color.r, bonus.color.g, bonus.color.b, bonus.color.a);
    glBegin(GL_QUADS);
    glVertex2f(bonus.position.x, bonus.position.y);
    glVertex2f(bonus.position.x + bonus.size.x, bonus.position.y);
    glVertex2f(bonus.position.x + bonus.size.x, bonus.position.y + bonus.size.y);
    glVertex2f(bonus.position.x, bonus.position.y + bonus.size.y);
    glEnd();

}
    static void renderGameObject(const GameObject &obj)
    {
        glColor4f(obj.color.r, obj.color.g, obj.color.b, obj.color.a);
        glBegin(GL_QUADS);
        glVertex2f(obj.position.x, obj.position.y);
        glVertex2f(obj.position.x + obj.size.x, obj.position.y);
        glVertex2f(obj.position.x + obj.size.x, obj.position.y + obj.size.y);
        glVertex2f(obj.position.x, obj.position.y + obj.size.y);
        glEnd();
    }

    static bool checkCollision(const GameObject &one, const GameObject &two) { return one.position.x < two.position.x + two.size.x && one.position.x + one.size.x > two.position.x && one.position.y < two.position.y + two.size.y && one.position.y + one.size.y > two.position.y; }
    void handleCollisions()
    {
        handleBallWallCollision();
        if (checkCollision(gameBall, playerPaddle))
        {
            resolveBallPaddleCollision();
        }
        for (auto &block : blocks)
        {
            if (block.active && checkCollision(gameBall, block))
            {
                resolveBallBlockCollision(block);
                break;
            }
        }
    }
    void handleBallWallCollision()
    {
        if (gameBall.position.x <= -gameBoundX)
        {
            gameBall.velocity.x = std::abs(gameBall.velocity.x);
            gameBall.position.x = -gameBoundX;
        }
        else if (gameBall.position.x + gameBall.size.x >= gameBoundX)
        {
            gameBall.velocity.x = -std::abs(gameBall.velocity.x);
            gameBall.position.x = gameBoundX - gameBall.size.x;
        }
        if (gameBall.position.y + gameBall.size.y >= gameBoundY)
        {
            //Collision avec le plafond
            if (!paddleShrunk) {
                paddleShrunk = true;
                playerPaddle.size.x *= 0.5f;
            }
            gameBall.velocity.y = -std::abs(gameBall.velocity.y);
            gameBall.position.y = gameBoundY - gameBall.size.y;
        }
    }

    void resolveBallPaddleCollision()
    {
        if (gameBall.velocity.y >= 0.0f)
            return;

        // Repositionnement au-dessus de la raquette
        gameBall.position.y = playerPaddle.position.y + playerPaddle.size.y;

        // Calcul de l'impact normalisé (-1 = bord gauche, +1 = bord droit)
        float ballCenterX = gameBall.position.x + gameBall.size.x * 0.5f;
        float paddleCenterX = playerPaddle.position.x + playerPaddle.size.x * 0.5f;
        float offset = (ballCenterX - paddleCenterX) / (playerPaddle.size.x * 0.5f);
        float normalizedOffset = std::max(-1.0f, std::min(offset, 1.0f));

        // Inversion de la composante verticale
        gameBall.velocity.y = std::abs(gameBall.velocity.y);

        // Définir les seuils pour les quarts de la raquette
        const float quarterThreshold = 0.5f;

        if (normalizedOffset <= -quarterThreshold) {
            // Quart gauche : peu de déviation horizontale
            gameBall.velocity.x = normalizedOffset * gameBall.speedMagnitude * 0.2f;

            // Recalculer la composante verticale pour maintenir la magnitude
            float vy = std::sqrt(
                gameBall.speedMagnitude * gameBall.speedMagnitude
                - gameBall.velocity.x * gameBall.velocity.x
            );
            gameBall.velocity.y = vy;
        }
        else if (normalizedOffset >= quarterThreshold) {
            // Quart droit : forte déviation horizontale
            gameBall.velocity.x = normalizedOffset * gameBall.speedMagnitude * 0.8f;

            // Recalculer la composante verticale pour maintenir la magnitude
            float vy = std::sqrt(
                gameBall.speedMagnitude * gameBall.speedMagnitude
                - gameBall.velocity.x * gameBall.velocity.x
            );
            gameBall.velocity.y = vy;
        }
    }
   void resolveBallBlockCollision(Block &block) {
        if (block.isWall) {
            if (block.isReflective) {
                // Mur à rétroréflexion - inversion de la direction
                gameBall.velocity.x = -gameBall.velocity.x;
                gameBall.velocity.y = -gameBall.velocity.y;
            } else {
                // Mur normal - rebond standard
                // Déterminer où la balle a frappé la brique
                float ballCenterX = gameBall.position.x + gameBall.size.x / 2.0f;
                float ballCenterY = gameBall.position.y + gameBall.size.y / 2.0f;
                float blockCenterX = block.position.x + block.size.x / 2.0f;
                float blockCenterY = block.position.y + block.size.y / 2.0f;

                // Calculer les distances relatives
                float diffX = ballCenterX - blockCenterX;
                float diffY = ballCenterY - blockCenterY;

                // Déterminer si la collision est horizontale ou verticale
                if (std::abs(diffX / block.size.x) > std::abs(diffY / block.size.y)) {
                    // Collision horizontale
                    gameBall.velocity.x = -gameBall.velocity.x;
                } else {
                    // Collision verticale
                    gameBall.velocity.y = -gameBall.velocity.y;
                }
            }
            return;
        }

        // Appliquer le rebond d'abord
        // Déterminer où la balle a frappé la brique
        float ballCenterX = gameBall.position.x + gameBall.size.x / 2.0f;
        float ballCenterY = gameBall.position.y + gameBall.size.y / 2.0f;
        float blockCenterX = block.position.x + block.size.x / 2.0f;
        float blockCenterY = block.position.y + block.size.y / 2.0f;

        // Calculer les distances relatives
        float diffX = ballCenterX - blockCenterX;
        float diffY = ballCenterY - blockCenterY;

        // Déterminer si la collision est horizontale ou verticale
        if (std::abs(diffX / block.size.x) > std::abs(diffY / block.size.y)) {
            // Collision horizontale
            gameBall.velocity.x = -gameBall.velocity.x;
        } else {
            // Collision verticale
            gameBall.velocity.y = -gameBall.velocity.y;
        }

        // Ajuster légèrement la position pour éviter une nouvelle collision immédiate
        float signY = (gameBall.velocity.y > 0) ? 1.0f : -1.0f;
        gameBall.position.y += signY * 0.001f;

        // Ensuite, diminuer le compteur de coups
        block.hitCounter--;

        // Si le compteur atteint 0, désactiver la brique
        if (block.hitCounter <= 0) {
            block.active = false;
            score += block.points;

            // Logique pour les briques bonus
            if (block.isBonus) {
                spawnBonus(block);
            }
        } else {
            block.color = getColorFromEnum(block.colorType);
        }

        // Incrémenter le compteur de coups et appliquer l'augmentation de vitesse
        gameBall.hitCount++;
        applySpeedIncrease(block);
    }

    void applySpeedIncrease(const Block &b)
    {
        bool speedIncreased = false;
        if (gameBall.hitCount == 4 || gameBall.hitCount == 12 )
        {
            gameBall.speedMagnitude *= BALL_SPEED_INCREMENT;
            speedIncreased = true;
        }

        if (firstContactRed && b.color.r==1.0f && b.color.g== 0.6f && b.color.b== 0.2f) {
            gameBall.speedMagnitude *= BALL_SPEED_INCREMENT;
            firstContactRed = false;
            speedIncreased = true;
        }

        if (firstContactOrange && b.color.r == 1.0f && b.color.g == 0.2f && b.color.b == 0.2f) {
            gameBall.speedMagnitude *= BALL_SPEED_INCREMENT;
            firstContactOrange = false;
            speedIncreased = true;
        }
        if (speedIncreased)
        {
            normalizeVelocity();
        }
    }
    void normalizeVelocity()
    {
        float currentSpeed = std::sqrt(gameBall.velocity.x * gameBall.velocity.x + gameBall.velocity.y * gameBall.velocity.y);
        if (currentSpeed > 0.0001f)
        {
            gameBall.velocity.x = (gameBall.velocity.x / currentSpeed) * gameBall.speedMagnitude;
            gameBall.velocity.y = (gameBall.velocity.y / currentSpeed) * gameBall.speedMagnitude;
        }
        else if (!gameBall.stuckToPaddle)
        {
            gameBall.velocity = Vec2{0.0f, gameBall.speedMagnitude};
        }
    }

    // --- GLFW Callbacks ---

    // Handles window resize events - updates viewport and projection matrix
    static void framebufferSizeCallback(GLFWwindow *window, int width, int height)
    {
        auto gameInstance = static_cast<Game *>(glfwGetWindowUserPointer(window));
        if (gameInstance)
        {
            gameInstance->updateProjectionMatrix(width, height);
        }
    }

    // Updates viewport and projection matrix based on window size
    void updateProjectionMatrix(int width, int height)
{
    if (height == 0)
        height = 1; // Prevent division by zero
    windowWidth = width;
    windowHeight = height;

    float oldBoundX = gameBoundX;
    float oldBoundY = gameBoundY;

    // Update orthographic projection based on aspect ratio
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspect = static_cast<float>(width) / static_cast<float>(height);

    if (width >= height) { // Wider than tall
        gameBoundX = aspect;
        gameBoundY = 1.0f;
        glOrtho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    }
    else { // Taller than wide
        gameBoundX = 1.0f;
        gameBoundY = 1.0f / aspect;
        glOrtho(-1.0f, 1.0f, -1.0f / aspect, 1.0f / aspect, -1.0f, 1.0f);
    }

    // Ajuster la vitesse en fonction du changement des dimensions du monde
    if (currentState == GameState::PLAYING && !gameBall.stuckToPaddle) {
        // Calculer le facteur d'échelle pour la vitesse
        float speedScaleFactor = (gameBoundX / oldBoundX + gameBoundY / oldBoundY) / 2.0f;

        // Appliquer ce facteur à la vitesse actuelle de la balle
        float currentSpeed = std::sqrt(gameBall.velocity.x * gameBall.velocity.x +
                                      gameBall.velocity.y * gameBall.velocity.y);

        if (currentSpeed > 0.0001f) {
            // Maintenir la direction mais ajuster la magnitude
            gameBall.velocity.x *= speedScaleFactor;
            gameBall.velocity.y *= speedScaleFactor;

            // Mettre à jour speedMagnitude pour les futurs calculs
            gameBall.speedMagnitude *= speedScaleFactor;
        }
    }

    // Reset model-view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Réinitialiser les blocs et autres éléments si nécessaire
    if (currentState == GameState::PLAYING && !blocks.empty())
        updateBlockPositions();
}
};

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
int main()
{
    try
    {
        Game breakoutGame(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
        breakoutGame.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        // Ensure termination even if constructor fails partially
        glfwTerminate();
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "An unknown error occurred." << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
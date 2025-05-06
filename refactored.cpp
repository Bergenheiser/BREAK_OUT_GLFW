//
// Created by Dimitri Copley on 06/05/2025.
//
/**
 * @file breakout.cpp
 * @brief Implémentation d'un jeu Breakout en C++ avec GLFW et Dear ImGui.
 *
 * Ce fichier contient l'intégralité du code source pour un jeu Breakout,
 * incluant la gestion de la fenêtre, le rendu graphique, la logique du jeu,
 * la gestion des collisions, les états du jeu (menu, jeu, game over),
 * et un système de bonus.
 */

// Pour la compatibilité macOS si nécessaire (désactive les avertissements de dépréciation OpenGL)
#define GL_SILENCE_DEPRECATION

// --- Inclusions des bibliothèques ---
#include <GLFW/glfw3.h>                 // Pour la gestion de la fenêtre et des entrées OpenGL
#include <vector>                       // Pour std::vector (conteneur dynamique)
#include <cmath>                        // Pour les fonctions mathématiques (sqrt, abs, etc.)
#include <iostream>                     // Pour les entrées/sorties console (std::cerr, std::cout)
#include <cstdlib>                      // Pour std::rand, std::srand, EXIT_SUCCESS, EXIT_FAILURE
#include <ctime>                        // Pour std::time (initialisation du générateur aléatoire)
#include <stdexcept>                    // Pour std::runtime_error
#include <string>                       // Pour std::string
#include <algorithm>                    // Pour std::min, std::max, std::remove_if

// --- Inclusions des en-têtes Dear ImGui ---
#include "imgui/imgui.h"                // En-tête principal de Dear ImGui
#include "imgui/backends/imgui_impl_glfw.h" // Backend Dear ImGui pour GLFW
#include "imgui/backends/imgui_impl_opengl2.h"// Backend Dear ImGui pour OpenGL 2

// === Commandes de Compilation (Exemples) ===
// MACOSX:
// g++ -std=c++11 breakout.cpp imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl2.cpp -o breakout -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
//
// LINUX:
// g++ -std=c++11 breakout.cpp imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl2.cpp -o breakout -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm
// (Assurez-vous que les paquets -dev nécessaires comme libglfw3-dev, libgl1-mesa-dev, xorg-dev sont installés)

//-----------------------------------------------------------------------------
// Constantes Globales
//-----------------------------------------------------------------------------

// --- Constantes de la Fenêtre ---
constexpr int WINDOW_WIDTH = 960;       ///< Largeur initiale de la fenêtre en pixels.
constexpr int WINDOW_HEIGHT = 540;      ///< Hauteur initiale de la fenêtre en pixels.
constexpr char* WINDOW_TITLE = "Breakout C++"; ///< Titre de la fenêtre du jeu.

// --- Constantes du Jeu ---
constexpr int BRICK_ROWS = 8;           ///< Nombre de rangées de briques.
constexpr int BRICKS_PER_ROW = 14;      ///< Nombre de briques par rangée.
constexpr float BRICK_GRID_WIDTH = 1.85f;///< Largeur de la grille de briques (coordonnées normalisées, ajusté pour marges).
constexpr float BRICK_START_Y = 0.85f;  ///< Position Y de départ de la première rangée de briques (coordonnées normalisées).
constexpr float BRICK_HEIGHT = 0.06f;   ///< Hauteur d'une brique (coordonnées normalisées).
constexpr float BRICK_GAP = 0.01f;      ///< Espace entre les briques (coordonnées normalisées).

// --- Constantes de la Raquette (Paddle) ---
constexpr float PADDLE_WIDTH = 0.25f;   ///< Largeur de la raquette (coordonnées normalisées).
constexpr float PADDLE_HEIGHT = 0.04f;  ///< Hauteur de la raquette (coordonnées normalisées).
constexpr float PADDLE_Y_POSITION = -0.9f; ///< Position Y de la raquette (coordonnées normalisées).
constexpr float PADDLE_SPEED = 1.5f;    ///< Vitesse de déplacement de la raquette (unités normalisées par seconde).

// --- Constantes de la Balle ---
constexpr float BALL_RADIUS = 0.02f;    ///< Rayon de la balle (coordonnées normalisées).
constexpr float INITIAL_BALL_SPEED = 1.0f; ///< Vitesse initiale de la balle (unités normalisées par seconde).
constexpr float BALL_SPEED_INCREMENT = 1.19f; ///< Facteur d'augmentation de la vitesse de la balle.

//-----------------------------------------------------------------------------
// Structures Utilitaires
//-----------------------------------------------------------------------------

/**
 * @struct Vec2
 * @brief Structure simple pour représenter un vecteur 2D (position, taille, vitesse).
 */
struct Vec2 {
    Vec2() = default;
    Vec2(float x_val, float y_val) : x(x_val), y(y_val) {};
    float x = 0.0f; ///< Composante x du vecteur.
    float y = 0.0f; ///< Composante y du vecteur.
};

/**
 * @struct Color
 * @brief Structure pour représenter une couleur RGBA.
 */
struct Color {
    Color(float r_val = 1.0f, float g_val = 1.0f, float b_val = 1.0f, float a_val = 1.0f)
        : r(r_val), g(g_val), b(b_val), a(a_val) {}
    float r = 1.0f; ///< Composante rouge (0.0 à 1.0).
    float g = 1.0f; ///< Composante verte (0.0 à 1.0).
    float b = 1.0f; ///< Composante bleue (0.0 à 1.0).
    float a = 1.0f; ///< Composante alpha (transparence, 0.0 à 1.0).
};

//-----------------------------------------------------------------------------
// Énumérations du Jeu
//-----------------------------------------------------------------------------

/**
 * @enum BrickColor
 * @brief Énumération des types de couleurs utilisés dans le jeu.
 * Utilisé pour déterminer la couleur des briques, de la raquette, et de la balle.
 */
enum class BrickColor {
    RED,           ///< Couleur rouge (briques supérieures).
    ORANGE,        ///< Couleur orange (briques intermédiaires supérieures).
    GREEN,         ///< Couleur verte (briques intermédiaires inférieures).
    YELLOW,        ///< Couleur jaune (briques inférieures).
    GRAY,          ///< Couleur grise (murs indestructibles).
    WHITE,         ///< Couleur blanche (murs réfléchissants).
    PADDLE,        ///< Couleur de la raquette.
    BALL           ///< Couleur de la balle.
};

/**
 * @enum GameState
 * @brief Énumération des différents états possibles du jeu.
 */
enum class GameState {
    MENU,          ///< Le jeu est dans le menu principal.
    PLAYING,       ///< Le jeu est en cours.
    GAME_OVER      ///< Le jeu est terminé (partie perdue).
};

/**
 * @enum BonusType
 * @brief Énumération des types de bonus pouvant être obtenus.
 */
enum BonusType {
    LIFE_ADD = 0,        ///< Ajoute une vie.
    LIFE_REMOVE = 1,     ///< Retire une vie.
    PADDLE_WIDEN = 2,    ///< Agrandit la raquette.
    PADDLE_SHRINK = 3,   ///< Rétrécit la raquette.
    BALL_SLOW = 4,       ///< Ralentit la balle.
    BALL_FAST = 5,       ///< Accélère la balle.
    BALL_STRAIGHTEN = 6, ///< Redresse la trajectoire de la balle.
    BALL_ANGLE = 7       ///< Incline davantage la trajectoire de la balle.
};

//-----------------------------------------------------------------------------
// Structures des Objets du Jeu
//-----------------------------------------------------------------------------

/**
 * @struct GameObject
 * @brief Structure de base pour tous les objets du jeu ayant une position et une taille.
 */
struct GameObject {
    Vec2 position;      ///< Position du coin inférieur gauche de l'objet.
    Vec2 size;          ///< Taille (largeur, hauteur) de l'objet.
    Color color;        ///< Couleur de l'objet.
    BrickColor colorType; ///< Type de couleur (utilisé pour la logique de jeu, ex: points).
};

/**
 * @struct Paddle
 * @brief Structure représentant la raquette contrôlée par le joueur. Hérite de GameObject.
 */
struct Paddle : public GameObject {
    // Pas de membres spécifiques supplémentaires pour l'instant.
};

/**
 * @struct Ball
 * @brief Structure représentant la balle. Hérite de GameObject.
 */
struct Ball : public GameObject {
    Vec2 velocity = Vec2{0.0f, 0.0f}; ///< Vitesse actuelle de la balle (direction et magnitude).
    float speedMagnitude = INITIAL_BALL_SPEED; ///< Magnitude de la vitesse de la balle.
    bool stuckToPaddle = true;      ///< Vrai si la balle est collée à la raquette avant le lancement.
    int hitCount = 0;               ///< Compteur de coups de la balle (pour augmenter la vitesse).
};

/**
 * @struct Block
 * @brief Structure représentant une brique. Hérite de GameObject.
 */
struct Block : public GameObject {
    bool active = true;         ///< Vrai si la brique est visible et peut être touchée.
    int points = 0;             ///< Nombre de points attribués lors de la destruction.
    int hitCounter = 1;         ///< Nombre de coups nécessaires pour détruire la brique (1 par défaut).
    bool isWall = false;        ///< Vrai si la brique est un mur indestructible ou réfléchissant.
    bool isReflective = false;  ///< Vrai si la brique est un mur qui rétroréflechit la balle.
    bool isBonus = false;       ///< Vrai si la brique lâche un bonus lors de sa destruction.
    int bonusType = 0;          ///< Type de bonus lâché (cf. BonusType).
};

/**
 * @struct FallingBonus
 * @brief Structure représentant un bonus qui tombe après la destruction d'une brique bonus.
 */
struct FallingBonus {
    Vec2 position;      ///< Position actuelle du bonus.
    Vec2 size;          ///< Taille du bonus.
    Color color;        ///< Couleur visuelle du bonus.
    int type = 0;       ///< Type de bonus (cf. BonusType).
    float fallSpeed = 0.0f; ///< Vitesse de chute du bonus.
    bool active = true; ///< Vrai si le bonus est actif et visible.
};

//-----------------------------------------------------------------------------
// Fonctions Utilitaires (Couleur)
//-----------------------------------------------------------------------------

/**
 * @brief Obtient une structure Color à partir d'une énumération BrickColor.
 * @param colorType Le type de couleur de l'énumération BrickColor.
 * @param isDarker Si vrai, retourne une version plus sombre de la couleur (utilisé pour les briques à compteur).
 * @param alpha Valeur alpha pour la transparence (1.0f par défaut pour opaque).
 * @return La structure Color correspondante.
 */
Color getColorFromEnum(BrickColor colorType, bool isDarker = false, float alpha = 1.0f) {
    Color result;
    switch (colorType) {
        case BrickColor::RED:    result = Color{1.0f, 0.2f, 0.2f, alpha}; break;
        case BrickColor::ORANGE: result = Color{1.0f, 0.6f, 0.2f, alpha}; break;
        case BrickColor::GREEN:  result = Color{0.2f, 1.0f, 0.2f, alpha}; break;
        case BrickColor::YELLOW: result = Color{1.0f, 1.0f, 0.2f, alpha}; break;
        case BrickColor::GRAY:   result = Color{0.5f, 0.5f, 0.5f, alpha}; break;
        case BrickColor::WHITE:  result = Color{1.0f, 1.0f, 1.0f, alpha}; break;
        case BrickColor::PADDLE: result = Color{0.8f, 0.8f, 0.8f, alpha}; break;
        case BrickColor::BALL:   result = Color{1.0f, 1.0f, 1.0f, alpha}; break;
    }
    if (isDarker) {
        result.r *= 0.7f;
        result.g *= 0.7f;
        result.b *= 0.7f;
    }
    return result;
}

//-----------------------------------------------------------------------------
// Classe Principale du Jeu
//-----------------------------------------------------------------------------
/**
 * @class Game
 * @brief Gère l'ensemble de la logique du jeu Breakout, y compris l'initialisation,
 *        la boucle principale, le rendu, la gestion des entrées et des états.
 */
class Game {
public:
    /**
     * @brief Constructeur de la classe Game.
     * Initialise GLFW, la fenêtre, Dear ImGui et les états initiaux du jeu.
     * @param width Largeur de la fenêtre.
     * @param height Hauteur de la fenêtre.
     * @param title Titre de la fenêtre.
     * @throw std::runtime_error en cas d'échec d'initialisation.
     */
    Game(int width, int height, const char* title)
        : windowWidth(width), windowHeight(height) {
        if (!initGLFW(width, height, title)) {
            throw std::runtime_error("Échec de l'initialisation de GLFW ou de la création de la fenêtre");
        }

        // Initialisation de Dear ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io; // io peut être utilisé pour configurer ImGui
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Optionnel: active la navigation clavier
        ImGui::StyleColorsDark(); // Style sombre par défaut

        // Initialisation des backends ImGui
        ImGui_ImplGlfw_InitForOpenGL(window, true); // Installe les callbacks pour GLFW
        ImGui_ImplOpenGL2_Init();                   // Initialise pour OpenGL 2

        // Initialisation du Jeu
        std::srand(static_cast<unsigned int>(std::time(nullptr))); // Initialise le générateur de nombres aléatoires
        glfwSetWindowUserPointer(window, this); // Lie la fenêtre GLFW à cette instance de Game
        setupCallbacks();                       // Configure les callbacks GLFW non gérés par ImGui
        updateProjectionMatrix(width, height);  // Configuration initiale de la matrice de projection

        currentState = GameState::MENU; // Commence dans l'état du menu
    }

    /**
     * @brief Destructeur de la classe Game.
     * Libère les ressources de Dear ImGui et GLFW.
     */
    ~Game() {
        // Arrêt de Dear ImGui
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // Arrêt de GLFW
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }

    /**
     * @brief Lance la boucle principale du jeu.
     * La boucle continue tant que la fenêtre ne doit pas être fermée.
     */
    void run() {
        lastTime = glfwGetTime();
        while (!glfwWindowShouldClose(window)) {
            // Gestion du temps (deltaTime)
            double currentTime = glfwGetTime();
            auto deltaTime = static_cast<float>(currentTime - lastTime);
            lastTime = currentTime;

            // Démarrage de la frame Dear ImGui
            ImGui_ImplOpenGL2_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Entrées et Mise à jour
            processInput(deltaTime); // Traite les entrées clavier pour le jeu
            update(deltaTime);       // Met à jour l'état du jeu / la simulation

            // Rendu
            render(); // Rend le monde du jeu et l'interface ImGui

            // Gestion des événements
            glfwPollEvents(); // Traite les événements de la fenêtre
        }
    }

private:
    // --- Membres Privés: Fenêtre & Graphismes ---
    GLFWwindow* window = nullptr;   ///< Pointeur vers la fenêtre GLFW.
    int windowWidth;                ///< Largeur actuelle de la fenêtre.
    int windowHeight;               ///< Hauteur actuelle de la fenêtre.
    float gameBoundX = 1.0f;        ///< Limite horizontale du monde du jeu (de -gameBoundX à +gameBoundX).
    float gameBoundY = 1.0f;        ///< Limite verticale du monde du jeu (de -gameBoundY à +gameBoundY).

    // --- Membres Privés: État du Jeu ---
    GameState currentState = GameState::MENU; ///< État actuel du jeu.

    // --- Membres Privés: Objets du Jeu ---
    Paddle playerPaddle;            ///< La raquette du joueur.
    Ball gameBall;                  ///< La balle.
    std::vector<Block> blocks;      ///< Collection de toutes les briques.
    int score = 0;                  ///< Score actuel du joueur.
    int lives = 3;                  ///< Nombre de vies restantes.
    int currentLevel = 1;           ///< Niveau actuel.

    // --- Membres Privés: Objets Bonus ---
    std::vector<FallingBonus> fallingBonuses; ///< Collection des bonus en chute.
    float bonusFallSpeed = 1.0f;    ///< Vitesse de chute des bonus (unités/sec, ajustée pour tomber en ~2s).

    // --- Membres Privés: Gestion du Temps ---
    double lastTime = 0.0;          ///< Temps de la dernière frame pour calculer deltaTime.

    // --- Membres Privés: Logique Spécifique du Jeu ---
    bool firstContactRed = true;    ///< Indicateur pour bonus de vitesse au premier contact avec brique rouge/orange.
    bool firstContactOrange = true; ///< Indicateur pour bonus de vitesse au premier contact avec brique rouge/orange.
    bool paddleShrunk = false;      ///< Indicateur si la raquette a été rétrécie par un malus/mécanique.

    // --- Méthodes Privées: Initialisation ---

    /**
     * @brief Initialise GLFW et crée la fenêtre du jeu.
     * @param width Largeur de la fenêtre.
     * @param height Hauteur de la fenêtre.
     * @param title Titre de la fenêtre.
     * @return Vrai si l'initialisation réussit, faux sinon.
     */
    bool initGLFW(const int& width, const int& height, const char* title) {
        if (!glfwInit()) {
            std::cerr << "Échec de l'initialisation de GLFW" << std::endl;
            return false;
        }
        // Demande un contexte OpenGL 2.1 (compatible avec le backend ImGui OpenGL2)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

        window = glfwCreateWindow(width, height, title, NULL, NULL);
        if (!window) {
            std::cerr << "Échec de la création de la fenêtre GLFW" << std::endl;
            glfwTerminate();
            return false;
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Active V-Sync
        return true;
    }

    /**
     * @brief Configure les callbacks GLFW qui ne sont pas gérés par Dear ImGui.
     * Actuellement, seul le callback de redimensionnement de la fenêtre est nécessaire.
     */
    void setupCallbacks() {
        // ImGui_ImplGlfw_InitForOpenGL installe ses propres gestionnaires pour la plupart des entrées.
        // Nous devons seulement conserver le callback de redimensionnement du framebuffer.
        glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    }

    /**
     * @brief Initialise ou réinitialise l'état du jeu pour une nouvelle partie ou un nouveau niveau.
     * Appelé lorsque le joueur commence une partie depuis le menu ou passe à un niveau supérieur.
     */
    void initGame() {
        score = 0;
        lives = 3;
        currentLevel = 1;
        firstContactRed = true;
        firstContactOrange = true;
        paddleShrunk = false;
        // currentState est défini à PLAYING *avant* d'appeler cette fonction.
        initBlocks();
        resetPlayerAndBall();
        fallingBonuses.clear(); // S'assurer qu'aucun bonus ne persiste d'une partie précédente
    }

    /**
     * @brief Crée et initialise les briques pour le niveau actuel.
     * Dispose les briques en grille avec différents types et propriétés.
     */
    void initBlocks() {
        blocks.clear();
        // Calcule la largeur des briques en fonction des limites actuelles du jeu
        // BRICK_GRID_WIDTH est utilisé pour définir la largeur totale de la zone des briques,
        // gameBoundX est la demi-largeur de l'écran.
        // La grille des briques s'étend de -gameBoundX * (BRICK_GRID_WIDTH / 2.0f) à gameBoundX * (BRICK_GRID_WIDTH / 2.0f)
        // Cependant, la version précédente utilisait 2*gameBoundX pour la largeur totale, ce qui est plus simple.
        // Je vais rester cohérent avec le code original qui s'attendait à ce que la grille s'étende sur toute la largeur.
        // float totalPlayableWidth = gameBoundX * BRICK_GRID_WIDTH; // Si on veut des marges.
        float totalPlayableWidth = 2.0f * gameBoundX * (BRICK_GRID_WIDTH / 2.0f); // (BRICK_GRID_WIDTH / 2.0f) est un peu maladroit, simplifions
              totalPlayableWidth = gameBoundX * BRICK_GRID_WIDTH; // Centré

        float startXOffset = - (totalPlayableWidth / 2.0f); // Pour centrer la grille

        // Le code original utilisait 2*gameBoundX pour la largeur totale de la grille des briques,
        // ce qui signifie que la grille s'étend sur toute la largeur de l'écran.
        // Puis startX = -gameBoundX. Je vais suivre cela pour la compatibilité avec le code original.
        float effectiveGridWidth = 2.0f * gameBoundX; // La grille occupe toute la largeur visible
        float totalGapWidth = (BRICKS_PER_ROW - 1) * BRICK_GAP;
        float brickWidth = (effectiveGridWidth - totalGapWidth) / BRICKS_PER_ROW;
        float startX = -gameBoundX; // Les briques commencent au bord gauche

        // Positions fixes pour les briques bonus et à compteur dans chaque rangée (pour reproductibilité)
        std::srand(42); // Graine fixe pour la génération des positions spéciales
        int bonusPositions[BRICK_ROWS];
        int counterPositions[BRICK_ROWS];

        for (int i = 0; i < BRICK_ROWS; i++) {
            // S'assurer que les positions spéciales ne sont pas sur les bords (0 ou BRICKS_PER_ROW - 1)
            // car ces emplacements peuvent être réservés pour les murs.
            bonusPositions[i] = 1 + (std::rand() % (BRICKS_PER_ROW - 2)); // évite les bords
            do {
                counterPositions[i] = 1 + (std::rand() % (BRICKS_PER_ROW - 2));
            } while (counterPositions[i] == bonusPositions[i]); // S'assure que les positions sont distinctes
        }

        for (int i = 0; i < BRICK_ROWS; ++i) {
            BrickColor baseColorType;
            int points;
            if (i < 2)       { baseColorType = BrickColor::RED;    points = 7; }
            else if (i < 4)  { baseColorType = BrickColor::ORANGE; points = 5; }
            else if (i < 6)  { baseColorType = BrickColor::GREEN;  points = 3; }
            else             { baseColorType = BrickColor::YELLOW; points = 1; }

            for (int j = 0; j < BRICKS_PER_ROW; ++j) {
                Block block;
                block.size = Vec2{brickWidth, BRICK_HEIGHT};
                block.position = Vec2{startX + j * (brickWidth + BRICK_GAP),
                                      BRICK_START_Y - i * (BRICK_HEIGHT + BRICK_GAP)};
                block.active = true;
                block.points = points;
                block.colorType = baseColorType;
                block.color = getColorFromEnum(baseColorType);
                block.hitCounter = 1; // Par défaut, 1 coup pour détruire

                // Attribution des types spéciaux de briques
                if (i == 0 && (j == 0 || j == BRICKS_PER_ROW - 1)) { // Coins supérieurs externes
                    block.isWall = true;
                    block.isReflective = false;
                    block.colorType = BrickColor::GRAY;
                    block.color = getColorFromEnum(BrickColor::GRAY);
                    block.hitCounter = -1; // Indestructible
                } else if (i == 0 && (j == 1 || j == BRICKS_PER_ROW - 2)) { // À côté des coins supérieurs
                    block.isWall = true;
                    block.isReflective = true;
                    block.colorType = BrickColor::WHITE;
                    block.color = getColorFromEnum(BrickColor::WHITE);
                    block.hitCounter = -1; // Indestructible mais réfléchissant
                } else if (j == counterPositions[i] && !block.isWall) { // Brique à compteur
                    block.hitCounter = 2; // Nécessite 2 coups
                    block.color = getColorFromEnum(baseColorType, true); // Couleur plus sombre
                } else if (j == bonusPositions[i] && !block.isWall) { // Brique bonus
                    block.isBonus = true;
                    block.bonusType = std::rand() % 8; // 8 types de bonus (0 à 7)
                }
                blocks.push_back(block);
            }
        }
        std::srand(static_cast<unsigned int>(std::time(nullptr))); // Réinitialise avec une graine temporelle pour le reste
    }

    /**
     * @brief Met à jour les positions et tailles des briques, typiquement après un redimensionnement de fenêtre.
     * Conserve l'état (actif, type, etc.) des briques existantes.
     */
    void updateBlockPositions() {
        if (blocks.empty()) return;

        float effectiveGridWidth = 2.0f * gameBoundX;
        float totalGapWidth = (BRICKS_PER_ROW - 1) * BRICK_GAP;
        float brickWidth = (effectiveGridWidth - totalGapWidth) / BRICKS_PER_ROW;
        float startX = -gameBoundX;

        int blockIndex = 0;
        for (int row = 0; row < BRICK_ROWS; ++row) {
            for (int col = 0; col < BRICKS_PER_ROW; ++col) {
                if (blockIndex < blocks.size()) {
                    Block& block = blocks[blockIndex];
                    block.size.x = brickWidth;
                    // BRICK_HEIGHT est une constante, donc block.size.y ne change pas avec la largeur de l'écran
                    block.position.x = startX + col * (brickWidth + BRICK_GAP);
                    block.position.y = BRICK_START_Y - row * (BRICK_HEIGHT + BRICK_GAP);
                    blockIndex++;
                }
            }
        }
    }


    /**
     * @brief Réinitialise la position et l'état de la raquette et de la balle.
     * Utilisé au début de chaque vie ou niveau.
     */
    void resetPlayerAndBall() {
        // Raquette
        playerPaddle.size = Vec2{paddleShrunk ? PADDLE_WIDTH * 0.5f : PADDLE_WIDTH, PADDLE_HEIGHT};
        playerPaddle.position = Vec2{0.0f - playerPaddle.size.x / 2.0f, PADDLE_Y_POSITION};
        playerPaddle.color = getColorFromEnum(BrickColor::PADDLE);

        // Balle
        gameBall.size = Vec2{BALL_RADIUS * 2.0f, BALL_RADIUS * 2.0f}; // Diamètre
        gameBall.position = Vec2{playerPaddle.position.x + playerPaddle.size.x / 2.0f - BALL_RADIUS,
                                 playerPaddle.position.y + playerPaddle.size.y};
        gameBall.color = getColorFromEnum(BrickColor::BALL);
        gameBall.velocity = Vec2{0.0f, 0.0f};
        gameBall.speedMagnitude = INITIAL_BALL_SPEED;
        gameBall.stuckToPaddle = true;
        gameBall.hitCount = 0; // Réinitialise le compteur de coups pour la vitesse
    }

    /**
     * @brief Fait apparaître un bonus à la position d'une brique détruite.
     * @param block La brique qui a lâché le bonus.
     */
    void spawnBonus(const Block& block) {
        FallingBonus bonus;
        bonus.position = block.position; // Le bonus apparaît où la brique était
        bonus.size = Vec2{BALL_RADIUS * 1.5f, BALL_RADIUS * 1.5f}; // Taille du bonus
        bonus.type = block.bonusType;
        bonus.fallSpeed = bonusFallSpeed; // Vitesse de chute définie globalement
        bonus.active = true;

        // Assigner une couleur distinctive à chaque type de bonus
        switch (bonus.type) {
            case LIFE_ADD:        bonus.color = Color{0.2f, 1.0f, 0.2f, 1.0f}; break; // Vert (vie+)
            case LIFE_REMOVE:     bonus.color = Color{1.0f, 0.2f, 0.2f, 1.0f}; break; // Rouge (vie-)
            case PADDLE_WIDEN:    bonus.color = Color{0.2f, 0.8f, 1.0f, 1.0f}; break; // Cyan (raquette+)
            case PADDLE_SHRINK:   bonus.color = Color{1.0f, 0.5f, 0.0f, 1.0f}; break; // Orange (raquette-)
            case BALL_SLOW:       bonus.color = Color{1.0f, 1.0f, 0.2f, 1.0f}; break; // Jaune (balle lente)
            case BALL_FAST:       bonus.color = Color{0.8f, 0.2f, 1.0f, 1.0f}; break; // Violet (balle rapide)
            case BALL_STRAIGHTEN: bonus.color = Color{1.0f, 1.0f, 1.0f, 1.0f}; break; // Blanc (balle droite)
            case BALL_ANGLE:      bonus.color = Color{0.6f, 0.6f, 0.6f, 1.0f}; break; // Gris (balle angle)
            default:              bonus.color = Color{1.0f, 1.0f, 1.0f, 1.0f}; // Blanc par défaut
        }
        fallingBonuses.push_back(bonus);
    }

    /**
     * @brief Applique l'effet d'un bonus collecté par le joueur.
     * @param bonus Le bonus collecté.
     */
    void applyBonus(const FallingBonus& bonus) {
        switch (bonus.type) {
            case LIFE_ADD:
                lives = std::min(lives + 1, 5); // Max 5 vies
                break;
            case LIFE_REMOVE:
                lives = std::max(lives - 1, 1); // Min 1 vie (ou 0 pour game over immédiat)
                if (lives == 0 && currentState == GameState::PLAYING) { currentState = GameState::GAME_OVER; }
                break;
            case PADDLE_WIDEN:
                playerPaddle.size.x = std::min(playerPaddle.size.x * 1.25f, gameBoundX * 1.5f); // Limite max taille raquette
                // Réajuster la position si elle sort des limites à cause de l'agrandissement
                playerPaddle.position.x = std::min(playerPaddle.position.x, gameBoundX - playerPaddle.size.x);
                break;
            case PADDLE_SHRINK:
                playerPaddle.size.x = std::max(playerPaddle.size.x * 0.75f, PADDLE_WIDTH * 0.25f); // Limite min taille raquette
                break;
            case BALL_SLOW:
                gameBall.speedMagnitude = std::max(gameBall.speedMagnitude * 0.8f, INITIAL_BALL_SPEED * 0.5f); // Vitesse minimale
                normalizeBallVelocity();
                break;
            case BALL_FAST:
                gameBall.speedMagnitude = std::min(gameBall.speedMagnitude * 1.2f, INITIAL_BALL_SPEED * 3.0f); // Vitesse maximale
                normalizeBallVelocity();
                break;
            case BALL_STRAIGHTEN:
                // Redresse la trajectoire (plus verticale)
                if (std::abs(gameBall.velocity.x) > 0.01f) { // Évite division par zéro ou comportement étrange si déjà vertical
                    float currentSignY = (gameBall.velocity.y > 0) ? 1.0f : -1.0f;
                    float currentSignX = (gameBall.velocity.x > 0) ? 1.0f : -1.0f;
                    gameBall.velocity.x = currentSignX * gameBall.speedMagnitude * 0.2f; // Faible composante X
                    gameBall.velocity.y = currentSignY * std::sqrt(gameBall.speedMagnitude * gameBall.speedMagnitude - gameBall.velocity.x * gameBall.velocity.x);
                }
                break;
            case BALL_ANGLE:
                 // Augmente l'angle (plus horizontale)
                if (std::abs(gameBall.velocity.y) > 0.01f) { // Évite comportement étrange si déjà horizontal
                    float currentSignY = (gameBall.velocity.y > 0) ? 1.0f : -1.0f;
                    float currentSignX = (gameBall.velocity.x > 0) ? 1.0f : -1.0f;
                    // Assure une composante X plus importante, mais pas trop pour éviter une balle quasi-horizontale
                    float targetAngleRatio = 0.7f; // 70% de la vitesse sur X, 30% sur Y (environ)
                    gameBall.velocity.x = currentSignX * gameBall.speedMagnitude * targetAngleRatio;
                    gameBall.velocity.y = currentSignY * std::sqrt(gameBall.speedMagnitude * gameBall.speedMagnitude - gameBall.velocity.x * gameBall.velocity.x);
                }
                break;
            default: break;
        }
    }

    // --- Méthodes Privées: Boucle de Jeu ---

    /**
     * @brief Traite les entrées du joueur (clavier).
     * Gère les déplacements de la raquette, le lancement de la balle et les actions du menu/game over.
     * @param dt Delta-temps, temps écoulé depuis la dernière frame.
     */
    void processInput(float dt) {
        // ImGuiIO& io = ImGui::GetIO(); // Peut être utilisé pour vérifier si ImGui capture les entrées
        // if (io.WantCaptureKeyboard) return;

        if (currentState == GameState::PLAYING) {
            // Déplacement de la raquette
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                playerPaddle.position.x -= PADDLE_SPEED * dt;
                playerPaddle.position.x = std::max(playerPaddle.position.x, -gameBoundX); // Bloque à gauche
            }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                playerPaddle.position.x += PADDLE_SPEED * dt;
                playerPaddle.position.x = std::min(playerPaddle.position.x, gameBoundX - playerPaddle.size.x); // Bloque à droite
            }
            // Lancement de la balle
            if (gameBall.stuckToPaddle && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                gameBall.stuckToPaddle = false;
                // Direction X aléatoire (gauche ou droite), Y toujours vers le haut
                float launchAngleFactor = (static_cast<float>(rand()) / RAND_MAX) * 0.8f + 0.2f; // Entre 0.2 et 1.0
                float directionX = (rand() % 2 == 0) ? -1.0f : 1.0f;

                gameBall.velocity.x = directionX * gameBall.speedMagnitude * launchAngleFactor; // Ajuste l'angle de lancement
                gameBall.velocity.y = std::sqrt(gameBall.speedMagnitude * gameBall.speedMagnitude - gameBall.velocity.x * gameBall.velocity.x);
                if (gameBall.velocity.y < 0.1f * gameBall.speedMagnitude) { // S'assurer que la balle monte
                    gameBall.velocity.y = 0.1f * gameBall.speedMagnitude;
                    gameBall.velocity.x = directionX * std::sqrt(gameBall.speedMagnitude * gameBall.speedMagnitude - gameBall.velocity.y * gameBall.velocity.y);
                }
            }
        } else if (currentState == GameState::GAME_OVER) {
            // Retour au menu
            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
                currentState = GameState::MENU;
            }
        }

        // Quitter le jeu (global)
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
    }

    /**
     * @brief Met à jour la logique du jeu (mouvement de la balle, collisions, états).
     * @param dt Delta-temps, temps écoulé depuis la dernière frame.
     */
    void update(float dt) {
        if (currentState == GameState::PLAYING) {
            // Mouvement de la balle
            if (gameBall.stuckToPaddle) {
                gameBall.position = Vec2{playerPaddle.position.x + playerPaddle.size.x / 2.0f - BALL_RADIUS,
                                         playerPaddle.position.y + playerPaddle.size.y};
            } else {
                gameBall.position.x += gameBall.velocity.x * dt;
                gameBall.position.y += gameBall.velocity.y * dt;

                handleCollisions(); // Gère toutes les collisions

                // Condition de perte de vie
                if (gameBall.position.y + gameBall.size.y < -gameBoundY) { // Balle sous le bord inférieur
                    lives--;
                    if (lives <= 0) {
                        currentState = GameState::GAME_OVER;
                    } else {
                        resetPlayerAndBall(); // Réinitialise pour la prochaine vie
                    }
                }
            }

            // Mise à jour des bonus en chute
            for (auto& bonus : fallingBonuses) {
                if (bonus.active) {
                    bonus.position.y -= bonus.fallSpeed * dt; // Mouvement vers le bas
                    // Vérification si le bonus sort de l'écran par le bas
                    if (bonus.position.y + bonus.size.y < -gameBoundY) {
                        bonus.active = false;
                    }
                    // Vérification de la collision avec la raquette
                    if (checkCollision(playerPaddle, bonus_to_gameobject(bonus))) { // Nécessite une conversion ou une fct checkCollision adaptée
                        applyBonus(bonus);
                        bonus.active = false;
                    }
                }
            }
            // Nettoyage des bonus inactifs
            fallingBonuses.erase(
                std::remove_if(fallingBonuses.begin(), fallingBonuses.end(),
                               [](const FallingBonus& b) { return !b.active; }),
                fallingBonuses.end());

            // Condition de victoire (passage au niveau suivant)
            bool allBlocksInactive = true;
            for (const auto& block : blocks) {
                if (block.active && !block.isWall) { // Ne compte pas les murs comme briques à détruire
                    allBlocksInactive = false;
                    break;
                }
            }
            if (allBlocksInactive) {
                currentLevel++;
                firstContactRed = true;    // Réinitialise les bonus de vitesse pour le nouveau niveau
                firstContactOrange = true;
                gameBall.hitCount = 0;     // Réinitialise le compteur de coups de la balle
                initBlocks();              // Génère les briques du nouveau niveau
                resetPlayerAndBall();      // Réinitialise la balle et la raquette
                fallingBonuses.clear();    // Nettoie les bonus restants de l'ancien niveau
                // Le score est conservé entre les niveaux
            }
        }
    }

    /**
     * @brief Adapte une structure FallingBonus pour qu'elle soit utilisable avec checkCollision(GameObject, GameObject).
     * @param bonus Le FallingBonus à convertir.
     * @return Un GameObject temporaire représentant le bonus.
     */
    GameObject bonus_to_gameobject(const FallingBonus& bonus) const {
        GameObject obj;
        obj.position = bonus.position;
        obj.size = bonus.size;
        // color et colorType ne sont pas nécessaires pour la collision ici
        return obj;
    }


    // --- Méthodes Privées: Rendu ---

    /**
     * @brief Fonction principale de rendu.
     * Efface l'écran et dessine les objets du jeu et l'interface utilisateur.
     */
    void render() {
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f); // Fond sombre
        glClear(GL_COLOR_BUFFER_BIT);

        // Rendu des éléments du monde du jeu (si applicable)
        if (currentState == GameState::PLAYING || currentState == GameState::GAME_OVER) {
            // Briques
            for (const auto& block : blocks) {
                if (block.active) {
                    renderGameObject(block);
                }
            }
            // Bonus en chute
            for (const auto& bonus : fallingBonuses) {
                if (bonus.active) {
                    renderFallingBonus(bonus);
                }
            }
            // Raquette
            renderGameObject(playerPaddle);
            // Balle
            // (Rendue si en jeu, ou si game over mais que la balle n'était pas encore perdue/collée)
            if (currentState == GameState::PLAYING || (lives > 0 && !gameBall.stuckToPaddle && currentState == GameState::GAME_OVER) ) {
                 renderGameObject(gameBall);
            }
        }

        // Rendu de l'UI avec Dear ImGui
        renderUI();

        // Finalisation de la frame ImGui et rendu
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window); // Échange les tampons d'affichage
    }

    /**
     * @brief Rend tous les éléments de l'interface utilisateur (UI) avec Dear ImGui.
     * Appelle les fonctions de rendu UI spécifiques à l'état du jeu.
     */
    void renderUI() {
        int currentWindowWidth, currentWindowHeight;
        glfwGetWindowSize(window, ¤tWindowWidth, ¤tWindowHeight);

        if (currentState == GameState::MENU) {
            renderMenuUI(currentWindowWidth, currentWindowHeight);
        } else if (currentState == GameState::PLAYING || currentState == GameState::GAME_OVER) {
            renderGameUI(currentWindowWidth, currentWindowHeight); // Affiche score, vies, niveau
            if (currentState == GameState::GAME_OVER) {
                renderGameOverUI(currentWindowWidth, currentWindowHeight); // Affiche message "Game Over"
            }
        }
    }

    /**
     * @brief Rend l'interface du menu principal.
     * @param wW Largeur actuelle de la fenêtre.
     * @param wH Hauteur actuelle de la fenêtre.
     */
    void renderMenuUI(const int& wW, const int& wH) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(wW), static_cast<float>(wH)));
        ImGui::Begin("MainMenu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);

        // Titre
        const char* title = "BREAKOUT";
        // Utiliser la police par défaut d'ImGui ou une police chargée spécifiquement.
        // Pour un titre plus grand, il faudrait charger une police plus grande et la pousser.
        // ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Supposons que la police par défaut est à l'index 0
        float defaultFontSize = ImGui::GetFontSize();
        ImGui::SetWindowFontScale(3.0f); // Agrandit la police pour ce texte
        ImVec2 titleSize = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX((wW - titleSize.x) / 2.0f);
        ImGui::SetCursorPosY(wH * 0.25f);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 1.0f, 1.0f), "%s", title);
        ImGui::SetWindowFontScale(1.0f); // Rétablir l'échelle de police par défaut
        // ImGui::PopFont();

        // Boutons
        constexpr float buttonWidth = 200.0f;
        constexpr float buttonHeight = 50.0f;
        const float buttonPosX = (wW - buttonWidth) / 2.0f;

        ImGui::SetCursorPos(ImVec2(buttonPosX, wH * 0.5f - buttonHeight - 10.0f));
        if (ImGui::Button("JOUER", ImVec2(buttonWidth, buttonHeight))) {
            currentState = GameState::PLAYING;
            initGame(); // Initialise une nouvelle partie
        }

        ImGui::SetCursorPos(ImVec2(buttonPosX, wH * 0.5f + 10.0f));
        if (ImGui::Button("QUITTER", ImVec2(buttonWidth, buttonHeight))) {
            glfwSetWindowShouldClose(window, true);
        }
        ImGui::End();
    }

    /**
     * @brief Rend l'interface en jeu (score, vies, niveau).
     * @param wW Largeur actuelle de la fenêtre.
     * @param wH Hauteur actuelle de la fenêtre.
     */
    void renderGameUI(const float& wW, const float& wH) const {
        ImDrawList* drawList = ImGui::GetForegroundDrawList(); // Dessine par-dessus le jeu

        // Affichage du score (en haut à gauche)
        std::string scoreText = "SCORE: " + std::to_string(score);
        drawList->AddText(ImVec2(15.0f, 10.0f), IM_COL32(255, 255, 255, 255), scoreText.c_str());

        // Affichage du niveau (en haut au centre)
        std::string levelText = "NIVEAU: " + std::to_string(currentLevel);
        ImVec2 levelTextSize = ImGui::CalcTextSize(levelText.c_str());
        drawList->AddText(ImVec2((wW - levelTextSize.x) / 2.0f, 10.0f), IM_COL32(255, 255, 255, 255), levelText.c_str());

        // Affichage des vies (en haut à droite)
        std::string livesText = "VIES: " + std::to_string(lives);
        ImVec2 livesTextSize = ImGui::CalcTextSize(livesText.c_str());
        drawList->AddText(ImVec2(wW - livesTextSize.x - 15.0f, 10.0f), IM_COL32(255, 255, 255, 255), livesText.c_str());
    }

    /**
     * @brief Rend l'interface de Game Over.
     * @param wW Largeur actuelle de la fenêtre.
     * @param wH Hauteur actuelle de la fenêtre.
     */
    static void renderGameOverUI(const float& wW, const float& wH) {
        ImDrawList* drawList = ImGui::GetForegroundDrawList();

        // Message "GAME OVER"
        const char* gameOverMsg = "GAME OVER";
        float originalFontSize = ImGui::GetFontSize();
        ImGui::SetWindowFontScale(2.5f); // Agrandir la police pour "GAME OVER"
        ImVec2 goTextSize = ImGui::CalcTextSize(gameOverMsg);
        ImVec2 goTextPos = ImVec2((wW - goTextSize.x) / 2.0f, wH * 0.4f);
        drawList->AddText(goTextPos, IM_COL32(255, 50, 50, 255), gameOverMsg);
        ImGui::SetWindowFontScale(1.0f); // Rétablir

        // Message "Press ENTER"
        const char* restartMsg = "Appuyez sur ENTRER pour retourner au Menu";
        ImVec2 restartTextSize = ImGui::CalcTextSize(restartMsg);
        ImVec2 restartTextPos = ImVec2((wW - restartTextSize.x) / 2.0f, wH * 0.6f);
        drawList->AddText(restartTextPos, IM_COL32(255, 255, 255, 255), restartMsg);
    }

    /**
     * @brief Rend un GameObject (brique, raquette, balle) en utilisant OpenGL en mode immédiat.
     * @param obj L'objet à rendre.
     */
    static void renderGameObject(const GameObject& obj) {
        glColor4f(obj.color.r, obj.color.g, obj.color.b, obj.color.a);
        glBegin(GL_QUADS);
        glVertex2f(obj.position.x, obj.position.y);
        glVertex2f(obj.position.x + obj.size.x, obj.position.y);
        glVertex2f(obj.position.x + obj.size.x, obj.position.y + obj.size.y);
        glVertex2f(obj.position.x, obj.position.y + obj.size.y);
        glEnd();
    }

    /**
     * @brief Rend un bonus en chute.
     * @param bonus Le bonus à rendre.
     */
    static void renderFallingBonus(const FallingBonus& bonus) {
        glColor4f(bonus.color.r, bonus.color.g, bonus.color.b, bonus.color.a);
        glBegin(GL_QUADS);
        glVertex2f(bonus.position.x, bonus.position.y);
        glVertex2f(bonus.position.x + bonus.size.x, bonus.position.y);
        glVertex2f(bonus.position.x + bonus.size.x, bonus.position.y + bonus.size.y);
        glVertex2f(bonus.position.x, bonus.position.y + bonus.size.y);
        glEnd();
    }


    // --- Méthodes Privées: Gestion des Collisions ---

    /**
     * @brief Vérifie s'il y a collision entre deux GameObjects (AABB - Axis-Aligned Bounding Box).
     * @param one Le premier GameObject.
     * @param two Le second GameObject.
     * @return Vrai si collision, faux sinon.
     */
    static bool checkCollision(const GameObject& one, const GameObject& two) {
        // Collision sur l'axe X?
        bool collisionX = one.position.x + one.size.x >= two.position.x &&
                          two.position.x + two.size.x >= one.position.x;
        // Collision sur l'axe Y?
        bool collisionY = one.position.y + one.size.y >= two.position.y &&
                          two.position.y + two.size.y >= one.position.y;
        // Collision seulement si les deux axes se chevauchent
        return collisionX && collisionY;
    }

    /**
     * @brief Gère toutes les collisions potentielles dans le jeu (balle-murs, balle-raquette, balle-briques).
     */
    void handleCollisions() {
        handleBallWallCollision(); // Collision balle avec les murs/plafond

        if (checkCollision(gameBall, playerPaddle)) { // Collision balle avec raquette
            resolveBallPaddleCollision();
        }

        for (auto& block : blocks) { // Collision balle avec les briques
            if (block.active && checkCollision(gameBall, block)) {
                resolveBallBlockCollision(block);
                // Important: 'break' après avoir traité UNE collision de brique par frame
                // pour éviter des comportements multiples si la balle est entre deux briques.
                break;
            }
        }
    }

    /**
     * @brief Gère la collision de la balle avec les bords de l'aire de jeu.
     */
    void handleBallWallCollision() {
        // Collision avec mur gauche
        if (gameBall.position.x <= -gameBoundX) {
            gameBall.velocity.x = std::abs(gameBall.velocity.x); // Rebond
            gameBall.position.x = -gameBoundX; // Correction de position
        }
        // Collision avec mur droit
        else if (gameBall.position.x + gameBall.size.x >= gameBoundX) {
            gameBall.velocity.x = -std::abs(gameBall.velocity.x); // Rebond
            gameBall.position.x = gameBoundX - gameBall.size.x; // Correction de position
        }
        // Collision avec plafond
        if (gameBall.position.y + gameBall.size.y >= gameBoundY) {
            gameBall.velocity.y = -std::abs(gameBall.velocity.y); // Rebond
            gameBall.position.y = gameBoundY - gameBall.size.y; // Correction de position
            // Effet de rétrécissement de la raquette au contact du plafond (mécanique de jeu)
            if (!paddleShrunk) {
                paddleShrunk = true;
                playerPaddle.size.x *= 0.5f;
                 // S'assurer que la raquette ne sort pas des limites après rétrécissement si elle était au bord
                playerPaddle.position.x = std::min(playerPaddle.position.x, gameBoundX - playerPaddle.size.x);
            }
        }
        // La collision avec le bas (perte de vie) est gérée dans update()
    }

    /**
     * @brief Résout la collision entre la balle et la raquette.
     * Modifie la direction de la balle en fonction du point d'impact sur la raquette.
     */
    void resolveBallPaddleCollision() {
        // S'assurer que la balle vient d'en haut (velocity.y < 0) pour éviter double collision
        // ou collision par le bas si la balle passait à travers.
        // Note: Dans ce jeu, la balle est toujours au-dessus de la raquette avant collision valide.
        // On peut cependant vérifier que la balle se déplace vers le bas.
        if (gameBall.velocity.y >= 0.0f && gameBall.position.y + gameBall.size.y > playerPaddle.position.y + playerPaddle.size.y) {
            // Si la balle est déjà au-dessus et monte, ou est statique au-dessus, ignorer.
            // Cela peut arriver si la balle est "poussée" par la raquette.
             return; // Ou gameBall.velocity.y = std::abs(gameBall.velocity.y); pour forcer le rebond vers le haut.
        }


        // Repositionnement de la balle juste au-dessus de la raquette pour éviter qu'elle ne s'enfonce.
        gameBall.position.y = playerPaddle.position.y + playerPaddle.size.y;
        gameBall.velocity.y = std::abs(gameBall.velocity.y); // Rebond vertical

        // Calcul du point d'impact normalisé sur la raquette (-1 à gauche, 0 au centre, 1 à droite)
        float ballCenterX = gameBall.position.x + gameBall.size.x * 0.5f;
        float paddleCenterX = playerPaddle.position.x + playerPaddle.size.x * 0.5f;
        float offset = (ballCenterX - paddleCenterX) / (playerPaddle.size.x * 0.5f);
        float normalizedOffset = std::max(-1.0f, std::min(1.0f, offset)); // Limite entre -1 et 1

        // Modifier la composante X de la vitesse en fonction de normalizedOffset
        // Un angle plus prononcé si la balle frappe les bords de la raquette.
        // Max angle influence: si offset est 1 ou -1, vx/vy ratio.
        float influence = 1.0f; // Facteur d'influence de l'offset sur l'angle.
        gameBall.velocity.x = gameBall.speedMagnitude * normalizedOffset * influence;

        // Recalculer la composante Y pour maintenir la magnitude de la vitesse totale
        // (speedMagnitude^2 = vx^2 + vy^2) => (vy = sqrt(speedMagnitude^2 - vx^2))
        float newVySquared = gameBall.speedMagnitude * gameBall.speedMagnitude - gameBall.velocity.x * gameBall.velocity.x;
        if (newVySquared < 0) { // Peut arriver si vx est trop grand (influence trop forte)
            // Si vx^2 > speedMagnitude^2, alors la balle irait trop horizontalement.
            // On limite vx pour s'assurer que vy est réel et positif.
            // Par exemple, limiter l'angle maximal.
            float maxVxRatio = 0.95f; // vx ne peut pas être plus de 95% de speedMagnitude
            if (std::abs(gameBall.velocity.x) > gameBall.speedMagnitude * maxVxRatio) {
                 gameBall.velocity.x = (gameBall.velocity.x > 0 ? 1.0f : -1.0f) * gameBall.speedMagnitude * maxVxRatio;
            }
            newVySquared = gameBall.speedMagnitude * gameBall.speedMagnitude - gameBall.velocity.x * gameBall.velocity.x;
        }
        gameBall.velocity.y = std::sqrt(newVySquared);

        // S'assurer que la balle monte toujours après avoir frappé la raquette
        if (gameBall.velocity.y <= 0) {
            gameBall.velocity.y = 0.1f * gameBall.speedMagnitude; // Petite vitesse verticale minimale
             // Recalculer X si Y a été forcé
            gameBall.velocity.x = (gameBall.velocity.x > 0 ? 1.0f : -1.0f) * std::sqrt(gameBall.speedMagnitude * gameBall.speedMagnitude - gameBall.velocity.y * gameBall.velocity.y);
        }
    }

    /**
     * @brief Résout la collision entre la balle et une brique.
     * Gère la destruction de la brique, le score, les bonus et le rebond de la balle.
     * @param block La brique avec laquelle la balle est entrée en collision.
     */
    void resolveBallBlockCollision(Block& block) {
        // Gestion des murs spéciaux (indestructibles ou réfléchissants)
        if (block.isWall) {
            if (block.isReflective) { // Mur réfléchissant : inversion complète de la vitesse
                gameBall.velocity.x *= -1.0f;
                gameBall.velocity.y *= -1.0f;
            } else { // Mur indestructible normal : rebond standard
                // Déterminer la direction de la collision pour le rebond
                float overlapLeft = (gameBall.position.x + gameBall.size.x) - block.position.x;
                float overlapRight = (block.position.x + block.size.x) - gameBall.position.x;
                float overlapTop = (gameBall.position.y + gameBall.size.y) - block.position.y;
                float overlapBottom = (block.position.y + block.size.y) - gameBall.position.y;

                bool fromLeft = std::abs(gameBall.velocity.x) > 0 && overlapLeft < overlapRight && overlapLeft < overlapTop && overlapLeft < overlapBottom;
                bool fromRight = std::abs(gameBall.velocity.x) > 0 && overlapRight < overlapLeft && overlapRight < overlapTop && overlapRight < overlapBottom;
                bool fromBottom = std::abs(gameBall.velocity.y) > 0 && overlapBottom < overlapTop && overlapBottom < overlapLeft && overlapBottom < overlapRight;
                // bool fromTop = std::abs(gameBall.velocity.y) > 0 && overlapTop < overlapBottom && overlapTop < overlapLeft && overlapTop < overlapRight;

                // Choisir le plus petit chevauchement pour déterminer le côté de la collision
                float minOverlapX = std::min(overlapLeft, overlapRight);
                float minOverlapY = std::min(overlapTop, overlapBottom);

                if (minOverlapX < minOverlapY) { // Collision horizontale
                    gameBall.velocity.x *= -1.0f;
                    // Correction de position
                    if (gameBall.velocity.x > 0) gameBall.position.x = block.position.x - gameBall.size.x; // Venait de gauche
                    else gameBall.position.x = block.position.x + block.size.x; // Venait de droite
                } else { // Collision verticale
                    gameBall.velocity.y *= -1.0f;
                     // Correction de position
                    if (gameBall.velocity.y > 0) gameBall.position.y = block.position.y - gameBall.size.y; // Venait du bas
                    else gameBall.position.y = block.position.y + block.size.y; // Venait du haut
                }
            }
            return;
        }

        // Briques destructibles
        block.hitCounter--;
        if (block.hitCounter <= 0) {
            block.active = false;
            score += block.points;
            if (block.isBonus) {
                spawnBonus(block);
            }
        } else {
             block.color = getColorFromEnum(block.colorType, false);
        }

        // Rebond sur la brique (similaire au mur normal)
        float overlapLeft = (gameBall.position.x + gameBall.size.x) - block.position.x;
        float overlapRight = (block.position.x + block.size.x) - gameBall.position.x;
        float overlapTop = (gameBall.position.y + gameBall.size.y) - block.position.y;
        float overlapBottom = (block.position.y + block.size.y) - gameBall.position.y;

        float minOverlapX = std::min(overlapLeft, overlapRight);
        float minOverlapY = std::min(overlapTop, overlapBottom);

        // Le côté de la collision affecte le rebond.
        // On essaie de déterminer si la collision est plus horizontale ou verticale.
        // Une heuristique simple : si la balle est plus "enfoncée" horizontalement qu'verticalement, c'est une collision latérale.
        float penetrationX = (gameBall.size.x / 2 + block.size.x / 2) - std::abs((gameBall.position.x + gameBall.size.x/2) - (block.position.x + block.size.x/2));
        float penetrationY = (gameBall.size.y / 2 + block.size.y / 2) - std::abs((gameBall.position.y + gameBall.size.y/2) - (block.position.y + block.size.y/2));

        if (penetrationX < penetrationY) { // Collision plus horizontale
            gameBall.velocity.x *= -1.0f;
            // Correction de position pour éviter de rester coincé
             if (gameBall.velocity.x > 0) gameBall.position.x = block.position.x - gameBall.size.x - 0.001f;
             else gameBall.position.x = block.position.x + block.size.x + 0.001f;
        } else { // Collision plus verticale
            gameBall.velocity.y *= -1.0f;
            // Correction de position
            if (gameBall.velocity.y > 0) gameBall.position.y = block.position.y - gameBall.size.y - 0.001f;
            else gameBall.position.y = block.position.y + block.size.y + 0.001f;
        }


        // Augmentation de la vitesse de la balle
        gameBall.hitCount++;
        applySpeedIncrease(block); // Le paramètre block est utilisé pour les contacts spécifiques
    }

    // --- Méthodes Privées: Utilitaires de Jeu ---

    /**
     * @brief Applique une augmentation de vitesse à la balle selon certaines conditions.
     * (nombre de coups, contact avec briques spécifiques).
     * @param b La brique qui vient d'être touchée (utilisé pour les conditions de couleur).
     */
    void applySpeedIncrease(const Block& b) {
        bool speedIncreased = false;
        // Augmentation de vitesse tous les X coups
        if (gameBall.hitCount == 4 || gameBall.hitCount == 12) {
            gameBall.speedMagnitude *= BALL_SPEED_INCREMENT;
            speedIncreased = true;
        }
        // Augmentation de vitesse au premier contact avec les briques orange/rouges du niveau
        if (firstContactOrange && b.colorType == BrickColor::ORANGE) {
            gameBall.speedMagnitude *= BALL_SPEED_INCREMENT;
            firstContactOrange = false;
            speedIncreased = true;
        }
        if (firstContactRed && b.colorType == BrickColor::RED) {
            gameBall.speedMagnitude *= BALL_SPEED_INCREMENT;
            firstContactRed = false;
            speedIncreased = true;
        }

        if (speedIncreased) {
            normalizeBallVelocity();
        }
    }

    /**
     * @brief Normalise la vitesse de la balle pour qu'elle corresponde à `gameBall.speedMagnitude`.
     * Maintient la direction actuelle de la balle mais ajuste sa vitesse.
     */
    void normalizeBallVelocity() {
        float currentSpeed = std::sqrt(gameBall.velocity.x * gameBall.velocity.x + gameBall.velocity.y * gameBall.velocity.y);
        if (currentSpeed > 0.0001f) { // Évite la division par zéro
            gameBall.velocity.x = (gameBall.velocity.x / currentSpeed) * gameBall.speedMagnitude;
            gameBall.velocity.y = (gameBall.velocity.y / currentSpeed) * gameBall.speedMagnitude;
        } else if (!gameBall.stuckToPaddle) {
            // Si la vitesse est nulle et que la balle n'est pas collée, lui donner une vitesse par défaut (ex: vers le haut)
            // Cela peut arriver si elle est spawnée avec vitesse nulle ou après un bug.
             float launchAngleFactor = (static_cast<float>(rand()) / RAND_MAX) * 0.8f + 0.2f;
             float directionX = (rand() % 2 == 0) ? -1.0f : 1.0f;
             gameBall.velocity.x = directionX * gameBall.speedMagnitude * launchAngleFactor;
             gameBall.velocity.y = std::sqrt(gameBall.speedMagnitude * gameBall.speedMagnitude - gameBall.velocity.x * gameBall.velocity.x);
             if (gameBall.velocity.y <= 0) gameBall.velocity.y = 0.1f * gameBall.speedMagnitude; // S'assurer qu'elle monte
        }
    }

    // --- Méthodes Privées: Callbacks GLFW ---

    /**
     * @brief Callback GLFW pour le redimensionnement du framebuffer (fenêtre).
     * Met à jour la matrice de projection et le viewport.
     * @param window La fenêtre GLFW concernée.
     * @param width Nouvelle largeur du framebuffer.
     * @param height Nouvelle hauteur du framebuffer.
     */
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
        auto gameInstance = static_cast<Game*>(glfwGetWindowUserPointer(window));
        if (gameInstance) {
            glViewport(0, 0, width, height); // Important: définir le viewport ici aussi
            gameInstance->updateProjectionMatrix(width, height);
        }
    }

    /**
     * @brief Met à jour la matrice de projection OpenGL en fonction de la taille de la fenêtre.
     * Maintient un aspect ratio correct et ajuste les limites du monde du jeu.
     * @param width Nouvelle largeur de la fenêtre.
     * @param height Nouvelle hauteur de la fenêtre.
     */
    void updateProjectionMatrix(int width, int height) {
        if (height == 0) height = 1; // Empêche la division par zéro

        windowWidth = width;
        windowHeight = height;

        // Sauvegarde des anciennes limites pour ajuster la vitesse de la balle si nécessaire
        // float oldBoundX = gameBoundX;
        // float oldBoundY = gameBoundY;

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        const float aspect = static_cast<float>(width) / static_cast<float>(height);

        // Ajuste gameBoundX et gameBoundY pour que le monde du jeu s'adapte à l'aspect ratio.
        // L'objectif est de toujours voir une hauteur de -1.0 à 1.0 (ou largeur, selon l'orientation).
        if (width >= height) { // Fenêtre plus large ou carrée
            gameBoundX = aspect; // Le monde s'étend de -aspect à +aspect en X
            gameBoundY = 1.0f;   // Le monde s'étend de -1.0 à +1.0 en Y
            glOrtho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
        } else { // Fenêtre plus haute
            gameBoundX = 1.0f;   // Le monde s'étend de -1.0 à +1.0 en X
            gameBoundY = 1.0f / aspect; // Le monde s'étend de -1/aspect à +1/aspect en Y
            glOrtho(-1.0f, 1.0f, -1.0f / aspect, 1.0f / aspect, -1.0f, 1.0f);
        }

        // Note: La logique de mise à l'échelle de la vitesse de la balle lors du redimensionnement
        // a été retirée car elle peut être complexe à bien faire et introduire des effets non désirés.
        // Les vitesses sont définies en unités normalisées, donc elles s'adapteront naturellement
        // à la nouvelle taille du monde si les objets eux-mêmes sont aussi en coordonnées normalisées.
        // La mise à jour des positions/tailles des briques est gérée par updateBlockPositions.

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Mettre à jour la position des briques pour s'adapter aux nouvelles limites
        if (currentState == GameState::PLAYING && !blocks.empty()) {
            updateBlockPositions();
        }
    }
}; // Fin de la classe Game

//-----------------------------------------------------------------------------
// Fonction Principale (main)
//-----------------------------------------------------------------------------
int main() {
    try {
        Game breakoutGame(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
        breakoutGame.run();
    } catch (const std::exception& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
        // S'assurer de la terminaison de GLFW même si le constructeur échoue partiellement
        if (glfwGetCurrentContext()) { // Vérifier si un contexte a été créé
             // Peut-être que le destructeur de Game n'est pas appelé si une exception est levée dans le constructeur
             // avant l'initialisation complète de ImGui/GLFW.
        }
        glfwTerminate(); // S'assurer que GLFW est terminé.
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Une erreur inconnue est survenue." << std::endl;
        if (glfwGetCurrentContext()) {
        }
        glfwTerminate();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
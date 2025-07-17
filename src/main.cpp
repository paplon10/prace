#include <gl2d/gl2d.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>  
#include <algorithm>  
#include <map>
#include <cctype>

// Window size
const int GAME_WIDTH = 640;
const int PANEL_WIDTH = 200;  
const int WIDTH = GAME_WIDTH + PANEL_WIDTH;
const int HEIGHT = 640;

// Structure to hold 2D points
struct Point
{
    float x, y;
    Point() : x(0.0f), y(0.0f) {}
    Point(float _x, float _y) : x(_x), y(_y) {}
};

// Structure to hold color
struct Color
{
    float r, g, b, a;
    Color(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
};

// Enemy types
enum class EnemyType
{
    SKELETON, // Blue, fast enemy
    ZOMBIE,   // Red, normal enemy
    BOSS,     // Large, slow enemy with lots of health
    TANK,     // Slow, high health enemy
    GHOST     // Very fast, low health enemy
};

// Tower types
enum class TowerType
{
    NONE,        // No tower selected
    APPLE,       // Balanced tower
    CARROT,      // Long range tower
    POTATO,      // Fast attack speed, low range
    PINEAPPLE,   // Area tower that shoots in 8 directions
    BANANA_PEEL, // Special trap that can only be placed on path
    CACTUS       // trap that does low damage but stays forever
};

// Tower costs
const int BASE_APPLE_COST = 15;
const int BASE_CARROT_COST = 25;
const int BASE_POTATO_COST = 20;
const int BASE_PINEAPPLE_COST = 30;
const int BASE_BANANA_PEEL_COST = 5; // Cheap trap tower
const int BASE_CACTUS_COST = 35;     // Expensive high damage tower

// Tower count trackers for increasing costs
int appleTowerCount = 0;
int carrotTowerCount = 0;
int potatoTowerCount = 0;
int pineappleTowerCount = 0;
int bananaPeelTowerCount = 0;
int cactusTowerCount = 0;

// Round system variables
int currentRound = 0;
int enemiesLeftInRound = 0;
float roundStartTimer = 5.0f;
bool isRoundActive = false;
float enemySpawnTimer = 0.0f;

// Game state variables
bool isGameOver = false;
int lives = 3; // Number of enemies that can exit before game over

// Global variable for tracking when mouse was just pressed
bool mouseJustPressed = false;

// Add after game state variables
float gameStartTimer = 2.0f; // Show 'GAME START' for 2 seconds

// Tower struct
struct Tower
{
    Point pos;
    bool isPlaced;
    float shootTimer;
    float range;
    float damage;
    float attackSpeed; // Shots per second
    float projectileSpeed;
    TowerType type;
    bool isUsed;                 // For one-time traps like banana peels
    int usesLeft;                // For multi-use traps like cactus
    int damageUpgradeLevel;      // Track damage upgrade level
    int attackSpeedUpgradeLevel; // Track attack speed upgrade level
    int rangeUpgradeLevel;       // Track range upgrade level
    
    Tower() : pos(0, 0), isPlaced(false), shootTimer(0), range(150.0f), 
              damage(10.0f), attackSpeed(1.0f), projectileSpeed(300.0f), 
              type(TowerType::NONE), isUsed(false), usesLeft(0), 
              damageUpgradeLevel(0), attackSpeedUpgradeLevel(0), rangeUpgradeLevel(0) {}
              
    Tower(float x, float y) : pos(x, y), isPlaced(true), shootTimer(0), 
                              range(150.0f), damage(10.0f), attackSpeed(1.0f), 
                              projectileSpeed(300.0f), type(TowerType::NONE), 
                              isUsed(false), usesLeft(0),
                              damageUpgradeLevel(0), attackSpeedUpgradeLevel(0), rangeUpgradeLevel(0) {}
                              
    void setType(TowerType newType)
    {
        type = newType;
        damageUpgradeLevel = 0;
        attackSpeedUpgradeLevel = 0;
        rangeUpgradeLevel = 0;
        switch (type)
        {
            case TowerType::APPLE:  
                range = 115.0f;  
                damage = 13.0f;
                attackSpeed = 1.5f;
                projectileSpeed = 600.0f;
                usesLeft = 0;
                // Fires 10x10 pixel black projectiles
                break;
            case TowerType::CARROT:  
                range = 250.0f;  
                damage = 26.0f;
                attackSpeed = 0.75f;
                projectileSpeed = 1125.0f;
                usesLeft = 0;
                // Fires 10x10 pixel black projectiles
                break;
            case TowerType::POTATO:  
                range = 75.0f;  
                damage = 4.0f;   
                attackSpeed = 5.0f;
                projectileSpeed = 750.0f;
                usesLeft = 0;
                // Fires 10x10 pixel black projectiles
                break;
            case TowerType::PINEAPPLE:  
                range = 90.0f;  
                damage = 10.0f;  
                attackSpeed = 1.0f;
                projectileSpeed = 450.0f;
                usesLeft = 0;
                // Fires 10x10 pixel black projectiles in 8 directions
                break;
            case TowerType::BANANA_PEEL:
                range = 0.0f;  
                damage = 50.0f;  
                attackSpeed = 0.0f; 
                projectileSpeed = 0.0f;
                usesLeft = 0;    
                break;
            case TowerType::CACTUS:
                range = 0.0f;    
                damage = 20.0f;   
                attackSpeed = 0.0f;
                projectileSpeed = 0.0f;
                usesLeft = -1;   
                break;
            default:
                break;
        }
    }

    // Get upgrade cost based on tower type and upgrade level
    int getUpgradeCost(TowerType type, int currentLevel)
    {
        if (currentLevel >= 3)
            return -1; // Max level reached
        if (type == TowerType::BANANA_PEEL)
            return -1; // No upgrades for banana peel
        
        // Get base cost for tower type
        int baseCost;
        switch (type)
        {
            case TowerType::APPLE:
                baseCost = BASE_APPLE_COST;
                break;
            case TowerType::CARROT:
                baseCost = BASE_CARROT_COST;
                break;
            case TowerType::POTATO:
                baseCost = BASE_POTATO_COST;
                break;
            case TowerType::PINEAPPLE:
                baseCost = BASE_PINEAPPLE_COST;
                break;
            case TowerType::CACTUS:
                baseCost = BASE_CACTUS_COST;
                break;
            default:
                return 0;
        }
        
        return (baseCost * (currentLevel + 1)) / 2; // Each upgrade costs 50% more than the last
    }

    // Apply upgrade effects
    void upgradeDamage()
    {
        if (damageUpgradeLevel >= 3)
            return; // Max level reached
        if (type == TowerType::BANANA_PEEL)
            return; // No upgrades for banana peel
        
        damageUpgradeLevel++;
        damage *= 1.3f; // +30% damage per level
    }

    void upgradeAttackSpeed()
    {
        if (attackSpeedUpgradeLevel >= 3)
            return; // Max level reached
        if (type == TowerType::BANANA_PEEL)
            return; // No upgrades for banana peel
        
        attackSpeedUpgradeLevel++;
        attackSpeed *= 1.2f; // +20% attack speed per level
    }

    void upgradeRange()
    {
        if (rangeUpgradeLevel >= 3)
            return; // Max level reached
        if (type == TowerType::BANANA_PEEL)
            return; // No upgrades for banana peel
        
        rangeUpgradeLevel++;
        range *= 1.15f;           // +15% range per level
        projectileSpeed *= 1.15f; // Also increase projectile speed by 15%
    }
};

// Projectile struct
struct Projectile
{
    Point pos;
    Point velocity;
    bool active;
    float speed;
    float damage;
    float distanceTraveled;
    float maxDistance;
    TowerType sourceType; // Track which tower type fired this projectile
    
    Projectile() : pos(0, 0), velocity(0, 0), active(false), speed(450.0f), damage(10.0f), 
                   distanceTraveled(0.0f), maxDistance(300.0f), sourceType(TowerType::NONE) {}
    
    void update(float deltaTime)
    {
        if (!active)
            return;
        
        // Calculate movement
        float moveX = velocity.x * speed * deltaTime;
        float moveY = velocity.y * speed * deltaTime;
        
        // Update position
        pos.x += moveX;
        pos.y += moveY;
        
        // Track distance traveled
        distanceTraveled += std::sqrt(moveX * moveX + moveY * moveY);
        
        // Deactivate if maximum distance reached
        if (distanceTraveled >= maxDistance)
        {
            active = false;
        }
    }
};

// Tower settings
const float TOWER_SIZE = 40.0f;
const float ENEMY_SIZE = 30.0f;       
const float BOSS_SIZE = 60.0f;        
const float TANK_SIZE = 40.0f;        
const float GHOST_SIZE = 28.0f;
const float PROJECTILE_SIZE = 20.0f; // Increased from 10 for better visibility

// Tower colors for different types
const Color APPLE_TOWER_COLOR = {1.0f, 0.2f, 0.2f, 1.0f};     // Red for apple
const Color CARROT_TOWER_COLOR = {1.0f, 0.4f, 0.0f, 1.0f};    // Orange for carrot
const Color POTATO_TOWER_COLOR = {1.0f, 1.0f, 0.0f, 1.0f};    // Yellow for potato (was RAPID)
const Color PINEAPPLE_TOWER_COLOR = {1.0f, 0.9f, 0.4f, 1.0f}; // Yellow-orange for pineapple
const Color BANANA_PEEL_COLOR = {0.9f, 0.8f, 0.2f, 1.0f};     // Bright yellow for banana peel
const Color CACTUS_TOWER_COLOR = {0.0f, 0.8f, 0.4f, 1.0f};    // Green for cactus

// Colors for the UI
const Color UI_BACKGROUND = {0.2f, 0.2f, 0.2f, 1.0f};
const Color UI_SELECTED = {0.4f, 0.4f, 0.4f, 1.0f};
const Color UI_TEXT = {1.0f, 1.0f, 1.0f, 1.0f};

// Rectangle structure for UI elements
struct Rectangle
{
    float x, y, w, h;
    Rectangle(float _x, float _y, float _w, float _h) : x(_x), y(_y), w(_w), h(_h) {}
    Rectangle() : x(0), y(0), w(0), h(0) {}
};

// Tower menu structure
struct TowerMenu
{
    bool isOpen;
    Tower *selectedTower;
    Rectangle sellButton;
    Rectangle upgradeButton1; // First upgrade option
    Rectangle upgradeButton2; // Second upgrade option
    Rectangle upgradeButton3; // Third upgrade option
    Rectangle closeButton;
    
    TowerMenu() : isOpen(false), selectedTower(nullptr)
    {
        // Initialize with dummy values; will be set dynamically before rendering
        sellButton = Rectangle(0, 0, 160, 40);
        upgradeButton1 = Rectangle(0, 0, 160, 40);
        upgradeButton2 = Rectangle(0, 0, 160, 40);
        upgradeButton3 = Rectangle(0, 0, 160, 40);
        closeButton = Rectangle(0, 0, 30, 30);
    }
    
    void open(Tower *tower)
    {
        isOpen = true;
        selectedTower = tower;
        // Button positions will be set dynamically before rendering
    }

    void close()
    {
        isOpen = false;
        selectedTower = nullptr;
    }
};

// Tower selection UI
struct TowerButton
{
    Rectangle rect;
    TowerType type;
    bool isHovered;
    
    TowerButton(float x, float y, float w, float h, TowerType t) 
        : rect{x, y, w, h}, type(t), isHovered(false) {}
};

// Add game state enum for menu and game
enum class GameScreen
{
    MAIN_MENU,
    MAP_SELECT,
    GAME,
    OPTIONS,
    PAUSE_MENU
};
GameScreen currentScreen = GameScreen::MAIN_MENU;

// Function to get enemy name
std::string getEnemyName(EnemyType type)
{
    switch (type)
    {
        case EnemyType::SKELETON:
            return "Skeleton";
        case EnemyType::BOSS:
            return "BOSS";
        case EnemyType::TANK:
            return "Tank";
        case EnemyType::GHOST:
            return "Ghost";
        case EnemyType::ZOMBIE:
        default:
            return "Zombie";
    }
}

// Path waypoints
std::vector<Point> waypoints = {
    Point(352, 0), // starting point
    Point(352, 96),
    Point(160, 96),
    Point(160, 160),
    Point(96, 160),
    Point(96, 352),
    Point(224, 352),
    Point(224, 288),
    Point(416, 288),
    Point(416, 224),
    Point(544, 224),
    Point(544, 480),
    Point(288, 480),
    Point(288, 544),
    Point(-69, 544) // end point
};

// For the desert map
std::vector<Point> desertWaypoints = {
    Point(90, 640),    
    Point(90, 85),   
    Point(330, 85),  
    Point(330, 465), 
    Point(470, 465), 
    Point(470, 28),  
    Point(520, 28),  
    Point(520, 520),    
    Point(280, 520),    
    Point(280, 130),
    Point(135, 130),
    Point(135, 640),
};

// Function to get scaled waypoints based on current scale
std::vector<Point> getScaledWaypoints(float scaleX, float scaleY)
{
    std::vector<Point> scaled;
    for (const auto &wp : waypoints)
    {
        scaled.emplace_back(wp.x * scaleX, wp.y * scaleY);
    }
    return scaled;
}

// Function to calculate distance between two points
float distance(const Point &a, const Point &b)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

// Define water region (top-right corner)
const bool isInWaterRegion(const Point &p)
{
    // Blue water is approximately in the top-right corner of the map
    return p.x > 500 && p.y < 120;
}

// Function to check if a point is too close to the path
bool isNearPath(const Point &p, const std::vector<Point> &waypoints, float threshold = 28.0f)
{
    for (size_t i = 0; i < waypoints.size() - 1; ++i)
    {
        const Point &a = waypoints[i];
        const Point &b = waypoints[i + 1];
        float lineLength = distance(a, b);
        if (lineLength < 0.1f)
            continue;
        float abx = b.x - a.x;
        float aby = b.y - a.y;
        float apx = p.x - a.x;
        float apy = p.y - a.y;
        float projection = (apx * abx + apy * aby) / (abx * abx + aby * aby);
        float clampedProjection = std::max(0.0f, std::min(1.0f, projection));
        float closestX = a.x + clampedProjection * abx;
        float closestY = a.y + clampedProjection * aby;
        float dist = std::sqrt((p.x - closestX) * (p.x - closestX) + 
                              (p.y - closestY) * (p.y - closestY));
        if (dist < threshold)
        {
            return true;
        }
    }
    return false;
}

// Structure for enemies
struct Enemy
{
    // Remove: Point pos;
    int currentWaypoint;
    bool isActive;
    EnemyType type;
    std::string name;
    float health;
    float maxHealth;
    float progress; // 0.0 to 1.0 between waypoints
    // Add:
    Point getPosition(const std::vector<Point> &waypoints) const
    {
        if (currentWaypoint >= waypoints.size() - 1)
            return waypoints.back();
        const Point &a = waypoints[currentWaypoint];
        const Point &b = waypoints[currentWaypoint + 1];
        return Point(a.x + (b.x - a.x) * progress, a.y + (b.y - a.y) * progress);
    }
    Enemy() : currentWaypoint(0), isActive(false), type(EnemyType::ZOMBIE),
              health(0.0f), maxHealth(0.0f), progress(0.0f)
    {
        name = getEnemyName(type);
    }
    void setType(EnemyType newType)
    {
        type = newType;
        name = getEnemyName(type);
        float baseHealth = 0.0f;
        switch (type)
        {
        case EnemyType::ZOMBIE:
            baseHealth = 40.0f;
            break;
        case EnemyType::SKELETON:
            baseHealth = 20.0f;
            break;
        case EnemyType::BOSS:
            baseHealth = 400.0f;
            break;
        case EnemyType::TANK:
            baseHealth = 100.0f;
            break;
        case EnemyType::GHOST:
            baseHealth = 10.0f;
            break;
        }
        float healthMultiplier = 1.0f;
        if (currentRound > 1)
        {
            healthMultiplier = 1.0f + ((currentRound - 1) * 0.1f);
        }
        maxHealth = baseHealth * healthMultiplier;
        health = maxHealth;
    }
};

// Game settings
const float SPAWN_INTERVAL = 0.7f;   // Seconds between enemy spawns
const int MAX_ENEMIES = 20;          // Maximum number of enemies at once
const float ZOMBIE_SPEED = 60.0f;    // Zombie speed (reduced from 100.0f)
const float SKELETON_SPEED = 120.0f; // Skeleton speed (reduced from 200.0f)
const float BOSS_SPEED = 20.0f;      // Boss speed (slower, reduced from 40.0f)
const float TANK_SPEED = 30.0f;      // Tank speed (slowest)
const float GHOST_SPEED = 160.0f;    // Ghost speed (fastest)

// Bean rewards for killing enemies
const int ZOMBIE_BEANS = 1;   // Beans for killing a zombie
const int SKELETON_BEANS = 2; // Beans for killing a skeleton (more beans as they're faster)
const int BOSS_BEANS = 30;    // Beans for killing a boss (significantly more)
const int TANK_BEANS = 5;     // Beans for killing a tank (more than zombie but less than boss)
const int GHOST_BEANS = 3;    // Beans for killing a ghost (more than skeleton)

// Function to get enemy color based on type
Color getEnemyColor(EnemyType type)
{
    switch (type)
    {
        case EnemyType::SKELETON:
            return Color(0.0f, 0.0f, 1.0f, 1.0f); // Blue for Skeleton
        case EnemyType::BOSS:
            return Color(0.5f, 0.0f, 0.5f, 1.0f); // Purple for Boss
        case EnemyType::TANK:
            return Color(0.5f, 0.5f, 0.0f, 1.0f); // Yellow for Tank
        case EnemyType::GHOST:
            return Color(0.8f, 0.8f, 0.8f, 0.8f); // Semi-transparent white for Ghost
        case EnemyType::ZOMBIE:
        default:
            return Color(1.0f, 0.0f, 0.0f, 1.0f); // Red for Zombie
    }
}

// Function to get enemy speed based on type
float getEnemySpeed(EnemyType type)
{
    // Base speeds in pixels per second
    float baseSpeed = 0.0f;
    
    // Get base speed for enemy type (in pixels per second)
    switch (type)
    {
        case EnemyType::SKELETON:
        baseSpeed = 80.0f; // fast
            break;
        case EnemyType::BOSS:
        baseSpeed = 25.0f; // very slow
            break;
        case EnemyType::TANK:
        baseSpeed = 35.0f; // slow
            break;
        case EnemyType::GHOST:
        baseSpeed = 120.0f; // very fast
            break;
        case EnemyType::ZOMBIE:
        default:
        baseSpeed = 50.0f; // normal
            break;
    }
    
    // Calculate speed multiplier based on round number
    float speedMultiplier = 1.0f;
    if (currentRound > 1)
    {
        // Increase speed by 8% per round
        speedMultiplier = 1.0f + ((currentRound - 1) * 0.08f);
    }
    
    // Return scaled speed
    return baseSpeed * speedMultiplier;
}

// Mouse position variables
double mouseX = 0.0, mouseY = 0.0;
bool mouseLeftPressed = false;

// Global TowerMenu for use in processInput
TowerMenu towerMenu;

// Tower that follows mouse cursor before placement
Tower placementTower;

// Function to process input
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        // Handle ESC key based on current screen
        if (currentScreen == GameScreen::GAME)
        {
            currentScreen = GameScreen::PAUSE_MENU;
        }
        else if (currentScreen == GameScreen::MAP_SELECT)
        {
            currentScreen = GameScreen::MAIN_MENU;
        }
        else if (currentScreen == GameScreen::OPTIONS)
        {
            currentScreen = GameScreen::MAIN_MENU;
        }
        else if (currentScreen == GameScreen::PAUSE_MENU)
        {
            currentScreen = GameScreen::GAME;
        }
    }
    
    // Get mouse position
    glfwGetCursorPos(window, &mouseX, &mouseY);
    
    // Get mouse button state
    if (!mouseJustPressed)
    { // Only check regular state if not just pressed
        mouseLeftPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    }
    
    // Tower type selection
    static bool key1Pressed = false;
    static bool key2Pressed = false;
    static bool key3Pressed = false;
    static bool key4Pressed = false;
    static bool key5Pressed = false;
    static bool key6Pressed = false;
    
    // Only allow tower selection via keyboard if the tower menu is closed and we're in the game
    if (!towerMenu.isOpen && currentScreen == GameScreen::GAME)
    {
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !key1Pressed)
        {
            placementTower.setType(TowerType::APPLE);
            std::cout << "Selected Apple Tower" << std::endl;
            key1Pressed = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE)
        {
            key1Pressed = false;
        }
        
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !key2Pressed)
        {
            placementTower.setType(TowerType::CARROT);
            std::cout << "Selected Carrot Tower" << std::endl;
            key2Pressed = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE)
        {
            key2Pressed = false;
        }
        
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !key3Pressed)
        {
            placementTower.setType(TowerType::POTATO);
            std::cout << "Selected Potato Tower" << std::endl;
            key3Pressed = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE)
        {
            key3Pressed = false;
        }
        
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !key4Pressed)
        {
            placementTower.setType(TowerType::PINEAPPLE);
            std::cout << "Selected Pineapple Tower" << std::endl;
            key4Pressed = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE)
        {
            key4Pressed = false;
        }
        
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS && !key5Pressed)
        {
            placementTower.setType(TowerType::BANANA_PEEL);
            std::cout << "Selected Banana Peel" << std::endl;
            key5Pressed = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_5) == GLFW_RELEASE)
        {
            key5Pressed = false;
        }
        
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS && !key6Pressed)
        {
            placementTower.setType(TowerType::CACTUS);
            std::cout << "Selected Cactus Tower" << std::endl;
            key6Pressed = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_6) == GLFW_RELEASE)
        {
            key6Pressed = false;
        }
    }
    else
    {
        // Still need to track key releases even if tower menu is open or not in game
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE)
            key1Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE)
            key2Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE)
            key3Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE)
            key4Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_RELEASE)
            key5Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_RELEASE)
            key6Pressed = false;
    }
}

// Mouse button callback
void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        mouseLeftPressed = true;
        mouseJustPressed = true; // Flag the initial press
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        mouseLeftPressed = false;
    }
}

// Function to update enemy position
void updateEnemy(Enemy &enemy, float deltaTime, const std::vector<Tower> &towers, int &beanCount, const std::vector<Point> &waypoints)
{
    if (!enemy.isActive || enemy.currentWaypoint >= waypoints.size() - 1)
        return;
    // Get current and next waypoint
    const Point &a = waypoints[enemy.currentWaypoint];
    const Point &b = waypoints[enemy.currentWaypoint + 1];
    float segLen = distance(a, b);
    if (segLen < 1e-4f)
    {
        enemy.currentWaypoint++;
        enemy.progress = 0.0f;
        return;
    }
    
    // Get base speed and adjust for window scaling
    float baseSpeed = getEnemySpeed(enemy.type);
    
    // Calculate the average segment length to normalize speed
    float totalPathLength = 0.0f;
    for (size_t i = 0; i < waypoints.size() - 1; i++)
    {
        totalPathLength += distance(waypoints[i], waypoints[i + 1]);
    }
    float averageSegmentLength = totalPathLength / (waypoints.size() - 1);
    
    // Adjust speed based on segment length relative to average
    // Longer segments should have slower movement
    float segmentRatio = segLen / averageSegmentLength;
    float speedMultiplier = 1.0f / std::sqrt(segmentRatio);
    float adjustedSpeed = baseSpeed * speedMultiplier;
    
    float moveDist = adjustedSpeed * deltaTime;
    float moveProgress = moveDist / segLen;
    enemy.progress += moveProgress;
    
    while (enemy.progress >= 1.0f && enemy.currentWaypoint < waypoints.size() - 2)
    {
        enemy.currentWaypoint++;
        enemy.progress -= 1.0f;
        // update a, b, segLen for next segment
        if (enemy.currentWaypoint < waypoints.size() - 1)
        {
            const Point &a2 = waypoints[enemy.currentWaypoint];
            const Point &b2 = waypoints[enemy.currentWaypoint + 1];
            segLen = distance(a2, b2);
            if (segLen < 1e-4f)
                break;
            // Recalculate speed multiplier for new segment
            segmentRatio = segLen / averageSegmentLength;
            speedMultiplier = 1.0f / std::sqrt(segmentRatio);
            adjustedSpeed = baseSpeed * speedMultiplier;
        }
    }
    if (enemy.progress >= 1.0f)
    {
        enemy.currentWaypoint++;
        enemy.progress = 0.0f;
    }
    if (enemy.currentWaypoint >= waypoints.size() - 1)
    {
        enemy.isActive = false;
        lives--;
        if (lives <= 0)
        {
            isGameOver = true;
            std::cout << "GAME OVER! Enemies reached the exit!" << std::endl;
        }
        else
        {
            std::cout << "Enemy escaped! Lives remaining: " << lives << std::endl;
        }
        return;
    }
    // Compute actual position for collision checks
    Point pos = enemy.getPosition(waypoints);
    for (const auto &tower : towers)
    {
        if (tower.type == TowerType::BANANA_PEEL && !tower.isUsed)
        {
            if (distance(pos, tower.pos) < ENEMY_SIZE / 2)
            {
                if (enemy.type != EnemyType::GHOST)
                {
                    enemy.health -= tower.damage;
                    Tower &actualTower = const_cast<Tower &>(tower);
                    actualTower.isUsed = true;
                    if (enemy.health <= 0)
                    {
                        enemy.isActive = false;
                        beanCount += (enemy.type == EnemyType::ZOMBIE) ? ZOMBIE_BEANS : (enemy.type == EnemyType::SKELETON) ? SKELETON_BEANS
                                                                                    : (enemy.type == EnemyType::TANK)       ? TANK_BEANS
                                                                                                                            : BOSS_BEANS;
                    }
                }
            }
        }
        else if (tower.type == TowerType::CACTUS && tower.usesLeft != 0)
        {
            if (distance(pos, tower.pos) < ENEMY_SIZE / 2)
            {
                if (enemy.type != EnemyType::GHOST)
                {
                    enemy.health -= tower.damage * deltaTime;
                    if (enemy.health <= 0)
                    {
                        enemy.isActive = false;
                        beanCount += (enemy.type == EnemyType::ZOMBIE) ? ZOMBIE_BEANS : (enemy.type == EnemyType::SKELETON) ? SKELETON_BEANS
                                                                                    : (enemy.type == EnemyType::TANK)       ? TANK_BEANS
                                                                                                                            : BOSS_BEANS;
                    }
                }
            }
        }
    }
}

// Function to check if any part of a tower overlaps with the path or water
bool canPlaceTower(const Point &center, float size, TowerType type, const std::vector<Point> &waypoints)
{
    if (center.x > GAME_WIDTH - size / 2)
    {
        return false;
    }
    if (type == TowerType::BANANA_PEEL || type == TowerType::CACTUS)
    {
        return isNearPath(center, waypoints, 20.0f);
    }
    float halfSize = size / 2.0f;
    float quarterSize = size / 4.0f;
    std::vector<Point> checkPoints = {
        center,
        Point(center.x - halfSize, center.y - halfSize),
        Point(center.x + halfSize, center.y - halfSize),
        Point(center.x - halfSize, center.y + halfSize),
        Point(center.x + halfSize, center.y + halfSize),
        Point(center.x - halfSize, center.y),
        Point(center.x + halfSize, center.y),
        Point(center.x, center.y - halfSize),
        Point(center.x, center.y + halfSize),
        Point(center.x - halfSize, center.y - quarterSize),
        Point(center.x - halfSize, center.y + quarterSize),
        Point(center.x + halfSize, center.y - quarterSize),
        Point(center.x + halfSize, center.y + quarterSize),
        Point(center.x - quarterSize, center.y - halfSize),
        Point(center.x + quarterSize, center.y - halfSize),
        Point(center.x - quarterSize, center.y + halfSize),
        Point(center.x + quarterSize, center.y + halfSize)};
    for (const auto &point : checkPoints)
    {
        if (isInWaterRegion(point) || isNearPath(point, waypoints))
        {
            return false;
        }
    }
    return true;
}

// Initialize projectiles vector (add this after tower initialization)
std::vector<Projectile> projectiles(100); // Pool of projectiles

// Function to find closest enemy to a point
Enemy *findClosestEnemy(const Point &pos, float range, const std::vector<Enemy> &enemies, const std::vector<Point> &waypoints)
{
    Enemy *closest = nullptr;
    float minDist = range;
    for (const auto &enemy : enemies)
    {
        if (!enemy.isActive)
            continue;
        Point enemyPos = enemy.getPosition(waypoints);
        float dist = distance(pos, enemyPos);
        if (dist < minDist)
        {
            minDist = dist;
            closest = const_cast<Enemy *>(&enemy);
        }
    }
    return closest;
}

// Function to spawn a projectile
void spawnProjectile(std::vector<Projectile> &projectiles, const Point &start, const Point &target, const Tower &tower)
{
    for (auto &proj : projectiles)
    {
        if (!proj.active)
        {
            proj.active = true;
            proj.pos = start;
            proj.damage = tower.damage;
            proj.speed = tower.projectileSpeed;
            proj.distanceTraveled = 0.0f;          // Reset distance traveled
            proj.maxDistance = tower.range * 1.2f; // Set max distance based on tower range
            proj.sourceType = tower.type;          // Set the source tower type
            
            // Calculate direction to target
            float dx = target.x - start.x;
            float dy = target.y - start.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            // Normalize direction
            if (dist > 0)
            {
                proj.velocity.x = dx / dist;
                proj.velocity.y = dy / dist;
            }
            
            return;
        }
    }
}

// Function to spawn projectiles in 8 directions
void spawnProjectilesInAllDirections(std::vector<Projectile> &projectiles, const Point &start, const Tower &tower)
{
    // 8 directions: N, NE, E, SE, S, SW, W, NW
    const int numDirections = 8;
    const float angleStep = 2.0f * 3.14159f / numDirections; // 45 degrees in radians
    
    for (int i = 0; i < numDirections; i++)
    {
        float angle = i * angleStep;
        float dx = cos(angle);
        float dy = sin(angle);
        
        // Find available projectile
        for (auto &proj : projectiles)
        {
            if (!proj.active)
            {
                proj.active = true;
                proj.pos = start;
                proj.damage = tower.damage;
                proj.speed = tower.projectileSpeed;
                proj.distanceTraveled = 0.0f;          // Reset distance traveled
                proj.maxDistance = tower.range * 1.2f; // Set max distance based on tower range
                proj.sourceType = tower.type;          // Set the source tower type
                proj.velocity.x = dx;
                proj.velocity.y = dy;
                break;
            }
        }
    }
}

// Function to check if point is inside rectangle
bool isPointInRect(float px, float py, const Rectangle &rect)
{
    return px >= rect.x && px <= rect.x + rect.w &&
           py >= rect.y && py <= rect.y + rect.h;
}

// Function to get tower type name
std::string getTowerTypeName(TowerType type)
{
    switch (type)
    {
    case TowerType::NONE:
        return "None";
    case TowerType::APPLE:
        return "Apple Tower";
    case TowerType::CARROT:
        return "Carrot Tower";
    case TowerType::POTATO:
        return "Potato Tower";
    case TowerType::PINEAPPLE:
        return "Pineapple Tower";
    case TowerType::BANANA_PEEL:
        return "Banana Peel";
    case TowerType::CACTUS:
        return "Cactus Tower";
    default:
        return "Unknown";
    }
}

// Function to get tower cost
int getTowerCost(TowerType type)
{
    float costMultiplier = 1.0f;
    int baseCost = 0;
    
    switch (type)
    {
        case TowerType::APPLE:
            baseCost = BASE_APPLE_COST;
            costMultiplier = pow(1.2f, appleTowerCount);
            break;
        case TowerType::CARROT:
            baseCost = BASE_CARROT_COST;
            costMultiplier = pow(1.2f, carrotTowerCount);
            break;
        case TowerType::POTATO:
            baseCost = BASE_POTATO_COST;
            costMultiplier = pow(1.2f, potatoTowerCount);
            break;
        case TowerType::PINEAPPLE:
            baseCost = BASE_PINEAPPLE_COST;
            costMultiplier = pow(1.2f, pineappleTowerCount);
            break;
        case TowerType::BANANA_PEEL:
            baseCost = BASE_BANANA_PEEL_COST;
            costMultiplier = pow(1.2f, bananaPeelTowerCount);
            break;
        case TowerType::CACTUS:
            baseCost = BASE_CACTUS_COST;
            costMultiplier = pow(1.2f, cactusTowerCount);
            break;
        default:
            return 0;
    }
    
    return int(baseCost * costMultiplier);
}

// Function to get tower stats as string
std::string getTowerStats(TowerType type)
{
    Tower temp;
    temp.setType(type);
    return "Cost: " + std::to_string(getTowerCost(type)) + " beans" +
           "\nRange: " + std::to_string(int(temp.range)) +
           "\nDamage: " + std::to_string(int(temp.damage)) +
           "\nSpeed: " + std::to_string(temp.attackSpeed) + "/s";
}

// Function to draw a digit (0-9) using rectangles
void drawDigit(gl2d::Renderer2D &renderer, int digit, float x, float y, float size)
{
    const float thickness = size / 5.0f;
    const float gap = thickness / 2.0f;

    // Segments for each number (1 = draw, 0 = don't draw)
    // Order: top, top-right, bottom-right, bottom, bottom-left, top-left, middle
    const bool segments[10][7] = {
        {1, 1, 1, 1, 1, 1, 0}, // 0
        {0, 1, 1, 0, 0, 0, 0}, // 1
        {1, 1, 0, 1, 1, 0, 1}, // 2
        {1, 1, 1, 1, 0, 0, 1}, // 3
        {0, 1, 1, 0, 0, 1, 1}, // 4
        {1, 0, 1, 1, 0, 1, 1}, // 5
        {1, 0, 1, 1, 1, 1, 1}, // 6
        {1, 1, 1, 0, 0, 0, 0}, // 7
        {1, 1, 1, 1, 1, 1, 1}, // 8
        {1, 1, 1, 1, 0, 1, 1}  // 9
    };

    // Draw segments based on the digit
    if (digit >= 0 && digit <= 9)
    {
        // Top
        if (segments[digit][0])
            renderer.renderRectangle(
                {x + gap, y, size - 2 * gap, thickness},
                {1.0f, 1.0f, 1.0f, 1.0f});
        // Top-right
        if (segments[digit][1])
            renderer.renderRectangle(
                {x + size - thickness, y + gap, thickness, size / 2 - gap},
                {1.0f, 1.0f, 1.0f, 1.0f});
        // Bottom-right
        if (segments[digit][2])
            renderer.renderRectangle(
                {x + size - thickness, y + size / 2, thickness, size / 2 - gap},
                {1.0f, 1.0f, 1.0f, 1.0f});
        // Bottom
        if (segments[digit][3])
            renderer.renderRectangle(
                {x + gap, y + size - thickness, size - 2 * gap, thickness},
                {1.0f, 1.0f, 1.0f, 1.0f});
        // Bottom-left
        if (segments[digit][4])
            renderer.renderRectangle(
                {x, y + size / 2, thickness, size / 2 - gap},
                {1.0f, 1.0f, 1.0f, 1.0f});
        // Top-left
        if (segments[digit][5])
            renderer.renderRectangle(
                {x, y + gap, thickness, size / 2 - gap},
                {1.0f, 1.0f, 1.0f, 1.0f});
        // Middle
        if (segments[digit][6])
            renderer.renderRectangle(
                {x + gap, y + size / 2 - thickness / 2, size - 2 * gap, thickness},
                {1.0f, 1.0f, 1.0f, 1.0f});
    }
}

// Function to draw a number using digits
void drawNumber(gl2d::Renderer2D &renderer, int number, float x, float y, float digitSize)
{
    std::string numStr = std::to_string(number);
    float spacing = digitSize * 1.2f; // Add some space between digits
    
    // Draw background
    float bgWidth = spacing * numStr.length() + digitSize;
    renderer.renderRectangle(
        {x - 10, y - 5, bgWidth + 20, digitSize + 10},
        {0.0f, 0.0f, 0.0f, 0.5f});

    // Draw bean icon
    renderer.renderRectangle(
        {x, y + digitSize / 4, digitSize / 2, digitSize / 2},
        {0.6f, 0.4f, 0.2f, 1.0f});

    // Draw each digit
    for (size_t i = 0; i < numStr.length(); i++)
    {
        drawDigit(renderer, numStr[i] - '0', x + digitSize + i * spacing, y, digitSize);
    }
}

// Function to get tower color based on type
Color getTowerColor(TowerType type)
{
    switch (type)
    {
    case TowerType::APPLE:
        return APPLE_TOWER_COLOR;
    case TowerType::CARROT:
        return CARROT_TOWER_COLOR;
    case TowerType::POTATO:
        return POTATO_TOWER_COLOR;
    case TowerType::PINEAPPLE:
        return PINEAPPLE_TOWER_COLOR;
    case TowerType::BANANA_PEEL:
        return BANANA_PEEL_COLOR;
    case TowerType::CACTUS:
        return CACTUS_TOWER_COLOR;
    default:
        return APPLE_TOWER_COLOR;
    }
}

// Function to calculate number of enemies for a round
int getEnemiesForRound(int round)
{
    // Start with 5 enemies in round 1, increase by 2 each round
    return 5 + (round - 1) * 2;
}

// Function to determine skeleton percentage for a round
float getSkeletonPercentage(int round)
{
    // Start with 0% skeletons, increase by 10% each round, max 70%
    float percentage = (round - 1) * 0.1f;
    return std::min(percentage, 0.7f);
}

// Function to determine tank percentage for a round
float getTankPercentage(int round)
{
    // Start with 0% tanks, increase by 5% each round, max 30%
    float percentage = (round - 1) * 0.05f;
    return std::min(percentage, 0.3f);
}

// Function to check if the round is a boss round
bool isBossRound(int round)
{
    // Boss appears every 10 rounds (rounds 10, 20, 30, etc.)
    return round % 10 == 0 && round > 0;
}

// Function to start a new round
void startNewRound(std::vector<Enemy> &enemies, int &round)
{
    round++;
    enemiesLeftInRound = getEnemiesForRound(round);
    
    // Add one extra enemy for the boss on boss rounds
    if (isBossRound(round))
    {
        enemiesLeftInRound += 1;
    }
    
    isRoundActive = true;
    enemySpawnTimer = 0.0f;
    
    std::cout << "Round " << round << " starting! Enemies: " << enemiesLeftInRound;
    if (isBossRound(round))
    {
        std::cout << " (including a BOSS!)";
    }
    std::cout << std::endl;
}

// Function to check if round is complete
bool isRoundComplete(const std::vector<Enemy> &enemies)
{
    // Round is complete when all enemies have been spawned and none are active
    if (enemiesLeftInRound > 0)
    {
        return false;
    }
    
    for (const auto &enemy : enemies)
    {
        if (enemy.isActive)
        {
            return false;
        }
    }
    
    return true;
}

// Function to reset the game state
void resetGame(std::vector<Enemy> &enemies, std::vector<Tower> &towers, std::vector<Projectile> &projectiles, int &beanCount)
{
    // Reset enemies
    for (auto &enemy : enemies)
    {
        enemy.isActive = false;
    }
    
    // Clear all towers
    towers.clear();
    
    // Clear all projectiles
    for (auto &proj : projectiles)
    {
        proj.active = false;
    }
    
    // Reset tower counts
    appleTowerCount = 0;
    carrotTowerCount = 0;
    potatoTowerCount = 0;
    pineappleTowerCount = 0;
    bananaPeelTowerCount = 0;
    cactusTowerCount = 0;
    
    // Reset round variables
    currentRound = 0;
    enemiesLeftInRound = 0;
    roundStartTimer = 5.0f;
    isRoundActive = false;
    
    // Reset game state
    isGameOver = false;
    lives = 3;
    
    // Reset beans
    beanCount = 69;
    
    std::cout << "Game reset! Starting new game..." << std::endl;
}

// Modify the drawing code for cactus
void renderCactus(gl2d::Renderer2D &renderer, const Tower &tower, float scaleX, float scaleY)
{
    Color towerColor = getTowerColor(tower.type);
    float cactusSize = TOWER_SIZE * 0.8f;
    float drawX = tower.pos.x * scaleX;
    float drawY = tower.pos.y * scaleY;
    float drawCactusSize = cactusSize * ((scaleX + scaleY) / 2.0f);
    
    // Draw main green cactus body
    renderer.renderRectangle(
        {drawX - drawCactusSize / 4, drawY - drawCactusSize / 2, drawCactusSize / 2, drawCactusSize},
        {towerColor.r, towerColor.g, towerColor.b, towerColor.a});
    
    // Draw a smaller arm on the side
    renderer.renderRectangle(
        {drawX - drawCactusSize / 4, drawY - drawCactusSize / 4, drawCactusSize / 2, drawCactusSize / 3},
        {towerColor.r, towerColor.g, towerColor.b, towerColor.a});
    
    // Draw some spines/spikes using small rectangles
    Color spikeColor(1.0f, 1.0f, 1.0f, 0.9f);
    
    // Always draw 5 spikes since cactus is permanent
    for (int i = 0; i < 5; i++)
    {
        float spikeX = drawX + ((i % 2) * 2 - 1) * drawCactusSize / 3;
        float spikeY = drawY - drawCactusSize / 2 + (i / 2) * drawCactusSize / 2;
        renderer.renderRectangle(
            {spikeX, spikeY, 3 * scaleX, 3 * scaleY},
            {spikeColor.r, spikeColor.g, spikeColor.b, spikeColor.a});
    }
}

// Function to determine ghost percentage for a round
float getGhostPercentage(int round)
{
    // Start with 0% ghosts, increase by 7% each round, max 25%
    float percentage = (round - 1) * 0.07f;
    return std::min(percentage, 0.25f);
}

// Function to get upgrade description based on tower type and upgrade type
std::string getUpgradeDescription(TowerType type, const std::string &upgradeType, int level)
{
    if (type == TowerType::BANANA_PEEL)
        return "No upgrades available";

    if (upgradeType == "damage")
    {
        return "Damage Upgrade: +30% (Level " + std::to_string(level) + "/3)";
    }
    else if (upgradeType == "attackSpeed")
    {
        return "Attack Speed Upgrade: +20% (Level " + std::to_string(level) + "/3)";
    }
    else if (upgradeType == "range")
    {
        return "Range Upgrade: +15% (Level " + std::to_string(level) + "/3)";
    }
    return "";
}

// Game constants are already defined earlier in the file

// Tower textures
gl2d::Texture appleTowerTexture;
gl2d::Texture carrotTowerTexture;
gl2d::Texture potatoTowerTexture;
gl2d::Texture pineappleTowerTexture;
gl2d::Texture bananaPeelTexture;
gl2d::Texture cactusTexture;

// Global textures
gl2d::Texture appleTexture;
gl2d::Texture carrotTexture;
gl2d::Texture potatoTexture;
gl2d::Texture pineappleTexture;

// Enemy textures
gl2d::Texture zombieTexture;
gl2d::Texture skeletonTexture;
gl2d::Texture bossTexture;
gl2d::Texture tankTexture;
gl2d::Texture ghostTexture;

// Add global texture for heart icon
gl2d::Texture heartTexture;

// Function to create simple colored texture if it doesn't exist
void createSimpleTexture(const std::string &filename, const Color &color)
{
    // Only create if file doesn't exist
    if (std::filesystem::exists(filename))
    {
        return;
    }
    
    // Create a simple 32x32 colored image
    const int width = 32;
    const int height = 32;
    std::vector<unsigned char> pixels(width * height * 4, 0);
    
    // Fill with color, making a circular shape
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int index = (y * width + x) * 4;
            
            // Calculate distance from center for circular shape
            float dx = x - width / 2;
            float dy = y - height / 2;
            float distance = std::sqrt(dx * dx + dy * dy);
            
            // If within radius, set pixel color
            if (distance <= width / 2)
            {
                pixels[index] = static_cast<unsigned char>(color.r * 255);
                pixels[index + 1] = static_cast<unsigned char>(color.g * 255);
                pixels[index + 2] = static_cast<unsigned char>(color.b * 255);
                pixels[index + 3] = static_cast<unsigned char>(color.a * 255);
            }
        }
    }
}

// Function to create all projectile textures
void createProjectileTextures()
{
    // Ensure resources directory exists
    if (!std::filesystem::exists("resources"))
    {
        std::filesystem::create_directory("resources");
    }

    // Create projectile textures if they don't exist
    createSimpleTexture("resources/apple.png", Color(1.0f, 0.2f, 0.2f, 1.0f));     // Red apple
    createSimpleTexture("resources/carrot.png", Color(1.0f, 0.5f, 0.0f, 1.0f));    // Orange carrot
    createSimpleTexture("resources/potato.png", Color(0.6f, 0.4f, 0.2f, 1.0f));    // Brown potato
    createSimpleTexture("resources/pineapple.png", Color(0.8f, 0.8f, 0.0f, 1.0f)); // Yellow pineapple
}

// Function to create all enemy textures
void createEnemyTextures()
{
    // Ensure resources directory exists
    if (!std::filesystem::exists("resources"))
    {
        std::filesystem::create_directory("resources");
    }

    // Create enemy textures if they don't exist
    createSimpleTexture("resources/zombie.png", getEnemyColor(EnemyType::ZOMBIE));
    createSimpleTexture("resources/skeleton.png", getEnemyColor(EnemyType::SKELETON));
    createSimpleTexture("resources/boss.png", getEnemyColor(EnemyType::BOSS));
    createSimpleTexture("resources/tank.png", getEnemyColor(EnemyType::TANK));
    createSimpleTexture("resources/ghost.png", getEnemyColor(EnemyType::GHOST));
}

std::map<char, gl2d::Texture> alphabetTextures;

void loadAlphabetTextures()
{
    for (char c = 'A'; c <= 'Z'; ++c)
    {
        std::string path = "resources/alphabet/";
        path += c;
        path += ".png";
        gl2d::Texture tex;
        tex.loadFromFile(path.c_str());
        alphabetTextures[c] = tex;
    }
}

void drawText(gl2d::Renderer2D &renderer, const std::string &text, float x, float y, float size, float spacing = 2.0f, float scale = 1.0f)
{
    float scaledSize = size * scale;
    float scaledSpacing = spacing * scale;
    float cursorX = x;
    for (char c : text)
    {
        if (c == ' ')
        {
            cursorX += scaledSize * 0.6f; // Space width
            continue;
        }
        char upper = std::toupper(static_cast<unsigned char>(c));
        auto it = alphabetTextures.find(upper);
        if (it != alphabetTextures.end() && it->second.id != 0)
        {
            renderer.renderRectangle(
                {cursorX, y, scaledSize, scaledSize},
                it->second, {1, 1, 1, 1});
        }
        cursorX += scaledSize + scaledSpacing;
    }
}

// Add enum for map selection
enum class MapType
{
    GRASS,
    DESERT,
    PLACEHOLDER
};
MapType selectedMap = MapType::GRASS;

// Add menu button rectangles
Rectangle playButton;
Rectangle optionsButton;
Rectangle exitButton;

// ... inside main() after loading background textures ...
    // Placeholder: use a solid color for now
    gl2d::Texture placeholderTexture;

gl2d::Texture backgroundDesertTexture;

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return -1;
    }

    // Create a window
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Tower Defense Game", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create window!" << std::endl;
        glfwTerminate();
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return -1;
    }

    // Make the OpenGL context current
    glfwMakeContextCurrent(window);
    
    // Set mouse button callback
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Initialize GLAD (loads OpenGL functions)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return -1;
    }

    // Initialize GL2D
    gl2d::init();

    // Create GL2D renderer
    gl2d::Renderer2D renderer;
    renderer.create();

    // Load alphabet textures for text rendering
    loadAlphabetTextures();

    // Load background texture
    gl2d::Texture backgroundTexture;
    
    // Check if file exists
    if (!std::filesystem::exists("resources/background.png"))
    {
        std::cerr << "ERROR: background.png not found in resources folder!" << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return -1;
    }
    
    backgroundTexture.loadFromFile("resources/background.png");

    if (backgroundTexture.id == 0)
    {
        std::cerr << "ERROR: Failed to load background texture!" << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return -1;
    }

    std::cout << "Successfully loaded background texture!" << std::endl;

    // Load background desert texture
    std::string backgroundDesertPath = "resources/backgroundDesert.png";
    backgroundDesertTexture.loadFromFile(backgroundDesertPath.c_str());
    if (backgroundDesertTexture.id == 0)
    {
        backgroundDesertPath = "../resources/backgroundDesert.png";
        backgroundDesertTexture.loadFromFile(backgroundDesertPath.c_str());
    }
    
    // Create projectile textures if they don't exist
    createSimpleTexture("resources/apple.png", Color(1.0f, 0.2f, 0.2f, 1.0f));     // Red apple
    createSimpleTexture("resources/carrot.png", Color(1.0f, 0.5f, 0.0f, 1.0f));    // Orange carrot
    createSimpleTexture("resources/potato.png", Color(0.6f, 0.4f, 0.2f, 1.0f));    // Brown potato
    createSimpleTexture("resources/pineapple.png", Color(0.8f, 0.8f, 0.0f, 1.0f)); // Yellow pineapple
    
    // Load projectile textures
    std::string applePath = "resources/apple.png";
    appleTexture.loadFromFile(applePath.c_str());
    if (appleTexture.id == 0)
    {
        applePath = "../resources/apple.png";
        appleTexture.loadFromFile(applePath.c_str());
    }
    
    std::string carrotPath = "resources/carrot.png";
    carrotTexture.loadFromFile(carrotPath.c_str());
    if (carrotTexture.id == 0)
    {
        carrotPath = "../resources/carrot.png";
        carrotTexture.loadFromFile(carrotPath.c_str());
    }

    std::string carrotTowerPath = "resources/carrotTower.png";
    carrotTowerTexture.loadFromFile(carrotTowerPath.c_str());
    if (carrotTowerTexture.id == 0)
    {
        carrotTowerPath = "../resources/carrotTower.png";
        carrotTowerTexture.loadFromFile(carrotTowerPath.c_str());
    }
    
    std::string appleTowerPath = "resources/appleTower.png";
    appleTowerTexture.loadFromFile(appleTowerPath.c_str());
    if (appleTowerTexture.id == 0)
    {
        appleTowerPath = "../resources/appleTower.png";
        appleTowerTexture.loadFromFile(appleTowerPath.c_str());
    }

    std::string bananaPeelPath = "resources/bananaPeel.png";
    bananaPeelTexture.loadFromFile(bananaPeelPath.c_str());
    if (bananaPeelTexture.id == 0)
    {
        bananaPeelPath = "../resources/bananaPeel.png";
        bananaPeelTexture.loadFromFile(bananaPeelPath.c_str());
    }

    std::string cactusTowerPath = "resources/cactus.png";
    cactusTexture.loadFromFile(cactusTowerPath.c_str());
    if (cactusTexture.id == 0)
    {
        cactusTowerPath = "../resources/cactus.png";
        cactusTexture.loadFromFile(cactusTowerPath.c_str());
    }

    std::string pineappleTowerPath = "resources/pineappleTower.png";
    pineappleTowerTexture.loadFromFile(pineappleTowerPath.c_str());
    if (pineappleTowerTexture.id == 0)
    {
        pineappleTowerPath = "../resources/pineappleTower.png";
        pineappleTowerTexture.loadFromFile(pineappleTowerPath.c_str());
    }

    std::string potatoTowerPath = "resources/potatoTower.png";
    potatoTowerTexture.loadFromFile(potatoTowerPath.c_str());
    if (potatoTowerTexture.id == 0)
    {
        potatoTowerPath = "../resources/potatoTower.png";
        potatoTowerTexture.loadFromFile(potatoTowerPath.c_str());
    }
    std::string potatoPath = "resources/potato.png";
    potatoTexture.loadFromFile(potatoPath.c_str());
    if (potatoTexture.id == 0)
    {
        potatoPath = "../resources/potato.png";
        potatoTexture.loadFromFile(potatoPath.c_str());
    }
    
    std::string pineapplePath = "resources/pineapple.png";
    pineappleTexture.loadFromFile(pineapplePath.c_str());
    if (pineappleTexture.id == 0)
    {
        pineapplePath = "../resources/pineapple.png";
        pineappleTexture.loadFromFile(pineapplePath.c_str());
    }
    
    // Load enemy textures
    std::string zombiePath = "resources/zombie.png";
    zombieTexture.loadFromFile(zombiePath.c_str());
    if (zombieTexture.id == 0)
    {
        zombiePath = "../resources/zombie.png";
        zombieTexture.loadFromFile(zombiePath.c_str());
    }
    
    std::string skeletonPath = "resources/skeleton.png";
    skeletonTexture.loadFromFile(skeletonPath.c_str());
    if (skeletonTexture.id == 0)
    {
        skeletonPath = "../resources/skeleton.png";
        skeletonTexture.loadFromFile(skeletonPath.c_str());
    }
    
    std::string bossPath = "resources/boss.png";
    bossTexture.loadFromFile(bossPath.c_str());
    if (bossTexture.id == 0)
    {
        bossPath = "../resources/boss.png";
        bossTexture.loadFromFile(bossPath.c_str());
    }
    
    std::string tankPath = "resources/tank.png";
    tankTexture.loadFromFile(tankPath.c_str());
    if (tankTexture.id == 0)
    {
        tankPath = "../resources/tank.png";
        tankTexture.loadFromFile(tankPath.c_str());
    }
    
    std::string ghostPath = "resources/ghost.png";
    ghostTexture.loadFromFile(ghostPath.c_str());
    if (ghostTexture.id == 0)
    {
        ghostPath = "../resources/ghost.png";
        ghostTexture.loadFromFile(ghostPath.c_str());
    }

    std::string heartPath = "resources/heart.png";
    heartTexture.loadFromFile(heartPath.c_str());
    if (heartTexture.id == 0)
    {
        heartPath = "../resources/heart.png";
        heartTexture.loadFromFile(heartPath.c_str());
    }
    std::cout << "Heart texture ID: " << heartTexture.id << std::endl;

    // Initialize enemies
    std::vector<Enemy> enemies(MAX_ENEMIES);
    
    // Initialize towers
    std::vector<Tower> towers;

    // Initialize bean counter
    int beanCount = 69;

    // Initialize placement tower with no type selected
    placementTower.setType(TowerType::NONE);

    // Initialize tower menu
    TowerMenu towerMenu;

    // Create tower selection buttons (unscaled, will be scaled each frame)
    std::vector<TowerButton> towerButtons = {
        TowerButton(GAME_WIDTH + 20, 50, 160, 60, TowerType::APPLE),
        TowerButton(GAME_WIDTH + 20, 120, 160, 60, TowerType::CARROT),
        TowerButton(GAME_WIDTH + 20, 190, 160, 60, TowerType::POTATO),
        TowerButton(GAME_WIDTH + 20, 260, 160, 60, TowerType::PINEAPPLE),
        TowerButton(GAME_WIDTH + 20, 330, 160, 60, TowerType::BANANA_PEEL),
        TowerButton(GAME_WIDTH + 20, 400, 160, 60, TowerType::CACTUS)
    };

    // Menu and map selection button rectangles
    Rectangle playButton(WIDTH / 2 - 100, HEIGHT / 2 - 80, 200, 60);
    Rectangle optionsButton(WIDTH / 2 - 100, HEIGHT / 2, 200, 60);
    Rectangle exitButton(WIDTH / 2 - 100, HEIGHT / 2 + 80, 200, 60);
    Rectangle map1Button(WIDTH / 2 - 320, HEIGHT / 2 - 100, 180, 180);
    Rectangle map2Button(WIDTH / 2 - 90, HEIGHT / 2 - 100, 180, 180);
    Rectangle map3Button(WIDTH / 2 + 140, HEIGHT / 2 - 100, 180, 180);

    // Main game loop
    float lastTime = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        // Calculate delta time
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Update window metrics
        int w = 0, h = 0;
        glfwGetWindowSize(window, &w, &h);
        renderer.updateWindowMetrics(w, h);

        // Calculate scaling factors for responsiveness
        float scaleX = (float)w / (float)WIDTH;
        float scaleY = (float)h / (float)HEIGHT;

        // Remove scaling logic for tower and projectile positions
        // Update tower positions based on scaling
        // for (Tower &tower : towers) {
        //     tower.pos.x *= scaleX;
        //     tower.pos.y *= scaleY;
        // }

        // Update projectile positions based on scaling
        // for (Projectile &proj : projectiles) {
        //     proj.pos.x *= scaleX;
        //     proj.pos.y *= scaleY;
        // }

        float buttonScale = std::min(scaleX, scaleY);

        // Select background and waypoints based on current screen
        gl2d::Texture& currentBg = (selectedMap == MapType::DESERT) ? backgroundDesertTexture : backgroundTexture;
        std::vector<Point> currentWaypoints = (selectedMap == MapType::DESERT)
            ? desertWaypoints
            : waypoints;
        auto scaledWaypoints = [&]() {
            std::vector<Point> scaled;
            for (const auto &wp : currentWaypoints) {
                scaled.emplace_back(wp.x * scaleX, wp.y * scaleY);
            }
            return scaled;
        }();
        // Use currentBg and scaledWaypoints in all rendering and logic for GAME, using selectedMap for map selection

        // Update menu button rectangles for scaling
        Rectangle scaledPlayButton = playButton;
        Rectangle scaledOptionsButton = optionsButton;
        Rectangle scaledExitButton = exitButton;
        scaledPlayButton.x *= scaleX;
        scaledPlayButton.y *= scaleY;
        scaledPlayButton.w *= scaleX;
        scaledPlayButton.h *= scaleY;
        scaledOptionsButton.x *= scaleX;
        scaledOptionsButton.y *= scaleY;
        scaledOptionsButton.w *= scaleX;
        scaledOptionsButton.h *= scaleY;
        scaledExitButton.x *= scaleX;
        scaledExitButton.y *= scaleY;
        scaledExitButton.w *= scaleX;
        scaledExitButton.h *= scaleY;

        // Handle input
        processInput(window);

        // MAIN MENU SCREEN
        if (currentScreen == GameScreen::MAIN_MENU)
        {
            renderer.clearScreen({0.1, 0.2, 0.6, 1});
            // Draw title with dynamic scaling
            std::string title = "TOWER DEFENSE";
            float titleSize = 64.0f * scaleY;
            float titleWidth = title.length() * (titleSize + 2.0f);
            float titleX = (w - titleWidth) / 2.0f;
            float titleY = 80 * scaleY;
            drawText(renderer, title, titleX, titleY, titleSize, 2.0f, 1.0f);
            // Draw buttons
            auto drawMenuButton = [&](const Rectangle &rect, const char *label, bool hovered)
            {
                Color color = hovered ? Color(0.3f, 0.5f, 0.3f, 1.0f) : Color(0.2f, 0.2f, 0.2f, 1.0f);
                renderer.renderRectangle({rect.x, rect.y, rect.w, rect.h}, {color.r, color.g, color.b, color.a});
                float textSize = 32.0f * scaleY;
                float textX = rect.x + (rect.w - strlen(label) * textSize * 0.7f) / 2.0f;
                float textY = rect.y + (rect.h - textSize) / 2.0f;
                drawText(renderer, label, textX, textY, textSize, 2.0f, 1.0f);
            };
            bool playHovered = isPointInRect((float)mouseX, (float)mouseY, scaledPlayButton);
            bool optionsHovered = isPointInRect((float)mouseX, (float)mouseY, scaledOptionsButton);
            bool exitHovered = isPointInRect((float)mouseX, (float)mouseY, scaledExitButton);
            drawMenuButton(scaledPlayButton, "PLAY", playHovered);
            drawMenuButton(scaledOptionsButton, "OPTIONS", optionsHovered);
            drawMenuButton(scaledExitButton, "EXIT", exitHovered);
            renderer.flush();
            // Handle button clicks
            if (mouseJustPressed)
            {
                if (playHovered)
                {
                    currentScreen = GameScreen::MAP_SELECT;
                }
                else if (optionsHovered)
                {
                    currentScreen = GameScreen::OPTIONS;
                }
                else if (exitHovered)
                {
                    glfwSetWindowShouldClose(window, true);
                }
                mouseJustPressed = false;
            }
            // Poll events and skip game logic
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }
        // MAP SELECTION SCREEN
        if (currentScreen == GameScreen::MAP_SELECT)
        {
            renderer.clearScreen({0.1, 0.2, 0.6, 1});
            std::string selectText = "SELECT MAP";
            float selectSize = 48.0f * scaleY;
            float selectX = (w - selectText.length() * (selectSize + 2.0f)) / 2.0f;
            float selectY = 60 * scaleY;
            drawText(renderer, selectText, selectX, selectY, selectSize, 2.0f, 1.0f);
            // Scale map buttons
            Rectangle scaledMap1 = map1Button, scaledMap2 = map2Button, scaledMap3 = map3Button;
            scaledMap1.x *= scaleX;
            scaledMap1.y *= scaleY;
            scaledMap1.w *= scaleX;
            scaledMap1.h *= scaleY;
            scaledMap2.x *= scaleX;
            scaledMap2.y *= scaleY;
            scaledMap2.w *= scaleX;
            scaledMap2.h *= scaleY;
            scaledMap3.x *= scaleX;
            scaledMap3.y *= scaleY;
            scaledMap3.w *= scaleX;
            scaledMap3.h *= scaleY;
            // Draw map preview buttons
            bool map1Hovered = isPointInRect((float)mouseX, (float)mouseY, scaledMap1);
            bool map2Hovered = isPointInRect((float)mouseX, (float)mouseY, scaledMap2);
            bool map3Hovered = isPointInRect((float)mouseX, (float)mouseY, scaledMap3);
            renderer.renderRectangle({scaledMap1.x, scaledMap1.y, scaledMap1.w, scaledMap1.h}, currentBg, {1, 1, 1, map1Hovered ? 1.0f : 0.8f});
            renderer.renderRectangle({scaledMap2.x, scaledMap2.y, scaledMap2.w, scaledMap2.h}, backgroundDesertTexture, {1, 1, 1, map2Hovered ? 1.0f : 0.8f});
            renderer.renderRectangle({scaledMap3.x, scaledMap3.y, scaledMap3.w, scaledMap3.h}, {0.5f, 0.5f, 0.5f, map3Hovered ? 1.0f : 0.8f});
            // Draw map labels with dynamic scaling
            float labelSize = 24.0f * scaleY;
            drawText(renderer, "GRASS", scaledMap1.x + 20 * scaleX, scaledMap1.y + scaledMap1.h + 10 * scaleY, labelSize, 2.0f, 1.0f);
            drawText(renderer, "DESERT", scaledMap2.x + 20 * scaleX, scaledMap2.y + scaledMap2.h + 10 * scaleY, labelSize, 2.0f, 1.0f);
            drawText(renderer, "???", scaledMap3.x + 60 * scaleX, scaledMap3.y + scaledMap3.h + 10 * scaleY, labelSize, 2.0f, 1.0f);
            renderer.flush();
            // Handle map selection
            if (mouseJustPressed)
            {
                if (map1Hovered)
                {
                    selectedMap = MapType::GRASS;
                    currentScreen = GameScreen::GAME;
                }
                else if (map2Hovered)
                {
                    selectedMap = MapType::DESERT;
                    currentScreen = GameScreen::GAME;
                }
                else if (map3Hovered)
                {
                    selectedMap = MapType::PLACEHOLDER;
                    currentScreen = GameScreen::GAME;
                }
                mouseJustPressed = false;
            }
            // Back to main menu on ESC
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            {
                currentScreen = GameScreen::MAIN_MENU;
            }
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }
        // OPTIONS SCREEN (placeholder)
        if (currentScreen == GameScreen::OPTIONS)
        {
            renderer.clearScreen({0.1, 0.2, 0.6, 1});
            std::string opt = "OPTIONS (not implemented)";
            float optSize = 40.0f * scaleY;
            float optX = (w - opt.length() * (optSize + 2.0f)) / 2.0f;
            float optY = 200 * scaleY;
            drawText(renderer, opt, optX, optY, optSize, 2.0f, 1.0f);
            std::string back = "BACK";
            Rectangle backBtn(WIDTH / 2 - 80, HEIGHT - 120, 160, 50);
            backBtn.x *= scaleX;
            backBtn.y *= scaleY;
            backBtn.w *= scaleX;
            backBtn.h *= scaleY;
            bool backHovered = isPointInRect((float)mouseX, (float)mouseY, backBtn);
            renderer.renderRectangle({backBtn.x, backBtn.y, backBtn.w, backBtn.h}, {0.2f, 0.2f, 0.2f, 1.0f});
            float backSize = 28.0f * scaleY;
            float backX = backBtn.x + (backBtn.w - back.length() * backSize * 0.7f) / 2.0f;
            float backY = backBtn.y + (backBtn.h - backSize) / 2.0f;
            drawText(renderer, back, backX, backY, backSize, 2.0f, 1.0f);
            renderer.flush();
            if (mouseJustPressed && backHovered)
            {
                currentScreen = GameScreen::MAIN_MENU;
                mouseJustPressed = false;
            }
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }
        
        // PAUSE MENU SCREEN
        if (currentScreen == GameScreen::PAUSE_MENU)
        {
            // Draw the game in the background (frozen)
            renderer.clearScreen({0.1, 0.2, 0.6, 1});
            
            // Draw background
            renderer.renderRectangle({0 * scaleX, 0 * scaleY, GAME_WIDTH * scaleX, HEIGHT * scaleY}, currentBg, {1, 1, 1, 1});
            
            // Draw UI panel background
            renderer.renderRectangle(
                {(float)GAME_WIDTH * scaleX, 0 * scaleY, (float)PANEL_WIDTH * scaleX, (float)HEIGHT * scaleY},
                {UI_BACKGROUND.r, UI_BACKGROUND.g, UI_BACKGROUND.b, UI_BACKGROUND.a});
            
            // Draw bean count
            drawNumber(renderer, beanCount, (GAME_WIDTH - 180) * scaleX, 15 * scaleY, 32.0f * scaleY);
            
            // Draw round information
            float roundTextX = 70 * scaleX;
            float roundTextY = 15 * scaleY;
            float roundDigitSize = 24.0f * scaleY;
            float rSize = 30.0f * scaleY;
            float rX = roundTextX - 50 * scaleX;
            float rY = roundTextY;
            drawText(renderer, "R", rX, rY, rSize);
            std::string roundStr = std::to_string(currentRound);
            float bgWidth = roundDigitSize * 1.2f * roundStr.length() + 20 * scaleX;
            renderer.renderRectangle(
                {roundTextX - 5 * scaleX, roundTextY - 5 * scaleY, bgWidth, roundDigitSize + 10 * scaleY},
                {0.0f, 0.0f, 0.0f, 0.5f});
            for (size_t i = 0; i < roundStr.length(); i++)
            {
                drawDigit(renderer, roundStr[i] - '0', roundTextX + i * (roundDigitSize * 1.2f), roundTextY, roundDigitSize);
            }
            
            // Draw lives
            float livesX = 200 * scaleX;
            float livesY = 15 * scaleY;
            for (int i = 0; i < lives; i++)
            {
                renderer.renderRectangle(
                    {livesX + i * 30 * scaleX, livesY, 20 * scaleX, 20 * scaleY * (scaleY * 0.9f)},
                    heartTexture, {1.0f, 1.0f, 1.0f, 1.0f});
            }
            
            // Draw towers (frozen)
            for (const auto &tower : towers)
            {
                Color towerColor = getTowerColor(tower.type);
                float uniformScale = std::min(scaleX, scaleY);
                float drawX = tower.pos.x * scaleX;
                float drawY = tower.pos.y * scaleY;
                float drawSize = TOWER_SIZE * uniformScale;
                if (tower.type == TowerType::CACTUS)
                {
                    if (cactusTexture.id != 0)
                    {
                        renderer.renderRectangle(
                            {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                            cactusTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                    }
                    else
                    {
                        renderCactus(renderer, tower, scaleX, scaleY);
                    }
                }
                else if (tower.type == TowerType::CARROT && carrotTowerTexture.id != 0)
                {
                        renderer.renderRectangle(
                        {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                        carrotTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                    }
                else if (tower.type == TowerType::APPLE && appleTowerTexture.id != 0)
                {
                    renderer.renderRectangle(
                        {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                        appleTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                }
                else if (tower.type == TowerType::BANANA_PEEL && bananaPeelTexture.id != 0)
                {
                    renderer.renderRectangle(
                        {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                        bananaPeelTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                }
                else if (tower.type == TowerType::PINEAPPLE && pineappleTowerTexture.id != 0)
                {
                    renderer.renderRectangle(
                        {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                        pineappleTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                }
                else if (tower.type == TowerType::POTATO && potatoTowerTexture.id != 0)
                {
                    renderer.renderRectangle(
                        {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                        potatoTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                }
                else
                {
                    renderer.renderRectangle(
                        {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                        {towerColor.r, towerColor.g, towerColor.b, towerColor.a});
                }
            }
            
            // Draw enemies (frozen)
            for (const auto &enemy : enemies)
            {
                if (enemy.isActive)
                {
                    Color enemyColor = getEnemyColor(enemy.type);
                    float enemySize = 0.0f;
                    switch (enemy.type)
                    {
                    case EnemyType::BOSS:
                        enemySize = BOSS_SIZE * ((scaleX + scaleY) / 2.0f);
                        break;
                    case EnemyType::TANK:
                        enemySize = TANK_SIZE * ((scaleX + scaleY) / 2.0f);
                        break;
                    case EnemyType::GHOST:
                        enemySize = GHOST_SIZE * ((scaleX + scaleY) / 2.0f);
                        break;
                    default:
                        enemySize = ENEMY_SIZE * ((scaleX + scaleY) / 2.0f);
                        break;
                    }
                    Point pos = enemy.getPosition(scaledWaypoints);
                    renderer.renderRectangle(
                        {pos.x - enemySize / 2, pos.y - enemySize / 2, enemySize, enemySize},
                        {enemyColor.r, enemyColor.g, enemyColor.b, enemyColor.a});
                }
            }
            
            // Draw projectiles (frozen)
            for (const auto &proj : projectiles)
            {
                if (proj.active)
                {
                    float halfSize = PROJECTILE_SIZE / 2.0f;
                    renderer.renderRectangle(
                        {proj.pos.x - halfSize, proj.pos.y - halfSize, PROJECTILE_SIZE, PROJECTILE_SIZE},
                        {0.0f, 0.0f, 0.0f, 1.0f});
                }
            }
            
            // Draw semi-transparent overlay
            renderer.renderRectangle(
                {0, 0, (float)w, (float)h},
                {0.0f, 0.0f, 0.0f, 0.5f});
            
            // Draw pause menu title
            std::string pauseTitle = "PAUSED";
            float titleSize = 48.0f * scaleY;
            float titleWidth = pauseTitle.length() * (titleSize + 2.0f);
            float titleX = (w - titleWidth) / 2.0f;
            float titleY = 150 * scaleY;
            drawText(renderer, pauseTitle, titleX, titleY, titleSize, 2.0f, 1.0f);
            
            // Create pause menu buttons
            Rectangle resumeBtn(WIDTH / 2 - 100, HEIGHT / 2 - 80, 200, 60);
            Rectangle optionsBtn(WIDTH / 2 - 100, HEIGHT / 2, 200, 60);
            Rectangle mainMenuBtn(WIDTH / 2 - 100, HEIGHT / 2 + 80, 200, 60);
            
            // Scale buttons
            resumeBtn.x *= scaleX;
            resumeBtn.y *= scaleY;
            resumeBtn.w *= scaleX;
            resumeBtn.h *= scaleY;
            optionsBtn.x *= scaleX;
            optionsBtn.y *= scaleY;
            optionsBtn.w *= scaleX;
            optionsBtn.h *= scaleY;
            mainMenuBtn.x *= scaleX;
            mainMenuBtn.y *= scaleY;
            mainMenuBtn.w *= scaleX;
            mainMenuBtn.h *= scaleY;
            
            // Check hover states
            bool resumeHovered = isPointInRect((float)mouseX, (float)mouseY, resumeBtn);
            bool optionsHovered = isPointInRect((float)mouseX, (float)mouseY, optionsBtn);
            bool mainMenuHovered = isPointInRect((float)mouseX, (float)mouseY, mainMenuBtn);
            
            // Draw buttons
            auto drawPauseButton = [&](const Rectangle &rect, const char *label, bool hovered)
            {
                Color color = hovered ? Color(0.3f, 0.5f, 0.3f, 1.0f) : Color(0.2f, 0.2f, 0.2f, 1.0f);
                renderer.renderRectangle({rect.x, rect.y, rect.w, rect.h}, {color.r, color.g, color.b, color.a});
                float textSize = 28.0f * scaleY;
                float textX = rect.x + (rect.w - strlen(label) * textSize * 0.7f) / 2.0f;
                float textY = rect.y + (rect.h - textSize) / 2.0f;
                drawText(renderer, label, textX, textY, textSize, 2.0f, 1.0f);
            };
            
            drawPauseButton(resumeBtn, "RESUME", resumeHovered);
            drawPauseButton(optionsBtn, "OPTIONS", optionsHovered);
            drawPauseButton(mainMenuBtn, "MAIN MENU", mainMenuHovered);
            
            renderer.flush();
            
            // Handle button clicks
            if (mouseJustPressed)
            {
                if (resumeHovered)
                {
                    currentScreen = GameScreen::GAME;
                }
                else if (optionsHovered)
                {
                    currentScreen = GameScreen::OPTIONS;
                }
                else if (mainMenuHovered)
                {
                    resetGame(enemies, towers, projectiles, beanCount);
                    currentScreen = GameScreen::MAIN_MENU;
                }
                mouseJustPressed = false;
            }
            
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        // Only update game elements if we're in the game screen
        if (currentScreen == GameScreen::GAME && !isGameOver)
        {
            // Update placement tower position
            // Convert mouse position to game world coordinates for placement
            float mouseGameX = mouseX / scaleX;
            float mouseGameY = mouseY / scaleY;
            placementTower.pos = Point(mouseGameX, mouseGameY);
            
            // Place tower when mouse clicked (only on initial press)
            if ((mouseLeftPressed && mouseJustPressed) && !isGameOver)
            {
                Point clickPos(mouseGameX, mouseGameY);
                
                // Check if clicked on tower menu buttons when menu is open
                if (towerMenu.isOpen)
                {
                    bool buttonClicked = false;
                    
                    if (isPointInRect((float)mouseX, (float)mouseY, towerMenu.closeButton))
                    {
                        towerMenu.close();
                        buttonClicked = true;
                    }
                    else if (isPointInRect((float)mouseX, (float)mouseY, towerMenu.upgradeButton1))
                    {
                        // Check if tower can be upgraded and player has enough beans
                        int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, towerMenu.selectedTower->damageUpgradeLevel);
                        if (upgradeCost > 0 && beanCount >= upgradeCost)
                        {
                            beanCount -= upgradeCost;
                            towerMenu.selectedTower->upgradeDamage();
                            std::cout << "Upgraded tower! New damage level: " << towerMenu.selectedTower->damageUpgradeLevel << std::endl;
                        }
                        else if (upgradeCost < 0)
                        {
                            std::cout << "Tower is already at maximum level!" << std::endl;
                        }
                        else
                        {
                            std::cout << "Not enough beans to upgrade! Need " << upgradeCost << " beans." << std::endl;
                        }
                        buttonClicked = true;
                    }
                    else if (isPointInRect((float)mouseX, (float)mouseY, towerMenu.upgradeButton2))
                    {
                        // Check if tower can be upgraded and player has enough beans
                        int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, towerMenu.selectedTower->attackSpeedUpgradeLevel);
                        if (upgradeCost > 0 && beanCount >= upgradeCost)
                        {
                            beanCount -= upgradeCost;
                            towerMenu.selectedTower->upgradeAttackSpeed();
                            std::cout << "Upgraded tower! New attack speed level: " << towerMenu.selectedTower->attackSpeedUpgradeLevel << std::endl;
                        }
                        else if (upgradeCost < 0)
                        {
                            std::cout << "Tower is already at maximum level!" << std::endl;
                        }
                        else
                        {
                            std::cout << "Not enough beans to upgrade! Need " << upgradeCost << " beans." << std::endl;
                        }
                        buttonClicked = true;
                    }
                    else if (isPointInRect((float)mouseX, (float)mouseY, towerMenu.upgradeButton3))
                    {
                        // Check if tower can be upgraded and player has enough beans
                        int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, towerMenu.selectedTower->rangeUpgradeLevel);
                        if (upgradeCost > 0 && beanCount >= upgradeCost)
                        {
                            beanCount -= upgradeCost;
                            towerMenu.selectedTower->upgradeRange();
                            std::cout << "Upgraded tower! New range level: " << towerMenu.selectedTower->rangeUpgradeLevel << std::endl;
                        }
                        else if (upgradeCost < 0)
                        {
                            std::cout << "Tower is already at maximum level!" << std::endl;
                        }
                        else
                        {
                            std::cout << "Not enough beans to upgrade! Need " << upgradeCost << " beans." << std::endl;
                        }
                        buttonClicked = true;
                    }
                    else if (isPointInRect((float)mouseX, (float)mouseY, towerMenu.sellButton))
                    {
                        // Calculate sell value (50% of current cost including upgrades)
                        int baseCost = getTowerCost(towerMenu.selectedTower->type);
                        
                        // Add value of each upgrade (50% of upgrade costs)
                        int damageUpgradeValue = 0;
                        int attackSpeedUpgradeValue = 0;
                        int rangeUpgradeValue = 0;
                        
                        // Calculate value of damage upgrades
                        for (int i = 0; i < towerMenu.selectedTower->damageUpgradeLevel; i++)
                        {
                            int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, i);
                            damageUpgradeValue += upgradeCost;
                        }
                        
                        // Calculate value of attack speed upgrades
                        for (int i = 0; i < towerMenu.selectedTower->attackSpeedUpgradeLevel; i++)
                        {
                            int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, i);
                            attackSpeedUpgradeValue += upgradeCost;
                        }
                        
                        // Calculate value of range upgrades
                        for (int i = 0; i < towerMenu.selectedTower->rangeUpgradeLevel; i++)
                        {
                            int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, i);
                            rangeUpgradeValue += upgradeCost;
                        }
                        
                        // Total upgrade value
                        int upgradeValue = damageUpgradeValue + attackSpeedUpgradeValue + rangeUpgradeValue;
                        
                        // Final sell value is 50% of (base cost + upgrades)
                        int sellValue = (baseCost + upgradeValue) / 2;
                        beanCount += sellValue;
                        
                        // Remove the tower
                        towers.erase(std::remove_if(towers.begin(), towers.end(),
                                                    [selectedTower = towerMenu.selectedTower](const Tower &tower)
                                                    {
                                return &tower == selectedTower;
                                                    }),
                                     towers.end());
                            
                        // Decrement tower count
                        switch (towerMenu.selectedTower->type)
                        {
                        case TowerType::APPLE:
                            appleTowerCount--;
                            break;
                        case TowerType::CARROT:
                            carrotTowerCount--;
                            break;
                        case TowerType::POTATO:
                            potatoTowerCount--;
                            break;
                        case TowerType::PINEAPPLE:
                            pineappleTowerCount--;
                            break;
                        case TowerType::BANANA_PEEL:
                            bananaPeelTowerCount--;
                            break;
                        case TowerType::CACTUS:
                            cactusTowerCount--;
                            break;
                        default:
                            break;
                        }
                        
                        std::cout << "Sold tower for " << sellValue << " beans. Total beans: " << beanCount << std::endl;
                        towerMenu.close();
                        buttonClicked = true;
                    }
                    
                    // Only check for tower selection if no button was clicked
                    if (!buttonClicked && mouseX < GAME_WIDTH)
                    {
                        for (auto &tower : towers)
                        {
                            float uniformScale = std::min(scaleX, scaleY);
                            float drawX = tower.pos.x * scaleX;
                            float drawY = tower.pos.y * scaleY;
                            float drawSize = TOWER_SIZE * uniformScale;
                            if (distance(Point(mouseX, mouseY), Point(drawX, drawY)) < drawSize / 2)
                            {
                                towerMenu.open(&tower);
                                break;
                            }
                        }
                    }
                }
                // Check if clicked on an existing tower
                else if (mouseX < GAME_WIDTH)
                { // Only check in game area
                    bool towerClicked = false;
                    for (auto &tower : towers)
                    {
                        float uniformScale = std::min(scaleX, scaleY);
                        float drawX = tower.pos.x * scaleX;
                        float drawY = tower.pos.y * scaleY;
                        float drawSize = TOWER_SIZE * uniformScale;
                        if (distance(Point(mouseX, mouseY), Point(drawX, drawY)) < drawSize / 2)
                        {
                            towerMenu.open(&tower);
                            towerClicked = true;
                            break;
                        }
                    }
                    
                    // Only try to place if no tower was clicked and a tower type is selected
                    if (!towerClicked && placementTower.type != TowerType::NONE)
                    {
                        // Check placement validity
                        bool canPlace = true;
                        
                        // Don't allow placing overlapping with path or water
                        if (!canPlaceTower(clickPos, TOWER_SIZE, placementTower.type, scaledWaypoints))
                        {
                            canPlace = false;
                            if (placementTower.type == TowerType::BANANA_PEEL)
                            {
                                std::cout << "Banana Peel must be placed directly on the path!" << std::endl;
                            }
                            else
                            {
                                std::cout << "Cannot place tower on water or path!" << std::endl;
                            }
                        }
                        
                        // Don't allow placing towers on top of each other
                        for (const auto &tower : towers)
                        {
                            if (distance(tower.pos, placementTower.pos) < TOWER_SIZE)
                            {
                                canPlace = false;
                                std::cout << "Cannot place tower on another tower!" << std::endl;
                                break;
                            }
                        }
                        
                        // Check if player has enough beans to buy the tower
                        int towerCost = getTowerCost(placementTower.type);
                        if (beanCount < towerCost)
                        {
                            canPlace = false;
                            std::cout << "Not enough beans to buy this tower! Need " << towerCost << " beans." << std::endl;
                        }
                        
                        if (canPlace)
                        {
                            Tower newTower((float)mouseX, (float)mouseY);
                            newTower.setType(placementTower.type); // Copy the selected tower type
                            towers.push_back(newTower);
                            
                            // Deduct tower cost from bean count
                            beanCount -= towerCost;
                            
                            // Increment tower count for that type
                            switch (newTower.type)
                            {
                                case TowerType::APPLE:
                                    appleTowerCount++;
                                    break;
                                case TowerType::CARROT:
                                    carrotTowerCount++;
                                    break;
                                case TowerType::POTATO:
                                    potatoTowerCount++;
                                    break;
                                case TowerType::PINEAPPLE:
                                    pineappleTowerCount++;
                                    break;
                                case TowerType::BANANA_PEEL:
                                    bananaPeelTowerCount++;
                                    break;
                                case TowerType::CACTUS:
                                    cactusTowerCount++;
                                    break;
                                default:
                                    break;
                            }
                            
                            // Deselect tower after placement
                            placementTower.type = TowerType::NONE;
                        }
                    }
                }
                
                // Reset mouseJustPressed regardless of whether placement was successful
                mouseJustPressed = false;
            }
            else if (!mouseLeftPressed)
            {
                // Reset flag when mouse is released
                mouseJustPressed = false;
            }

            // Update all active enemies
            for (auto &enemy : enemies)
            {
                updateEnemy(enemy, deltaTime, towers, beanCount, currentWaypoints);
            }

            // Handle round system
            if (!isGameOver)
            {
                if (!isRoundActive)
                {
                    // Countdown to next round
                    roundStartTimer -= deltaTime;
                    if (roundStartTimer <= 0)
                    {
                        startNewRound(enemies, currentRound);
                        // Reset timer for next round - always 5 seconds
                        roundStartTimer = 5.0f; // Standard 5 second break between all rounds
                    }
                }
                else
                {
                    // Handle enemy spawning during active round
                    if (enemiesLeftInRound > 0)
                    {
                        enemySpawnTimer += deltaTime;
                        float spawnRate = 1.0f; // Fixed spawn rate of 1 enemy per second
                        
                        if (enemySpawnTimer >= spawnRate)
                        {
                            enemySpawnTimer = 0.0f;
                            
                            // Find inactive enemy to spawn
                            for (auto &enemy : enemies)
                            {
                                if (!enemy.isActive)
                                {
                                    enemy.isActive = true;
                                    enemy.currentWaypoint = 0;
                                    enemy.progress = 0.0f;
                                    
                                    // Spawn boss as the last enemy of a boss round
                                    if (isBossRound(currentRound) && enemiesLeftInRound == 1)
                                    {
                                        enemy.type = EnemyType::BOSS;
                                        std::cout << "A BOSS has appeared!" << std::endl;
                                    }
                                    else
                                    {
                                        // Determine regular enemy type based on round
                                        float skeletonChance = getSkeletonPercentage(currentRound);
                                        float tankChance = getTankPercentage(currentRound);
                                        float ghostChance = getGhostPercentage(currentRound);
                                        float random = static_cast<float>(rand()) / RAND_MAX;
                                        
                                        if (random < skeletonChance)
                                        {
                                            enemy.type = EnemyType::SKELETON;
                                        }
                                        else if (random < skeletonChance + tankChance)
                                        {
                                            enemy.type = EnemyType::TANK;
                                        }
                                        else if (random < skeletonChance + tankChance + ghostChance)
                                        {
                                            enemy.type = EnemyType::GHOST;
                                        }
                                        else
                                        {
                                            enemy.type = EnemyType::ZOMBIE;
                                        }
                                    }
                                    
                                    enemy.setType(enemy.type);
                                    enemiesLeftInRound--;
                                    break;
                                }
                            }
                        }
                    }
                    
                    // Check if round is complete
                    if (isRoundComplete(enemies))
                    {
                        isRoundActive = false;
                        // Give bonus beans for completing a round
                        int roundBonus = currentRound * 5;
                        beanCount += roundBonus;
                        
                        // Regular round completion message
                        std::cout << "Round " << currentRound << " complete! Bonus beans: " << beanCount << std::endl;
                    }
                }
            }
            else
            {
                // Game is over - check for mouse click to restart
                if (mouseLeftPressed)
                {
                    resetGame(enemies, towers, projectiles, beanCount);
                    mouseLeftPressed = false;
                }
            }

            // Update towers and handle shooting
            for (auto &tower : towers)
            {
                // Only update towers if game is not over
                if (!isGameOver)
                {
                    tower.shootTimer += deltaTime;
                    
                    // Only shooting towers need to find targets and shoot
                    if (tower.type != TowerType::BANANA_PEEL && tower.type != TowerType::CACTUS)
                    {
                        if (tower.shootTimer >= 1.0f / tower.attackSpeed)
                        {
                            if (tower.type == TowerType::PINEAPPLE)
                            {
                                bool enemyInRange = false;
                                for (const auto &enemy : enemies)
                                {
                                    if (enemy.isActive)
                                    {
                                        Point enemyPos = enemy.getPosition(currentWaypoints);
                                        if (distance(tower.pos, enemyPos) <= tower.range)
                                        {
                                            enemyInRange = true;
                                            break;
                                        }
                                    }
                                }
                                if (enemyInRange)
                                {
                                    spawnProjectilesInAllDirections(projectiles, tower.pos, tower);
                                    tower.shootTimer = 0;
                                }
                            }
                            else
                            {
                                Enemy *target = findClosestEnemy(tower.pos, tower.range, enemies, currentWaypoints);
                                if (target)
                                {
                                    Point targetPos = target->getPosition(currentWaypoints);
                                    spawnProjectile(projectiles, tower.pos, targetPos, tower);
                                    tower.shootTimer = 0;
                                }
                            }
                        }
                    }
                }
            }
            
            // Update and check projectile collisions
            for (auto &proj : projectiles)
            {
                if (!proj.active)
                    continue;
                
                proj.update(deltaTime);
                
                // Check for collisions with enemies
                for (auto &enemy : enemies)
                {
                    if (!enemy.isActive)
                        continue;
                    
                    float enemySize = (enemy.type == EnemyType::BOSS) ? BOSS_SIZE : ENEMY_SIZE;
                    
                    Point enemyPos = enemy.getPosition(currentWaypoints);
                    if (distance(proj.pos, enemyPos) < enemySize / 2)
                    {
                        // Hit! Deactivate projectile
                        proj.active = false;
                        
                        // Apply damage to enemy
                        enemy.health -= proj.damage;
                        
                        // Check if enemy is defeated
                        if (enemy.health <= 0)
                        {
                            // Add beans based on enemy type
                            if (enemy.type == EnemyType::BOSS)
                            {
                                beanCount += BOSS_BEANS;
                                std::cout << "BOSS destroyed! +" << BOSS_BEANS << " beans!" << std::endl;
                            }
                            else
                            {
                                beanCount += (enemy.type == EnemyType::ZOMBIE) ? ZOMBIE_BEANS : SKELETON_BEANS;
                                std::cout << "Enemy destroyed! Beans: " << beanCount << std::endl;
                            }
                            enemy.isActive = false;
                        }
                        break;
                    }
                }
                
                // Deactivate projectiles that go too far off screen
                if (proj.pos.x < -50 || proj.pos.x > WIDTH + 50 ||
                    proj.pos.y < -50 || proj.pos.y > HEIGHT + 50)
                {
                    proj.active = false;
                }
            }
            
            // Remove used banana peels and empty cacti
            towers.erase(std::remove_if(towers.begin(), towers.end(),
                                        [](const Tower &tower)
                                        {
                    return (tower.type == TowerType::BANANA_PEEL && tower.isUsed); 
                                        }),
                         towers.end());

            // Decrease game start timer
            if (gameStartTimer > 0.0f)
            {
                gameStartTimer -= deltaTime;
            }
        }

        // Update tower button hover states
        if (!isGameOver)
        {
            for (auto &button : towerButtons)
            {
                button.isHovered = isPointInRect((float)mouseX, (float)mouseY, button.rect);
                if (button.isHovered && mouseLeftPressed && !towerMenu.isOpen)
                { // Only allow tower selection if menu is not open
                    // If clicking the same tower type that's already selected, deselect it
                    if (placementTower.type == button.type)
                    {
                        placementTower.setType(TowerType::NONE);
                    }
                    else
                    {
                        placementTower.setType(button.type);
                    }
                    mouseLeftPressed = false; // Consume the click
                }
            }
        }

        // Clear screen
        renderer.clearScreen({0.1, 0.2, 0.6, 1});

        // Draw background
        renderer.renderRectangle({0 * scaleX, 0 * scaleY, GAME_WIDTH * scaleX, HEIGHT * scaleY}, currentBg, {1, 1, 1, 1});
        
        // Draw UI panel background
        renderer.renderRectangle(
            {(float)GAME_WIDTH * scaleX, 0 * scaleY, (float)PANEL_WIDTH * scaleX, (float)HEIGHT * scaleY},
            {UI_BACKGROUND.r, UI_BACKGROUND.g, UI_BACKGROUND.b, UI_BACKGROUND.a});
        
        // Draw bean count in top-right corner with larger size and better position
        drawNumber(renderer, beanCount, (GAME_WIDTH - 180) * scaleX, 15 * scaleY, 32.0f * scaleY); // Moved from -120 to -180 to accommodate larger numbers
        
        // Draw round information in top-left corner
        float roundTextX = 70 * scaleX;
        float roundTextY = 15 * scaleY;
        float roundDigitSize = 24.0f * scaleY;
        
        // Draw 'R' symbol using alphabet texture
        float rSize = 30.0f * scaleY;
        float rX = roundTextX - 50 * scaleX;
        float rY = roundTextY;
        drawText(renderer, "R", rX, rY, rSize);
        
        // Draw round number with background
        std::string roundStr = std::to_string(currentRound);
        float bgWidth = roundDigitSize * 1.2f * roundStr.length() + 20 * scaleX;
        
        renderer.renderRectangle(
            {roundTextX - 5 * scaleX, roundTextY - 5 * scaleY, bgWidth, roundDigitSize + 10 * scaleY},
            {0.0f, 0.0f, 0.0f, 0.5f});
        
        // Draw each digit of the round number
        for (size_t i = 0; i < roundStr.length(); i++)
        {
            drawDigit(renderer, roundStr[i] - '0', roundTextX + i * (roundDigitSize * 1.2f), roundTextY, roundDigitSize);
        }
        
        // Draw current lives
        float livesX = 200 * scaleX;
        float livesY = 15 * scaleY;
        
        // Draw heart icon for lives
        if (!isGameOver)
        {
            for (int i = 0; i < lives; i++)
            {
                renderer.renderRectangle(
                    {livesX + i * 30 * scaleX, livesY, 20 * scaleX, 20 * scaleY * (scaleY * 0.9f)},
                    heartTexture, {1.0f, 1.0f, 1.0f, 1.0f});
            }
        }
        
        // If game is over, show game over screen
        if (isGameOver)
        {
            // Overlay with semi-transparent black
            renderer.renderRectangle(
                {0, 0, (float)w, (float)h},
                {0.0f, 0.0f, 0.0f, 0.7f});
            
            // Show 'YOU LOST' centered with dynamic scaling
            std::string lostText = "YOU LOST";
            float textSize = 64.0f * scaleY;
            float textWidth = (lostText.length() * (textSize + 2.0f));
            float x = (w - textWidth) / 2.0f;
            float y = (h / 2.0f) - (textSize / 2.0f);
            renderer.renderRectangle({x - 30, y - 30, textWidth + 60, textSize + 60}, {0, 0, 0, 0.8f});
            drawText(renderer, lostText, x, y, textSize, 2.0f, 1.0f);
            
            // Check for click to restart - use mouseJustPressed instead of mouseLeftPressed
            if (mouseJustPressed)
            {
                resetGame(enemies, towers, projectiles, beanCount);
                mouseJustPressed = false;
            }
        }
        
        // If we're between rounds, show countdown
        if (!isRoundActive && !isGameOver && currentScreen == GameScreen::GAME)
        {
            int countdown = (int)roundStartTimer + 1;
            std::string countdownStr = std::to_string(countdown);
            float countdownX = w / 2 - 30 * scaleX;
            float countdownY = 100 * scaleY;
            float countdownSize = 60.0f * scaleY;
            
            // Draw large countdown number
            renderer.renderRectangle(
                {countdownX - 10 * scaleX, countdownY - 10 * scaleY, countdownSize + 20 * scaleX, countdownSize + 20 * scaleY},
                {0.0f, 0.0f, 0.0f, 0.7f});
            
            for (size_t i = 0; i < countdownStr.length(); i++)
            {
                drawDigit(renderer, countdownStr[i] - '0', countdownX + i * (countdownSize * 0.8f), countdownY, countdownSize);
            }
            
            // Draw "NEXT ROUND" text using rectangles with dynamic scaling
            float textY = countdownY + countdownSize + 20 * scaleY;
            renderer.renderRectangle({w / 2 - 100 * scaleX, textY, 200 * scaleX, 5 * scaleY}, {1.0f, 1.0f, 1.0f, 1.0f});
        }

        // Draw tower selection buttons
        if (!towerMenu.isOpen)
        { // Only draw tower selection buttons if tower menu is not open
            float buttonW = 160 * scaleX;
            float buttonH = 60 * scaleY;
            float buttonSpacing = 10 * scaleY;
            float totalButtonsHeight = towerButtons.size() * buttonH + (towerButtons.size() - 1) * buttonSpacing;
            float startY = (HEIGHT * scaleY - totalButtonsHeight) / 2.0f;
            for (size_t i = 0; i < towerButtons.size(); ++i) {
                towerButtons[i].rect.x = (GAME_WIDTH + 20) * scaleX;
                towerButtons[i].rect.y = startY + i * (buttonH + buttonSpacing);
                towerButtons[i].rect.w = buttonW;
                towerButtons[i].rect.h = buttonH;
            }
            for (const auto &button : towerButtons)
            {
                // Draw button background
                Color buttonColor = button.isHovered ? UI_SELECTED : UI_BACKGROUND;
                if (placementTower.type == button.type)
                {
                    buttonColor = {0.3f, 0.5f, 0.3f, 1.0f}; // Green tint for selected tower
                }
                
                // Check if player can afford this tower
                bool canAfford = beanCount >= getTowerCost(button.type);
                
                // If can't afford, make button appear darker
                if (!canAfford)
                {
                    buttonColor.r *= 0.7f;
                    buttonColor.g *= 0.7f;
                    buttonColor.b *= 0.7f;
                }
                
                // Use the same conservative scaling for all visual elements
                float bx = button.rect.x;
                float by = button.rect.y;
                float bw = button.rect.w;
                float bh = button.rect.h;
                renderer.renderRectangle(
                    {bx, by, bw, bh},
                    {buttonColor.r, buttonColor.g, buttonColor.b, buttonColor.a});
                
                // Draw tower preview with its specific color or texture
                float towerSize = 32.0f * buttonScale;
                float towerX = bx + bw / 2 - towerSize / 2;
                float towerY = by + 10 * buttonScale;
                Color previewColor = getTowerColor(button.type);
                
                // Make the tower preview darker if can't afford
                if (!canAfford)
                {
                    previewColor.r *= 0.7f;
                    previewColor.g *= 0.7f;
                    previewColor.b *= 0.7f;
                }
                
                // Use the correct texture for each tower type if available
                if (button.type == TowerType::CARROT && carrotTowerTexture.id != 0)
                {
                    renderer.renderRectangle(
                        {towerX, towerY, towerSize, towerSize},
                        carrotTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                }
                else if (button.type == TowerType::APPLE && appleTowerTexture.id != 0)
                {
                    renderer.renderRectangle(
                        {towerX, towerY, towerSize, towerSize},
                        appleTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                }
                else if (button.type == TowerType::BANANA_PEEL && bananaPeelTexture.id != 0)
                {
                    renderer.renderRectangle(
                        {towerX, towerY, towerSize, towerSize},
                        bananaPeelTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                }
                else if (button.type == TowerType::CACTUS && cactusTexture.id != 0)
                {
                    renderer.renderRectangle(
                        {towerX, towerY, towerSize, towerSize},
                        cactusTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                }
                else if (button.type == TowerType::PINEAPPLE && pineappleTowerTexture.id != 0)
                {
                    renderer.renderRectangle(
                        {towerX, towerY, towerSize, towerSize},
                        pineappleTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                }
                else if (button.type == TowerType::POTATO && potatoTowerTexture.id != 0)
                {
                    renderer.renderRectangle(
                        {towerX, towerY, towerSize, towerSize},
                        potatoTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                }
                else
                {
                    renderer.renderRectangle(
                        {towerX, towerY, towerSize, towerSize},
                        {previewColor.r, previewColor.g, previewColor.b, previewColor.a});
                }
                
                // Draw cost indicator
                int cost = getTowerCost(button.type);
                std::string costStr = std::to_string(cost);
                float digitSize = 16.0f * buttonScale;
                float costX = bx + 10 * buttonScale;
                float costY = by + bh - 20 * buttonScale;
                
                // Draw bean icon
                renderer.renderRectangle(
                    {costX, costY, 10.0f * buttonScale, 10.0f * buttonScale},
                    {0.6f, 0.4f, 0.2f, 1.0f});
                
                // Draw digits for cost with increased spacing
                for (size_t i = 0; i < costStr.length(); i++)
                {
                    drawDigit(renderer, costStr[i] - '0', costX + 15 * buttonScale + i * (digitSize * 1.2f), costY - 5 * buttonScale, digitSize);
                }
                
                // Draw tower count indicator
                int towerCount = 0;
                switch (button.type)
                {
                case TowerType::APPLE:
                    towerCount = appleTowerCount;
                    break;
                case TowerType::CARROT:
                    towerCount = carrotTowerCount;
                    break;
                case TowerType::POTATO:
                    towerCount = potatoTowerCount;
                    break;
                case TowerType::PINEAPPLE:
                    towerCount = pineappleTowerCount;
                    break;
                case TowerType::BANANA_PEEL:
                    towerCount = bananaPeelTowerCount;
                    break;
                case TowerType::CACTUS:
                    towerCount = cactusTowerCount;
                    break;
                }

                if (towerCount > 0)
                {
                    std::string countStr = "x" + std::to_string(towerCount);
                    float countX = bx + bw - 30 * buttonScale;
                    float countY = by + 15 * buttonScale;
                    
                    renderer.renderRectangle(
                        {countX - 5 * buttonScale, countY - 5 * buttonScale, 30 * buttonScale, 20 * buttonScale},
                        {0.0f, 0.0f, 0.0f, 0.5f});
                    
                    float smallDigitSize = 14.0f * buttonScale;
                    for (size_t i = 0; i < countStr.length(); i++)
                    {
                        if (countStr[i] == 'x')
                            continue;
                        drawDigit(renderer, countStr[i] - '0', countX + 10 * buttonScale + (i - 1) * (smallDigitSize * 0.8f), countY, smallDigitSize);
                    }
                }
            }
        }
        
        // Draw tower menu if open
        if (towerMenu.isOpen)
        {
            // Calculate dynamic positions for menu buttons based on scaling
            float panelX = GAME_WIDTH * scaleX;
            float panelW = PANEL_WIDTH * scaleX;
            float panelH = HEIGHT * scaleY;
            float btnW = 180 * scaleX;
            float btnH = 40 * scaleY;
            float btnX = panelX + (panelW - btnW) / 2.0f;
            float spacing = 50 * scaleY; // Increased spacing
            float firstBtnY = 170 * scaleY;
            float secondBtnY = firstBtnY + btnH + spacing;
            float thirdBtnY = secondBtnY + btnH + spacing;
            float sellBtnY = panelH - 60 * scaleY;
            float closeBtnSize = 30 * scaleX;
            float closeBtnX = panelX + panelW - closeBtnSize - 10 * scaleX;
            float closeBtnY = sellBtnY;

            // Set rectangles dynamically
            towerMenu.upgradeButton1 = Rectangle(btnX, firstBtnY, btnW, btnH);
            towerMenu.upgradeButton2 = Rectangle(btnX, secondBtnY, btnW, btnH);
            towerMenu.upgradeButton3 = Rectangle(btnX, thirdBtnY, btnW, btnH);
            towerMenu.sellButton = Rectangle(btnX, sellBtnY, btnW, btnH);
            towerMenu.closeButton = Rectangle(closeBtnX, closeBtnY, closeBtnSize, closeBtnSize);

            // Draw menu background
            renderer.renderRectangle(
                {panelX, 0, panelW, panelH},
                {UI_BACKGROUND.r, UI_BACKGROUND.g, UI_BACKGROUND.b, 1.0f});
            
            // Draw selected tower info
            if (towerMenu.selectedTower)
            {
                // Draw tower name at the top
                float nameY = 50 * scaleY;
                std::string towerName = getTowerTypeName(towerMenu.selectedTower->type);

                // Draw tower preview (larger)
                float previewSize = TOWER_SIZE * 1.5f * scaleY;
                float previewY = 100 * scaleY;
                Color towerColor = getTowerColor(towerMenu.selectedTower->type);
                float previewX = GAME_WIDTH * scaleX + PANEL_WIDTH * scaleX / 2 - previewSize / 2;

                // Draw the correct texture for the tower type if available
                bool drewTexture = false;
                switch (towerMenu.selectedTower->type) {
                    case TowerType::CARROT:
                        if (carrotTowerTexture.id != 0) {
                            renderer.renderRectangle(
                                {previewX, previewY, previewSize, previewSize},
                                carrotTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                            drewTexture = true;
                        }
                        break;
                    case TowerType::APPLE:
                        if (appleTowerTexture.id != 0) {
                            renderer.renderRectangle(
                                {previewX, previewY, previewSize, previewSize},
                                appleTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                            drewTexture = true;
                        }
                        break;
                    case TowerType::BANANA_PEEL:
                        if (bananaPeelTexture.id != 0) {
                            renderer.renderRectangle(
                                {previewX, previewY, previewSize, previewSize},
                                bananaPeelTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                            drewTexture = true;
                        }
                        break;
                    case TowerType::CACTUS:
                        if (cactusTexture.id != 0) {
                            renderer.renderRectangle(
                                {previewX, previewY, previewSize, previewSize},
                                cactusTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                            drewTexture = true;
                        }
                        break;
                    case TowerType::PINEAPPLE:
                        if (pineappleTowerTexture.id != 0) {
                            renderer.renderRectangle(
                                {previewX, previewY, previewSize, previewSize},
                                pineappleTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                            drewTexture = true;
                        }
                        break;
                    case TowerType::POTATO:
                        if (potatoTowerTexture.id != 0) {
                            renderer.renderRectangle(
                                {previewX, previewY, previewSize, previewSize},
                                potatoTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                            drewTexture = true;
                        }
                        break;
                    default:
                        break;
                }
                if (!drewTexture) {
                    renderer.renderRectangle(
                        {previewX, previewY, previewSize, previewSize},
                        {towerColor.r, towerColor.g, towerColor.b, 1.0f});
                }

                // Draw upgrade buttons
                std::vector<std::pair<std::string, int *>> upgrades = {
                    {"damage", &towerMenu.selectedTower->damageUpgradeLevel},
                    {"attackSpeed", &towerMenu.selectedTower->attackSpeedUpgradeLevel},
                    {"range", &towerMenu.selectedTower->rangeUpgradeLevel}};

                for (int i = 0; i < 3; i++)
                {
                    Rectangle &upgradeButton = (i == 0) ? towerMenu.upgradeButton1 : (i == 1) ? towerMenu.upgradeButton2
                                                                                          : towerMenu.upgradeButton3;

                    Color upgradeColor(0.4f, 0.1f, 0.1f, 1.0f); // Default to dark red
                    int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, *upgrades[i].second);
                    bool canUpgrade = upgradeCost > 0 && beanCount >= upgradeCost;
                    
                    if (*upgrades[i].second >= 3)
                    {
                        upgradeColor = Color(0.0f, 0.8f, 0.0f, 1.0f); // Green for max level
                    }
                    else if (canUpgrade)
                    {
                        upgradeColor = isPointInRect((float)mouseX, (float)mouseY, upgradeButton)
                                               ? Color(0.0f, 0.8f, 0.0f, 1.0f)  // Bright green when hovered
                                               : Color(0.0f, 0.6f, 0.0f, 1.0f); // Dark green normally
                    }
                    
                    // Draw button background
                    renderer.renderRectangle(
                        {upgradeButton.x, upgradeButton.y, upgradeButton.w, upgradeButton.h},
                        {upgradeColor.r, upgradeColor.g, upgradeColor.b, upgradeColor.a});
                    
                    // Draw button label background
                    renderer.renderRectangle(
                        {upgradeButton.x + 5, upgradeButton.y + 5, upgradeButton.w - 10, 20},
                        {0.0f, 0.0f, 0.0f, 0.5f});
                    
                    // Draw upgrade type text label below the button, left-aligned and smaller
                    const char* upgradeLabel = (i == 0) ? "DAMAGE" : (i == 1) ? "ATTACK SPEED" : "RANGE";
                    float labelSize = 14.0f * scaleY; // Smaller text
                    float labelX = upgradeButton.x + 10 * scaleX; // Left edge with margin
                    float labelY = upgradeButton.y + upgradeButton.h + 6 * scaleY; // Slightly closer
                    drawText(renderer, upgradeLabel, labelX, labelY, labelSize, 2.0f, 1.0f);
                    
                    // Instead of trying to render text, use color-coded symbols
                    if (i == 0)
                    { // DAMAGE - red symbol
                        // Draw damage symbol (sword/arrow)
                        renderer.renderRectangle(
                            {upgradeButton.x + 10, upgradeButton.y + 10, 30, 10},
                            {1.0f, 0.3f, 0.3f, 1.0f} // Red
                        );
                        
                        // Draw cost text only if not maxed out
                        if (*upgrades[i].second < 3)
                        {
                            drawNumber(renderer, upgradeCost, upgradeButton.x + upgradeButton.w - 80, upgradeButton.y + 10, 20.0f);
                        }
                    }
                    else if (i == 1)
                    { // ATTACK SPEED - blue symbol
                        // Draw speed symbol (lightning)
                        renderer.renderRectangle(
                            {upgradeButton.x + 10, upgradeButton.y + 10, 10, 20},
                            {0.3f, 0.3f, 1.0f, 1.0f} // Blue
                        );
                        renderer.renderRectangle(
                            {upgradeButton.x + 30, upgradeButton.y + 10, 10, 20},
                            {0.3f, 0.3f, 1.0f, 1.0f} // Blue
                        );
                        
                        // Draw cost text only if not maxed out
                        if (*upgrades[i].second < 3)
                        {
                            drawNumber(renderer, upgradeCost, upgradeButton.x + upgradeButton.w - 80, upgradeButton.y + 10, 20.0f);
                        }
                    }
                    else
                    { // RANGE - green symbol
                        // Draw range symbol (circle)
                        renderer.renderRectangle(
                            {upgradeButton.x + 20, upgradeButton.y + 10, 20, 20},
                            {0.3f, 0.8f, 0.3f, 1.0f} // Green
                        );
                        
                        // Draw cost text only if not maxed out
                        if (*upgrades[i].second < 3)
                        {
                            drawNumber(renderer, upgradeCost, upgradeButton.x + upgradeButton.w - 80, upgradeButton.y + 10, 20.0f);
                        }
                    }
                    
                    // Draw upgrade level indicator at the bottom of button
                    int level = *upgrades[i].second;
                    float indicatorWidth = upgradeButton.w / 3;
                    float indicatorHeight = 5.0f;
                    float indicatorY = upgradeButton.y + upgradeButton.h - 10.0f;
                    
                    for (int j = 0; j < 3; j++)
                    {
                        Color levelColor = (j < level) ? Color(1.0f, 1.0f, 0.0f, 1.0f) : Color(0.5f, 0.5f, 0.5f, 0.5f);
                        renderer.renderRectangle(
                            {upgradeButton.x + 10.0f + (j * indicatorWidth), indicatorY, indicatorWidth - 5.0f, indicatorHeight},
                            {levelColor.r, levelColor.g, levelColor.b, levelColor.a});
                    }
                }
                
                // Draw sell button
                Color sellColor = isPointInRect((float)mouseX, (float)mouseY, towerMenu.sellButton) 
                                      ? Color(0.8f, 0.2f, 0.2f, 1.0f)
                                      : Color(0.6f, 0.1f, 0.1f, 1.0f);
                
                renderer.renderRectangle(
                    {towerMenu.sellButton.x, towerMenu.sellButton.y, 
                     towerMenu.sellButton.w, towerMenu.sellButton.h},
                    {sellColor.r, sellColor.g, sellColor.b, sellColor.a});
                
                // Draw sell value (exactly 50% of current cost)
                int currentCost = getTowerCost(towerMenu.selectedTower->type);
                int sellValue = currentCost / 2;
                std::string sellText = std::to_string(sellValue);
                float digitSize = 20.0f;
                float textX = towerMenu.sellButton.x + 40;
                float textY = towerMenu.sellButton.y + 10;
                
                // Draw bean icon
                renderer.renderRectangle(
                    {textX - 25, textY + 5, 15, 15},
                    {0.6f, 0.4f, 0.2f, 1.0f});
                
                // Draw sell value
                for (size_t i = 0; i < sellText.length(); i++)
                {
                    drawDigit(renderer, sellText[i] - '0', textX + i * (digitSize * 1.2f), textY, digitSize);
                }
                
                // Draw close button (X)
                Color closeColor = isPointInRect((float)mouseX, (float)mouseY, towerMenu.closeButton) 
                                       ? Color(1.0f, 0.2f, 0.2f, 1.0f)
                                       : Color(0.8f, 0.0f, 0.0f, 1.0f); // Brighter red on hover
                
                // Draw X shape with thicker lines and better positioning
                float xSize = 16.0f;                                                // Reduced size from 24.0f to 16.0f
                float xX = GAME_WIDTH * scaleX + PANEL_WIDTH * scaleX - xSize - 10; // Position in top right
                float xY = 10;                                                      // Small margin from top
                float thickness = 3.0f;                                             // Reduced thickness from 4.0f to 3.0f
                
                // Background circle for the X
                renderer.renderRectangle(
                    {xX - 3, xY - 3, xSize + 6, xSize + 6}, // Adjusted padding for smaller size
                    {0.2f, 0.0f, 0.0f, 1.0f}                // Dark red background
                );
                
                // Draw diagonal lines for X
                float diagonal = sqrt(2.0f) * xSize; // Length of diagonal
                
                // First diagonal (top-left to bottom-right)
                for (int i = 0; i < diagonal; i++)
                {
                    renderer.renderRectangle(
                        {xX + i, xY + i, thickness, thickness},
                        {closeColor.r, closeColor.g, closeColor.b, closeColor.a});
                }
                
                // Second diagonal (top-right to bottom-left)
                for (int i = 0; i < diagonal; i++)
                {
                    renderer.renderRectangle(
                        {xX + xSize - i, xY + i, thickness, thickness},
                        {closeColor.r, closeColor.g, closeColor.b, closeColor.a});
                }
                
                // Update close button hitbox
                towerMenu.closeButton = Rectangle(xX - 3, xY - 3, xSize + 6, xSize + 6);
            }
        }

        // Draw towers
        for (const auto &tower : towers)
        {
    Color towerColor = getTowerColor(tower.type);
            float uniformScale = std::min(scaleX, scaleY);
            float drawX = tower.pos.x * scaleX;
            float drawY = tower.pos.y * scaleY;
            float drawSize = TOWER_SIZE * uniformScale;
            if (tower.type == TowerType::CACTUS)
            {
                if (cactusTexture.id != 0)
                {
                    renderer.renderRectangle(
                        {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                        cactusTexture, {1.0f, 1.0f, 1.0f, 1.0f});
                }
                else
                {
                    renderCactus(renderer, tower, scaleX, scaleY);
                }
            }
            else if (tower.type == TowerType::CARROT && carrotTowerTexture.id != 0)
            {
                renderer.renderRectangle(
                    {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                    carrotTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
            }
            else if (tower.type == TowerType::APPLE && appleTowerTexture.id != 0)
            {
                renderer.renderRectangle(
                    {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                    appleTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
            }
            else if (tower.type == TowerType::BANANA_PEEL && bananaPeelTexture.id != 0)
            {
                renderer.renderRectangle(
                    {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                    bananaPeelTexture, {1.0f, 1.0f, 1.0f, 1.0f});
            }
            else if (tower.type == TowerType::PINEAPPLE && pineappleTowerTexture.id != 0)
            {
                renderer.renderRectangle(
                    {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                    pineappleTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
            }
            else if (tower.type == TowerType::POTATO && potatoTowerTexture.id != 0)
            {
                renderer.renderRectangle(
                    {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                    potatoTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f});
            }
            else
            {
                renderer.renderRectangle(
                    {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                    {towerColor.r, towerColor.g, towerColor.b, towerColor.a});
            }
        }

        // Draw projectiles
        for (const auto &proj : projectiles)
        {
            if (proj.active)
            {
                float drawX = proj.pos.x * scaleX;
                float drawY = proj.pos.y * scaleY;
                float halfSize = PROJECTILE_SIZE / 2.0f * ((scaleX + scaleY) / 2.0f);
                float projSize = PROJECTILE_SIZE * ((scaleX + scaleY) / 2.0f);
                switch (proj.sourceType)
                {
                case TowerType::APPLE:
                    if (appleTexture.id != 0)
                    {
            renderer.renderRectangle(
                            {drawX - halfSize, drawY - halfSize, projSize, projSize},
                            appleTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 3.14159f, {0, 0, 1, 1});
                    }
                    else
                    {
            renderer.renderRectangle(
                            {drawX - halfSize, drawY - halfSize, projSize, projSize},
                            {1.0f, 0.2f, 0.2f, 1.0f});
                    }
                    break;
                case TowerType::CARROT:
                    if (carrotTexture.id != 0)
                    {
        renderer.renderRectangle(
                            {drawX - halfSize, drawY - halfSize, projSize, projSize},
                            carrotTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1});
                    }
                    else
                    {
                        renderer.renderRectangle(
                            {drawX - halfSize, drawY - halfSize, projSize, projSize},
                            {1.0f, 0.5f, 0.0f, 1.0f});
                    }
                    break;
                case TowerType::POTATO:
                    if (potatoTexture.id != 0)
                    {
                        renderer.renderRectangle(
                            {drawX - halfSize, drawY - halfSize, projSize, projSize},
                            potatoTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1});
                    }
                    else
                    {
                        renderer.renderRectangle(
                            {drawX - halfSize, drawY - halfSize, projSize, projSize},
                            {0.6f, 0.4f, 0.2f, 1.0f});
                    }
                    break;
                case TowerType::PINEAPPLE:
                    if (pineappleTexture.id != 0)
                    {
                        renderer.renderRectangle(
                            {drawX - halfSize, drawY - halfSize, projSize, projSize},
                            pineappleTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1});
                    }
                    else
                    {
                        renderer.renderRectangle(
                            {drawX - halfSize, drawY - halfSize, projSize, projSize},
                            {0.8f, 0.8f, 0.0f, 1.0f});
                    }
                    break;
                default:
                    renderer.renderRectangle(
                        {drawX - halfSize, drawY - halfSize, projSize, projSize},
                        {0.0f, 0.0f, 0.0f, 1.0f});
                    break;
                }
    }
}
        
        // Draw placement tower (semi-transparent)
        if (placementTower.type != TowerType::NONE)
        {
            Color placementColor = getTowerColor(placementTower.type);
            float uniformScale = std::min(scaleX, scaleY);
            float drawX = placementTower.pos.x * scaleX;
            float drawY = placementTower.pos.y * scaleY;
            float drawSize = TOWER_SIZE * uniformScale;
            renderer.renderRectangle(
                {drawX - drawSize / 2, drawY - drawSize / 2, drawSize, drawSize},
                {placementColor.r, placementColor.g, placementColor.b, 0.5f});
        }

        // Draw all active enemies
        for (const auto &enemy : enemies)
        {
            if (enemy.isActive)
            {
                Color enemyColor = getEnemyColor(enemy.type);
                float enemySize = 0.0f;
                
                switch (enemy.type)
                {
                    case EnemyType::BOSS:
                        enemySize = BOSS_SIZE * ((scaleX + scaleY) / 2.0f);
                        break;
                    case EnemyType::TANK:
                        enemySize = TANK_SIZE * ((scaleX + scaleY) / 2.0f);
                        break;
                    case EnemyType::GHOST:
                        enemySize = GHOST_SIZE * ((scaleX + scaleY) / 2.0f);
                        break;
                    case EnemyType::SKELETON:
                        enemySize = ENEMY_SIZE * ((scaleX + scaleY) / 2.0f);
                        break;
                    case EnemyType::ZOMBIE:
                    default:
                        enemySize = ENEMY_SIZE * ((scaleX + scaleY) / 2.0f);
                        break;
                }

                // Get unscaled position and scale it for rendering
                Point pos = enemy.getPosition(currentWaypoints);
                float drawX = pos.x * scaleX;
                float drawY = pos.y * scaleY;

                switch (enemy.type)
                {
                    case EnemyType::BOSS:
                    if (bossTexture.id != 0)
                    {
                            renderer.renderRectangle(
                            {drawX - enemySize / 2, drawY - enemySize / 2, enemySize, enemySize},
                            bossTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1});
                    }
                    else
                    {
                            renderer.renderRectangle(
                            {drawX - enemySize / 2, drawY - enemySize / 2, enemySize, enemySize},
                            {enemyColor.r, enemyColor.g, enemyColor.b, enemyColor.a});
                        }
                        break;
                    case EnemyType::TANK:
                    if (tankTexture.id != 0)
                    {
                            renderer.renderRectangle(
                            {drawX - enemySize / 2, drawY - enemySize / 2, enemySize, enemySize},
                            tankTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1});
                    }
                    else
                    {
                            renderer.renderRectangle(
                            {drawX - enemySize / 2, drawY - enemySize / 2, enemySize, enemySize},
                            {enemyColor.r, enemyColor.g, enemyColor.b, enemyColor.a});
                        }
                        break;
                    case EnemyType::GHOST:
                    if (ghostTexture.id != 0)
                    {
                            renderer.renderRectangle(
                            {drawX - enemySize / 2, drawY - enemySize / 2, enemySize, enemySize},
                            ghostTexture, {1.0f, 1.0f, 1.0f, enemyColor.a}, {0, 0}, 0, {0, 0, 1, 1});
                    }
                    else
                    {
                            renderer.renderRectangle(
                            {drawX - enemySize / 2, drawY - enemySize / 2, enemySize, enemySize},
                            {enemyColor.r, enemyColor.g, enemyColor.b, enemyColor.a});
                        }
                        break;
                    case EnemyType::SKELETON:
                    if (skeletonTexture.id != 0)
                    {
                            renderer.renderRectangle(
                            {drawX - enemySize / 2, drawY - enemySize / 2, enemySize, enemySize},
                            skeletonTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1});
                    }
                    else
                    {
                            renderer.renderRectangle(
                            {drawX - enemySize / 2, drawY - enemySize / 2, enemySize, enemySize},
                            {enemyColor.r, enemyColor.g, enemyColor.b, enemyColor.a});
                        }
                        break;
                    case EnemyType::ZOMBIE:
                    default:
                    if (zombieTexture.id != 0)
                    {
                            renderer.renderRectangle(
                            {drawX - enemySize / 2, drawY - enemySize / 2, enemySize, enemySize},
                            zombieTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1});
                    }
                    else
                    {
                            renderer.renderRectangle(
                            {drawX - enemySize / 2, drawY - enemySize / 2, enemySize, enemySize},
                            {enemyColor.r, enemyColor.g, enemyColor.b, enemyColor.a});
                        }
                        break;
                }

                // Draw health bar background
                float healthBarWidth = enemySize;
                float healthBarHeight = 6.0f * ((scaleX + scaleY) / 2.0f);
                renderer.renderRectangle(
                    {drawX - healthBarWidth / 2, drawY - enemySize / 2 - 10.0f * ((scaleX + scaleY) / 2.0f),
                     healthBarWidth, healthBarHeight},
                    {0.2f, 0.2f, 0.2f, 0.8f});

                // Draw health bar fill
                float healthPercent = enemy.health / enemy.maxHealth;
                float healthBarFillWidth = healthBarWidth * healthPercent;

                // Color the health bar from red (low health) to green (full health)
                Color healthColor = {1.0f - healthPercent, healthPercent, 0.0f, 1.0f};

                renderer.renderRectangle(
                    {drawX - healthBarWidth / 2, drawY - enemySize / 2 - 10.0f * ((scaleX + scaleY) / 2.0f),
                     healthBarFillWidth, healthBarHeight},
                    {healthColor.r, healthColor.g, healthColor.b, healthColor.a});
            }
        }

        // Draw tower range indicators using rectangles
        for (const auto &tower : towers)
        {
            float drawX = tower.pos.x * scaleX;
            float drawY = tower.pos.y * scaleY;
            float drawRangeX = tower.range * scaleX;
            float drawRangeY = tower.range * scaleY;
            // Draw a cross shape to indicate range
            renderer.renderRectangle(
                {drawX - drawRangeX, drawY - 2, drawRangeX * 2, 4},
                {1.0f, 1.0f, 1.0f, 0.2f} // Semi-transparent white
            );
            renderer.renderRectangle(
                {drawX - 2, drawY - drawRangeY, 4, drawRangeY * 2},
                {1.0f, 1.0f, 1.0f, 0.2f} // Semi-transparent white
            );
        }

        // Flush renderer (draw everything)
        renderer.flush();

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


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

// Window size
const int GAME_WIDTH = 640;
const int PANEL_WIDTH = 200;  
const int WIDTH = GAME_WIDTH + PANEL_WIDTH;
const int HEIGHT = 640;

// Structure to hold 2D points
struct Point {
    float x, y;
    Point(float _x, float _y) : x(_x), y(_y) {}
};

// Structure to hold color
struct Color {
    float r, g, b, a;
    Color(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
};

// Enemy types
enum class EnemyType {
    SKELETON,   // Blue, fast enemy
    ZOMBIE,      // Red, normal enemy
    BOSS,        // Large, slow enemy with lots of health
    TANK,        // Slow, high health enemy
    GHOST       // Very fast, low health enemy
};

// Tower types
enum class TowerType {
    NONE,       // No tower selected
    APPLE,      // Balanced tower
    CARROT,     // Long range tower
    POTATO,     // Fast attack speed, low range
    PINEAPPLE,  // Area tower that shoots in 8 directions
    BANANA_PEEL, // Special trap that can only be placed on path
    CACTUS      // trap that does low damage but stays forever
};

// Tower costs
const int BASE_APPLE_COST = 15;
const int BASE_CARROT_COST = 25;
const int BASE_POTATO_COST = 20;
const int BASE_PINEAPPLE_COST = 30;
const int BASE_BANANA_PEEL_COST = 5;  // Cheap trap tower
const int BASE_CACTUS_COST = 35;      // Expensive high damage tower

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
int lives = 3;  // Number of enemies that can exit before game over
bool isPaused = false;  // Track if game is paused

// Global variable for tracking when mouse was just pressed
bool mouseJustPressed = false;

// Tower struct
struct Tower {
    Point pos;
    bool isPlaced;
    float shootTimer;
    float range;
    float damage;
    float attackSpeed;  // Shots per second
    float projectileSpeed;
    TowerType type;
    bool isUsed;  // For one-time traps like banana peels
    int usesLeft; // For multi-use traps like cactus
    int damageUpgradeLevel;    // Track damage upgrade level
    int attackSpeedUpgradeLevel; // Track attack speed upgrade level
    int rangeUpgradeLevel;     // Track range upgrade level
    
    Tower() : pos(0, 0), isPlaced(false), shootTimer(0), range(150.0f), 
              damage(10.0f), attackSpeed(1.0f), projectileSpeed(300.0f), 
              type(TowerType::NONE), isUsed(false), usesLeft(0), 
              damageUpgradeLevel(0), attackSpeedUpgradeLevel(0), rangeUpgradeLevel(0) {}
              
    Tower(float x, float y) : pos(x, y), isPlaced(true), shootTimer(0), 
                              range(150.0f), damage(10.0f), attackSpeed(1.0f), 
                              projectileSpeed(300.0f), type(TowerType::NONE), 
                              isUsed(false), usesLeft(0),
                              damageUpgradeLevel(0), attackSpeedUpgradeLevel(0), rangeUpgradeLevel(0) {}
                              
    void setType(TowerType newType) {
        type = newType;
        damageUpgradeLevel = 0;
        attackSpeedUpgradeLevel = 0;
        rangeUpgradeLevel = 0;
        switch(type) {
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
    int getUpgradeCost(TowerType type, int currentLevel) {
        if (currentLevel >= 3) return -1;  // Max level reached
        if (type == TowerType::BANANA_PEEL) return -1;  // No upgrades for banana peel
        
        // Get base cost for tower type
        int baseCost;
        switch(type) {
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
        
        return (baseCost * (currentLevel + 1)) / 2;  // Each upgrade costs 50% more than the last
    }

    // Apply upgrade effects
    void upgradeDamage() {
        if (damageUpgradeLevel >= 3) return;  // Max level reached
        if (type == TowerType::BANANA_PEEL) return;  // No upgrades for banana peel
        
        damageUpgradeLevel++;
        damage *= 1.3f;  // +30% damage per level
    }

    void upgradeAttackSpeed() {
        if (attackSpeedUpgradeLevel >= 3) return;  // Max level reached
        if (type == TowerType::BANANA_PEEL) return;  // No upgrades for banana peel
        
        attackSpeedUpgradeLevel++;
        attackSpeed *= 1.2f;  // +20% attack speed per level
    }

    void upgradeRange() {
        if (rangeUpgradeLevel >= 3) return;  // Max level reached
        if (type == TowerType::BANANA_PEEL) return;  // No upgrades for banana peel
        
        rangeUpgradeLevel++;
        range *= 1.15f;  // +15% range per level
        projectileSpeed *= 1.15f;  // Also increase projectile speed by 15%
    }
};

// Projectile struct
struct Projectile {
    Point pos;
    Point velocity;
    bool active;
    float speed;
    float damage;
    float distanceTraveled;
    float maxDistance;
    TowerType sourceType;  // Track which tower type fired this projectile
    
    Projectile() : pos(0, 0), velocity(0, 0), active(false), speed(450.0f), damage(10.0f), 
                   distanceTraveled(0.0f), maxDistance(300.0f), sourceType(TowerType::NONE) {}
    
    void update(float deltaTime) {
        if (!active) return;
        
        // Calculate movement
        float moveX = velocity.x * speed * deltaTime;
        float moveY = velocity.y * speed * deltaTime;
        
        // Update position
        pos.x += moveX;
        pos.y += moveY;
        
        // Track distance traveled
        distanceTraveled += std::sqrt(moveX * moveX + moveY * moveY);
        
        // Deactivate if maximum distance reached
        if (distanceTraveled >= maxDistance) {
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
const float PROJECTILE_SIZE = 20.0f;  // Increased from 10 for better visibility

// Tower colors for different types
const Color APPLE_TOWER_COLOR = {1.0f, 0.2f, 0.2f, 1.0f};      // Red for apple
const Color CARROT_TOWER_COLOR = {1.0f, 0.4f, 0.0f, 1.0f};     // Orange for carrot
const Color POTATO_TOWER_COLOR = {1.0f, 1.0f, 0.0f, 1.0f};     // Yellow for potato (was RAPID)
const Color PINEAPPLE_TOWER_COLOR = {1.0f, 0.9f, 0.4f, 1.0f};  // Yellow-orange for pineapple
const Color BANANA_PEEL_COLOR = {0.9f, 0.8f, 0.2f, 1.0f};      // Bright yellow for banana peel
const Color CACTUS_TOWER_COLOR = {0.0f, 0.8f, 0.4f, 1.0f};     // Green for cactus

// Colors for the UI
const Color UI_BACKGROUND = {0.2f, 0.2f, 0.2f, 1.0f};
const Color UI_SELECTED = {0.4f, 0.4f, 0.4f, 1.0f};
const Color UI_TEXT = {1.0f, 1.0f, 1.0f, 1.0f};

// Rectangle structure for UI elements
struct Rectangle {
    float x, y, w, h;
    Rectangle(float _x, float _y, float _w, float _h) : x(_x), y(_y), w(_w), h(_h) {}
    Rectangle() : x(0), y(0), w(0), h(0) {}
};

// Tower menu structure
struct TowerMenu {
    bool isOpen;
    Tower* selectedTower;
    Rectangle sellButton;
    Rectangle upgradeButton1;  // First upgrade option
    Rectangle upgradeButton2;  // Second upgrade option
    Rectangle upgradeButton3;  // Third upgrade option
    Rectangle closeButton;
    
    TowerMenu() : isOpen(false), selectedTower(nullptr) {
        // Initialize button positions (will be updated when menu opens)
        sellButton = Rectangle(0, 0, 160, 40);
        upgradeButton1 = Rectangle(0, 0, 160, 40);
        upgradeButton2 = Rectangle(0, 0, 160, 40);
        upgradeButton3 = Rectangle(0, 0, 160, 40);
        closeButton = Rectangle(0, 0, 30, 30);
    }
    
    void open(Tower* tower) {
        isOpen = true;
        selectedTower = tower;
        
        // Position buttons in the tower selection panel area with increased spacing
        upgradeButton1.x = GAME_WIDTH + 20;
        upgradeButton1.y = 170;  // First upgrade option (moved lower)
        upgradeButton1.w = 180;  // Make buttons wider
        
        upgradeButton2.x = GAME_WIDTH + 20;
        upgradeButton2.y = 260;  // Second upgrade option (moved lower)
        upgradeButton2.w = 180;  // Make buttons wider
        
        upgradeButton3.x = GAME_WIDTH + 20;
        upgradeButton3.y = 350;  // Third upgrade option (moved lower)
        upgradeButton3.w = 180;  // Make buttons wider
        
        sellButton.x = GAME_WIDTH + 20;
        sellButton.y = HEIGHT - 60;  // Near bottom of screen
        
        closeButton.x = GAME_WIDTH + 150;  // Right side of panel
        closeButton.y = HEIGHT - 60;
    }
    
    void close() {
        isOpen = false;
        selectedTower = nullptr;
    }
};

// Tower selection UI
struct TowerButton {
    Rectangle rect;
    TowerType type;
    bool isHovered;
    
    TowerButton(float x, float y, float w, float h, TowerType t) 
        : rect{x, y, w, h}, type(t), isHovered(false) {}
};

// Function to get enemy name
std::string getEnemyName(EnemyType type) {
    switch (type) {
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
    Point(352, 0), //starting point
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
    Point(-69, 544) //end point
};

// Function to calculate distance between two points
float distance(const Point& a, const Point& b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

// Define water region (top-right corner)
const bool isInWaterRegion(const Point& p) {
    // Blue water is approximately in the top-right corner of the map
    return p.x > 500 && p.y < 120;
}

// Function to check if a point is too close to the path
bool isNearPath(const Point& p, float threshold = 28.0f) {
    // Check if the point is near any path segment
    for (size_t i = 0; i < waypoints.size() - 1; ++i) {
        const Point& a = waypoints[i];
        const Point& b = waypoints[i + 1];
        
        // Calculate distance from point to line segment
        float lineLength = distance(a, b);
        if (lineLength < 0.1f) continue; // Skip very short segments
        
        // Vector from a to b
        float abx = b.x - a.x;
        float aby = b.y - a.y;
        
        // Vector from a to p
        float apx = p.x - a.x;
        float apy = p.y - a.y;
        
        // Project ap onto ab to find the closest point
        float projection = (apx * abx + apy * aby) / (abx * abx + aby * aby);
        
        // Clamp projection to [0,1] to get point on segment
        float clampedProjection = std::max(0.0f, std::min(1.0f, projection));
        
        // Calculate closest point on segment
        float closestX = a.x + clampedProjection * abx;
        float closestY = a.y + clampedProjection * aby;
        
        // Calculate distance from p to closest point
        float dist = std::sqrt((p.x - closestX) * (p.x - closestX) + 
                              (p.y - closestY) * (p.y - closestY));
        
        // If distance is less than threshold, point is near the path
        if (dist < threshold) {
            return true;
        }
    }
    return false;
}

// Structure for enemies
struct Enemy {
    Point pos;
    int currentWaypoint;
    bool isActive;
    EnemyType type;
    std::string name;
    float health;
    float maxHealth;
    
    Enemy() : pos(waypoints[0]), currentWaypoint(0), isActive(false), type(EnemyType::ZOMBIE),
              health(0.0f), maxHealth(0.0f) {
        name = getEnemyName(type);
    }
    
    void setType(EnemyType newType) {
        type = newType;
        name = getEnemyName(type);
        
        // Base health values - starting lower for easier early game
        float baseHealth = 0.0f;
        
        // Set health based on enemy type
        switch(type) {
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
                baseHealth = 100.0f;  // High health but not as much as boss
                break;
            case EnemyType::GHOST:
                baseHealth = 10.0f;   // Half of skeleton's health
                break;
        }
        
        // Health scales with round number - less aggressive scaling
        float healthMultiplier = 1.0f;
        if (currentRound > 1) {
            // Linear scaling with a smaller increase per round
            // Round 1: 1.0x, Round 2: 1.1x, Round 3: 1.2x, Round 4: 1.3x, etc.
            healthMultiplier = 1.0f + ((currentRound - 1) * 0.1f);
        }
        
        maxHealth = baseHealth * healthMultiplier;
        health = maxHealth;
    }
};

// Game settings
const float SPAWN_INTERVAL = 0.7f;    // Seconds between enemy spawns
const int MAX_ENEMIES = 20;            // Maximum number of enemies at once
const float ZOMBIE_SPEED = 60.0f;     // Zombie speed (reduced from 100.0f)
const float SKELETON_SPEED = 120.0f;  // Skeleton speed (reduced from 200.0f)
const float BOSS_SPEED = 20.0f;       // Boss speed (slower, reduced from 40.0f)
const float TANK_SPEED = 30.0f;       // Tank speed (slowest)
const float GHOST_SPEED = 160.0f;     // Ghost speed (fastest)

// Bean rewards for killing enemies
const int ZOMBIE_BEANS = 1;    // Beans for killing a zombie
const int SKELETON_BEANS = 2;  // Beans for killing a skeleton (more beans as they're faster)
const int BOSS_BEANS = 30;     // Beans for killing a boss (significantly more)
const int TANK_BEANS = 5;      // Beans for killing a tank (more than zombie but less than boss)
const int GHOST_BEANS = 3;     // Beans for killing a ghost (more than skeleton)

// Function to get enemy color based on type
Color getEnemyColor(EnemyType type) {
    switch (type) {
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
float getEnemySpeed(EnemyType type) {
    // Base speeds - lower than original speeds to make early game easier
    float baseSpeed = 0.0f;
    
    // Get base speed for enemy type
    switch (type) {
        case EnemyType::SKELETON:
            baseSpeed = 80.0f;  // Reduced from 120.0f
            break;
        case EnemyType::BOSS:
            baseSpeed = 15.0f;  // Very slow, reduced from 30.0f
            break;
        case EnemyType::TANK:
            baseSpeed = 20.0f;  // Very slow
            break;
        case EnemyType::GHOST:
            baseSpeed = 100.0f; // Very fast
            break;
        case EnemyType::ZOMBIE:
        default:
            baseSpeed = 40.0f;  // Reduced from 60.0f
            break;
    }
    
    // Calculate speed multiplier based on round number
    float speedMultiplier = 1.0f;
    if (currentRound > 1) {
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
        glfwSetWindowShouldClose(window, true);
        std::cout << "Closing game..." << std::endl;
    }
    
    // Get mouse position
    glfwGetCursorPos(window, &mouseX, &mouseY);
    
    // Get mouse button state
    if (!mouseJustPressed) {  // Only check regular state if not just pressed
        mouseLeftPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    }
    
    // Pause handling
    static bool pKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pKeyPressed) {
        isPaused = !isPaused;
        pKeyPressed = true;
        if (isPaused) {
            std::cout << "Game PAUSED. Press any key to resume." << std::endl;
        } else {
            std::cout << "Game RESUMED." << std::endl;
        }
    }
    else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        pKeyPressed = false;
    }
    
    // If game is paused, check for any key press to unpause
    if (isPaused) {
        // Check for any key press except P (which is handled above)
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++) {
            if (key != GLFW_KEY_P && key != GLFW_KEY_ESCAPE && glfwGetKey(window, key) == GLFW_PRESS) {
                isPaused = false;
                std::cout << "Game RESUMED." << std::endl;
                break;
            }
        }
    }
    
    // Tower type selection
    static bool key1Pressed = false;
    static bool key2Pressed = false;
    static bool key3Pressed = false;
    static bool key4Pressed = false;
    static bool key5Pressed = false;
    static bool key6Pressed = false;
    
    // Only allow tower selection via keyboard if the tower menu is closed
    if (!towerMenu.isOpen) {
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !key1Pressed) {
            placementTower.setType(TowerType::APPLE);
            std::cout << "Selected Apple Tower" << std::endl;
            key1Pressed = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE) {
            key1Pressed = false;
        }
        
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !key2Pressed) {
            placementTower.setType(TowerType::CARROT);
            std::cout << "Selected Carrot Tower" << std::endl;
            key2Pressed = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE) {
            key2Pressed = false;
        }
        
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !key3Pressed) {
            placementTower.setType(TowerType::POTATO);
            std::cout << "Selected Potato Tower" << std::endl;
            key3Pressed = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE) {
            key3Pressed = false;
        }
        
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !key4Pressed) {
            placementTower.setType(TowerType::PINEAPPLE);
            std::cout << "Selected Pineapple Tower" << std::endl;
            key4Pressed = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE) {
            key4Pressed = false;
        }
        
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS && !key5Pressed) {
            placementTower.setType(TowerType::BANANA_PEEL);
            std::cout << "Selected Banana Peel" << std::endl;
            key5Pressed = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_5) == GLFW_RELEASE) {
            key5Pressed = false;
        }
        
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS && !key6Pressed) {
            placementTower.setType(TowerType::CACTUS);
            std::cout << "Selected Cactus Tower" << std::endl;
            key6Pressed = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_6) == GLFW_RELEASE) {
            key6Pressed = false;
        }
    } else {
        // Still need to track key releases even if tower menu is open
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE) key1Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE) key2Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE) key3Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE) key4Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_RELEASE) key5Pressed = false;
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_RELEASE) key6Pressed = false;
    }
}

// Mouse button callback
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        mouseLeftPressed = true;
        mouseJustPressed = true;  // Flag the initial press
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        mouseLeftPressed = false;
    }
}

// Function to update enemy position
void updateEnemy(Enemy& enemy, float deltaTime, const std::vector<Tower>& towers, int& beanCount) {
    if (!enemy.isActive || enemy.currentWaypoint >= waypoints.size() - 1) return;

    Point& target = waypoints[enemy.currentWaypoint + 1];
    float dx = target.x - enemy.pos.x;
    float dy = target.y - enemy.pos.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    
    float speed = getEnemySpeed(enemy.type);
    
    if (dist > 1.0f) {
        float moveX = (dx / dist) * speed * deltaTime;
        float moveY = (dy / dist) * speed * deltaTime;
        enemy.pos.x += moveX;
        enemy.pos.y += moveY;
        
        // Check for banana peel traps and cactus traps
        for (const auto& tower : towers) {
            if (tower.type == TowerType::BANANA_PEEL && !tower.isUsed) {
                if (distance(enemy.pos, tower.pos) < ENEMY_SIZE / 2) {
                    // Enemy stepped on a banana peel!
                    // Ghosts are immune to banana peel damage
                    if (enemy.type != EnemyType::GHOST) {
                        enemy.health -= tower.damage;  // Deal damage instantly

                        // Mark the banana peel as used
                        Tower& actualTower = const_cast<Tower&>(tower);
                        actualTower.isUsed = true;
                        
                        // If enemy is killed by banana peel
                        if (enemy.health <= 0) {
                            enemy.isActive = false;
                            // Add beans based on enemy type
                            beanCount += (enemy.type == EnemyType::ZOMBIE) ? ZOMBIE_BEANS : 
                                         (enemy.type == EnemyType::SKELETON) ? SKELETON_BEANS : 
                                         (enemy.type == EnemyType::TANK) ? TANK_BEANS : BOSS_BEANS;
                        }
                    }
                    // Ghost passes through banana peel silently
                }
            }
            else if (tower.type == TowerType::CACTUS && tower.usesLeft != 0) {
                if (distance(enemy.pos, tower.pos) < ENEMY_SIZE / 2) {
                    // Enemy is on a cactus - deal small continuous damage
                    // Ghosts are immune to cactus damage
                    if (enemy.type != EnemyType::GHOST) {
                        enemy.health -= tower.damage * deltaTime;  // Deal damage per second
                        
                        // If enemy is killed by cactus
                        if (enemy.health <= 0) {
                            enemy.isActive = false;
                            // Add beans based on enemy type
                            beanCount += (enemy.type == EnemyType::ZOMBIE) ? ZOMBIE_BEANS : 
                                         (enemy.type == EnemyType::SKELETON) ? SKELETON_BEANS : 
                                         (enemy.type == EnemyType::TANK) ? TANK_BEANS : BOSS_BEANS;
                        }
                    }
                    // Ghost passes through cactus silently
                }
            }
        }
    } else {
        enemy.currentWaypoint++;
        if (enemy.currentWaypoint >= waypoints.size() - 1) {
            enemy.isActive = false;
            
            // Enemy reached the exit - reduce lives!
            lives--;
            
            // Check for game over
            if (lives <= 0) {
                isGameOver = true;
                std::cout << "GAME OVER! Enemies reached the exit!" << std::endl;
            } else {
                std::cout << "Enemy escaped! Lives remaining: " << lives << std::endl;
            }
        }
    }
}

// Function to check if any part of a tower overlaps with the path or water
bool canPlaceTower(const Point& center, float size, TowerType type = TowerType::APPLE) {
    // First check if tower is in the game area
    if (center.x > GAME_WIDTH - size/2) {
        return false;  // Tower is in or too close to the UI panel
    }

    // Special case for Banana Peel and Cactus
    if (type == TowerType::BANANA_PEEL || type == TowerType::CACTUS) {
        // These types can ONLY be placed on the path
        return isNearPath(center, 20.0f);  // Must be within 20 pixels of path center
    }

    // Check multiple points on the tower including corners and more edge points
    float halfSize = size / 2.0f;
    float quarterSize = size / 4.0f;
    
    // Create a more detailed grid of points to check
    std::vector<Point> checkPoints = {
        center, // Center
        
        // Four corners
        Point(center.x - halfSize, center.y - halfSize), // Top-left
        Point(center.x + halfSize, center.y - halfSize), // Top-right
        Point(center.x - halfSize, center.y + halfSize), // Bottom-left
        Point(center.x + halfSize, center.y + halfSize), // Bottom-right
        
        // Middle of each edge
        Point(center.x - halfSize, center.y), // Left middle
        Point(center.x + halfSize, center.y), // Right middle
        Point(center.x, center.y - halfSize), // Top middle
        Point(center.x, center.y + halfSize), // Bottom middle
        
        // Quarter points on each edge
        Point(center.x - halfSize, center.y - quarterSize), // Left edge 1/4
        Point(center.x - halfSize, center.y + quarterSize), // Left edge 3/4
        Point(center.x + halfSize, center.y - quarterSize), // Right edge 1/4
        Point(center.x + halfSize, center.y + quarterSize), // Right edge 3/4
        Point(center.x - quarterSize, center.y - halfSize), // Top edge 1/4
        Point(center.x + quarterSize, center.y - halfSize), // Top edge 3/4
        Point(center.x - quarterSize, center.y + halfSize), // Bottom edge 1/4
        Point(center.x + quarterSize, center.y + halfSize)  // Bottom edge 3/4
    };
    
    // Check if any of the points are in a restricted area
    for (const auto& point : checkPoints) {
        if (isInWaterRegion(point) || isNearPath(point)) {
            return false;
        }
    }
    
    return true;
}

// Initialize projectiles vector (add this after tower initialization)
std::vector<Projectile> projectiles(100); // Pool of projectiles

// Function to find closest enemy to a point
Enemy* findClosestEnemy(const Point& pos, float range, const std::vector<Enemy>& enemies) {
    Enemy* closest = nullptr;
    float minDist = range;
    
    for (const auto& enemy : enemies) {
        if (!enemy.isActive) continue;
        
        float dist = distance(pos, enemy.pos);
        if (dist < minDist) {
            minDist = dist;
            closest = const_cast<Enemy*>(&enemy);
        }
    }
    
    return closest;
}

// Function to spawn a projectile
void spawnProjectile(std::vector<Projectile>& projectiles, const Point& start, const Point& target, const Tower& tower) {
    for (auto& proj : projectiles) {
        if (!proj.active) {
            proj.active = true;
            proj.pos = start;
            proj.damage = tower.damage;
            proj.speed = tower.projectileSpeed;
            proj.distanceTraveled = 0.0f;  // Reset distance traveled
            proj.maxDistance = tower.range * 1.2f;  // Set max distance based on tower range
            proj.sourceType = tower.type;  // Set the source tower type
            
            // Calculate direction to target
            float dx = target.x - start.x;
            float dy = target.y - start.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            // Normalize direction
            if (dist > 0) {
                proj.velocity.x = dx / dist;
                proj.velocity.y = dy / dist;
            }
            
            return;
        }
    }
}

// Function to spawn projectiles in 8 directions
void spawnProjectilesInAllDirections(std::vector<Projectile>& projectiles, const Point& start, const Tower& tower) {
    // 8 directions: N, NE, E, SE, S, SW, W, NW
    const int numDirections = 8;
    const float angleStep = 2.0f * 3.14159f / numDirections;  // 45 degrees in radians
    
    for (int i = 0; i < numDirections; i++) {
        float angle = i * angleStep;
        float dx = cos(angle);
        float dy = sin(angle);
        
        // Find available projectile
        for (auto& proj : projectiles) {
            if (!proj.active) {
                proj.active = true;
                proj.pos = start;
                proj.damage = tower.damage;
                proj.speed = tower.projectileSpeed;
                proj.distanceTraveled = 0.0f;  // Reset distance traveled
                proj.maxDistance = tower.range * 1.2f;  // Set max distance based on tower range
                proj.sourceType = tower.type;  // Set the source tower type
                proj.velocity.x = dx;
                proj.velocity.y = dy;
                break;
            }
        }
    }
}

// Function to check if point is inside rectangle
bool isPointInRect(float px, float py, const Rectangle& rect) {
    return px >= rect.x && px <= rect.x + rect.w &&
           py >= rect.y && py <= rect.y + rect.h;
}

// Function to get tower type name
std::string getTowerTypeName(TowerType type) {
    switch(type) {
        case TowerType::NONE: return "None";
        case TowerType::APPLE: return "Apple Tower";
        case TowerType::CARROT: return "Carrot Tower";
        case TowerType::POTATO: return "Potato Tower";
        case TowerType::PINEAPPLE: return "Pineapple Tower";
        case TowerType::BANANA_PEEL: return "Banana Peel";
        case TowerType::CACTUS: return "Cactus Tower";
        default: return "Unknown";
    }
}

// Function to get tower cost
int getTowerCost(TowerType type) {
    float costMultiplier = 1.0f;
    int baseCost = 0;
    
    switch(type) {
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
std::string getTowerStats(TowerType type) {
    Tower temp;
    temp.setType(type);
    return "Cost: " + std::to_string(getTowerCost(type)) + " beans" +
           "\nRange: " + std::to_string(int(temp.range)) +
           "\nDamage: " + std::to_string(int(temp.damage)) +
           "\nSpeed: " + std::to_string(temp.attackSpeed) + "/s";
}

// Function to draw a digit (0-9) using rectangles
void drawDigit(gl2d::Renderer2D& renderer, int digit, float x, float y, float size) {
    const float thickness = size / 5.0f;
    const float gap = thickness / 2.0f;

    // Segments for each number (1 = draw, 0 = don't draw)
    // Order: top, top-right, bottom-right, bottom, bottom-left, top-left, middle
    const bool segments[10][7] = {
        {1,1,1,1,1,1,0}, // 0
        {0,1,1,0,0,0,0}, // 1
        {1,1,0,1,1,0,1}, // 2
        {1,1,1,1,0,0,1}, // 3
        {0,1,1,0,0,1,1}, // 4
        {1,0,1,1,0,1,1}, // 5
        {1,0,1,1,1,1,1}, // 6
        {1,1,1,0,0,0,0}, // 7
        {1,1,1,1,1,1,1}, // 8
        {1,1,1,1,0,1,1}  // 9
    };

    // Draw segments based on the digit
    if (digit >= 0 && digit <= 9) {
        // Top
        if (segments[digit][0])
            renderer.renderRectangle(
                {x + gap, y, size - 2*gap, thickness},
                {1.0f, 1.0f, 1.0f, 1.0f}
            );
        // Top-right
        if (segments[digit][1])
            renderer.renderRectangle(
                {x + size - thickness, y + gap, thickness, size/2 - gap},
                {1.0f, 1.0f, 1.0f, 1.0f}
            );
        // Bottom-right
        if (segments[digit][2])
            renderer.renderRectangle(
                {x + size - thickness, y + size/2, thickness, size/2 - gap},
                {1.0f, 1.0f, 1.0f, 1.0f}
            );
        // Bottom
        if (segments[digit][3])
            renderer.renderRectangle(
                {x + gap, y + size - thickness, size - 2*gap, thickness},
                {1.0f, 1.0f, 1.0f, 1.0f}
            );
        // Bottom-left
        if (segments[digit][4])
            renderer.renderRectangle(
                {x, y + size/2, thickness, size/2 - gap},
                {1.0f, 1.0f, 1.0f, 1.0f}
            );
        // Top-left
        if (segments[digit][5])
            renderer.renderRectangle(
                {x, y + gap, thickness, size/2 - gap},
                {1.0f, 1.0f, 1.0f, 1.0f}
            );
        // Middle
        if (segments[digit][6])
            renderer.renderRectangle(
                {x + gap, y + size/2 - thickness/2, size - 2*gap, thickness},
                {1.0f, 1.0f, 1.0f, 1.0f}
            );
    }
}

// Function to draw a number using digits
void drawNumber(gl2d::Renderer2D& renderer, int number, float x, float y, float digitSize) {
    std::string numStr = std::to_string(number);
    float spacing = digitSize * 1.2f;  // Add some space between digits
    
    // Draw background
    float bgWidth = spacing * numStr.length() + digitSize;
    renderer.renderRectangle(
        {x - 10, y - 5, bgWidth + 20, digitSize + 10},
        {0.0f, 0.0f, 0.0f, 0.5f}
    );

    // Draw bean icon
    renderer.renderRectangle(
        {x, y + digitSize/4, digitSize/2, digitSize/2},
        {0.6f, 0.4f, 0.2f, 1.0f}
    );

    // Draw each digit
    for (size_t i = 0; i < numStr.length(); i++) {
        drawDigit(renderer, numStr[i] - '0', x + digitSize + i * spacing, y, digitSize);
    }
}

// Function to get tower color based on type
Color getTowerColor(TowerType type) {
    switch(type) {
        case TowerType::APPLE: return APPLE_TOWER_COLOR;
        case TowerType::CARROT: return CARROT_TOWER_COLOR;
        case TowerType::POTATO: return POTATO_TOWER_COLOR;
        case TowerType::PINEAPPLE: return PINEAPPLE_TOWER_COLOR;
        case TowerType::BANANA_PEEL: return BANANA_PEEL_COLOR;
        case TowerType::CACTUS: return CACTUS_TOWER_COLOR;
        default: return APPLE_TOWER_COLOR;
    }
}

// Function to calculate number of enemies for a round
int getEnemiesForRound(int round) {
    // Start with 5 enemies in round 1, increase by 2 each round
    return 5 + (round - 1) * 2;
}

// Function to determine skeleton percentage for a round
float getSkeletonPercentage(int round) {
    // Start with 0% skeletons, increase by 10% each round, max 70%
    float percentage = (round - 1) * 0.1f;
    return std::min(percentage, 0.7f);
}

// Function to determine tank percentage for a round
float getTankPercentage(int round) {
    // Start with 0% tanks, increase by 5% each round, max 30%
    float percentage = (round - 1) * 0.05f;
    return std::min(percentage, 0.3f);
}

// Function to check if the round is a boss round
bool isBossRound(int round) {
    // Boss appears every 10 rounds (rounds 10, 20, 30, etc.)
    return round % 10 == 0 && round > 0;
}

// Function to start a new round
void startNewRound(std::vector<Enemy>& enemies, int& round) {
    round++;
    enemiesLeftInRound = getEnemiesForRound(round);
    
    // Add one extra enemy for the boss on boss rounds
    if (isBossRound(round)) {
        enemiesLeftInRound += 1;
    }
    
    isRoundActive = true;
    enemySpawnTimer = 0.0f;
    
    std::cout << "Round " << round << " starting! Enemies: " << enemiesLeftInRound;
    if (isBossRound(round)) {
        std::cout << " (including a BOSS!)";
    }
    std::cout << std::endl;
}

// Function to check if round is complete
bool isRoundComplete(const std::vector<Enemy>& enemies) {
    // Round is complete when all enemies have been spawned and none are active
    if (enemiesLeftInRound > 0) {
        return false;
    }
    
    for (const auto& enemy : enemies) {
        if (enemy.isActive) {
            return false;
        }
    }
    
    return true;
}

// Function to reset the game state
void resetGame(std::vector<Enemy>& enemies, std::vector<Tower>& towers, std::vector<Projectile>& projectiles, int& beanCount) {
    // Reset enemies
    for (auto& enemy : enemies) {
        enemy.isActive = false;
    }
    
    // Clear all towers
    towers.clear();
    
    // Clear all projectiles
    for (auto& proj : projectiles) {
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
void renderCactus(gl2d::Renderer2D& renderer, const Tower& tower) {
    Color towerColor = getTowerColor(tower.type);
    float cactusSize = TOWER_SIZE * 0.8f;
    
    // Draw main green cactus body
    renderer.renderRectangle(
        {tower.pos.x - cactusSize/4, tower.pos.y - cactusSize/2, cactusSize/2, cactusSize},
        {towerColor.r, towerColor.g, towerColor.b, towerColor.a}
    );
    
    // Draw a smaller arm on the side
    renderer.renderRectangle(
        {tower.pos.x - cactusSize/4, tower.pos.y - cactusSize/4, cactusSize/2, cactusSize/3},
        {towerColor.r, towerColor.g, towerColor.b, towerColor.a}
    );
    
    // Draw some spines/spikes using small rectangles
    Color spikeColor(1.0f, 1.0f, 1.0f, 0.9f);
    
    // Always draw 5 spikes since cactus is permanent
    for (int i = 0; i < 5; i++) {
        float spikeX = tower.pos.x + ((i % 2) * 2 - 1) * cactusSize/3;
        float spikeY = tower.pos.y - cactusSize/2 + (i / 2) * cactusSize/2;
        renderer.renderRectangle(
            {spikeX, spikeY, 3, 3},
            {spikeColor.r, spikeColor.g, spikeColor.b, spikeColor.a}
        );
    }
}

// Function to determine ghost percentage for a round
float getGhostPercentage(int round) {
    // Start with 0% ghosts, increase by 7% each round, max 25%
    float percentage = (round - 1) * 0.07f;
    return std::min(percentage, 0.25f);
}

// Function to get upgrade description based on tower type and upgrade type
std::string getUpgradeDescription(TowerType type, const std::string& upgradeType, int level) {
    if (type == TowerType::BANANA_PEEL) return "No upgrades available";
    
    if (upgradeType == "damage") {
        return "Damage Upgrade: +30% (Level " + std::to_string(level) + "/3)";
    } else if (upgradeType == "attackSpeed") {
        return "Attack Speed Upgrade: +20% (Level " + std::to_string(level) + "/3)";
    } else if (upgradeType == "range") {
        return "Range Upgrade: +15% (Level " + std::to_string(level) + "/3)";
    }
    return "";
}

// Game constants are already defined earlier in the file

//Tower textures
//gl2d::Texture appleTowerTexture;
gl2d::Texture carrotTowerTexture;
//gl2d::Texture potatoTowerTexture;
//gl2d::Texture pineappleTowerTexture;

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

// Function to create simple colored texture if it doesn't exist
void createSimpleTexture(const std::string& filename, const Color& color) {
    // Only create if file doesn't exist
    if (std::filesystem::exists(filename)) {
        return;
    }
    
    // Create a simple 32x32 colored image
    const int width = 32;
    const int height = 32;
    std::vector<unsigned char> pixels(width * height * 4, 0);
    
    // Fill with color, making a circular shape
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * 4;
            
            // Calculate distance from center for circular shape
            float dx = x - width/2;
            float dy = y - height/2;
            float distance = std::sqrt(dx*dx + dy*dy);
            
            // If within radius, set pixel color
            if (distance <= width/2) {
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
    if (!std::filesystem::exists("resources")) {
        std::filesystem::create_directory("resources");
    }

    // Create projectile textures if they don't exist
    createSimpleTexture("resources/apple.png", Color(1.0f, 0.2f, 0.2f, 1.0f));      // Red apple
    createSimpleTexture("resources/carrot.png", Color(1.0f, 0.5f, 0.0f, 1.0f));   // Orange carrot
    createSimpleTexture("resources/potato.png", Color(0.6f, 0.4f, 0.2f, 1.0f));   // Brown potato
    createSimpleTexture("resources/pineapple.png", Color(0.8f, 0.8f, 0.0f, 1.0f)); // Yellow pineapple
}

// Function to create all enemy textures
void createEnemyTextures()
{
    // Ensure resources directory exists
    if (!std::filesystem::exists("resources")) {
        std::filesystem::create_directory("resources");
    }

    // Create enemy textures if they don't exist
    createSimpleTexture("resources/zombie.png", getEnemyColor(EnemyType::ZOMBIE));
    createSimpleTexture("resources/skeleton.png", getEnemyColor(EnemyType::SKELETON));
    createSimpleTexture("resources/boss.png", getEnemyColor(EnemyType::BOSS));
    createSimpleTexture("resources/tank.png", getEnemyColor(EnemyType::TANK));
    createSimpleTexture("resources/ghost.png", getEnemyColor(EnemyType::GHOST));
}

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

    // Load background texture
    gl2d::Texture backgroundTexture;
    
    // Check if file exists
    if (!std::filesystem::exists("resources/background.png")) {
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
    
    // Create projectile textures if they don't exist
    createSimpleTexture("resources/apple.png", Color(1.0f, 0.2f, 0.2f, 1.0f)); // Red apple
    createSimpleTexture("resources/carrot.png", Color(1.0f, 0.5f, 0.0f, 1.0f)); // Orange carrot
    createSimpleTexture("resources/potato.png", Color(0.6f, 0.4f, 0.2f, 1.0f)); // Brown potato
    createSimpleTexture("resources/pineapple.png", Color(0.8f, 0.8f, 0.0f, 1.0f)); // Yellow pineapple
    
    // Load projectile textures
    std::string applePath = "resources/apple.png";
    appleTexture.loadFromFile(applePath.c_str());
    if (appleTexture.id == 0) {
        applePath = "../resources/apple.png";
        appleTexture.loadFromFile(applePath.c_str());
    }
    
    std::string carrotPath = "resources/carrot.png";
    carrotTexture.loadFromFile(carrotPath.c_str());
    if (carrotTexture.id == 0) {
        carrotPath = "../resources/carrot.png";
        carrotTexture.loadFromFile(carrotPath.c_str());
    }

    std::string carrotTowerPath = "resources/carrotTower.png";
    carrotTowerTexture.loadFromFile(carrotTowerPath.c_str());
    if (carrotTowerTexture.id == 0) {
        carrotTowerPath = "../resources/carrotTower.png";
        carrotTowerTexture.loadFromFile(carrotTowerPath.c_str());
    }
    
    std::string potatoPath = "resources/potato.png";
    potatoTexture.loadFromFile(potatoPath.c_str());
    if (potatoTexture.id == 0) {
        potatoPath = "../resources/potato.png";
        potatoTexture.loadFromFile(potatoPath.c_str());
    }
    
    std::string pineapplePath = "resources/pineapple.png";
    pineappleTexture.loadFromFile(pineapplePath.c_str());
    if (pineappleTexture.id == 0) {
        pineapplePath = "../resources/pineapple.png";
        pineappleTexture.loadFromFile(pineapplePath.c_str());
    }
    
    // Load enemy textures
    std::string zombiePath = "resources/zombie.png";
    zombieTexture.loadFromFile(zombiePath.c_str());
    if (zombieTexture.id == 0) {
        zombiePath = "../resources/zombie.png";
        zombieTexture.loadFromFile(zombiePath.c_str());
    }
    
    std::string skeletonPath = "resources/skeleton.png";
    skeletonTexture.loadFromFile(skeletonPath.c_str());
    if (skeletonTexture.id == 0) {
        skeletonPath = "../resources/skeleton.png";
        skeletonTexture.loadFromFile(skeletonPath.c_str());
    }
    
    std::string bossPath = "resources/boss.png";
    bossTexture.loadFromFile(bossPath.c_str());
    if (bossTexture.id == 0) {
        bossPath = "../resources/boss.png";
        bossTexture.loadFromFile(bossPath.c_str());
    }
    
    std::string tankPath = "resources/tank.png";
    tankTexture.loadFromFile(tankPath.c_str());
    if (tankTexture.id == 0) {
        tankPath = "../resources/tank.png";
        tankTexture.loadFromFile(tankPath.c_str());
    }
    
    std::string ghostPath = "resources/ghost.png";
    ghostTexture.loadFromFile(ghostPath.c_str());
    if (ghostTexture.id == 0) {
        ghostPath = "../resources/ghost.png";
        ghostTexture.loadFromFile(ghostPath.c_str());
    }

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

    // Create tower selection buttons
    std::vector<TowerButton> towerButtons = {
        TowerButton(GAME_WIDTH + 20, 50, 160, 60, TowerType::APPLE),
        TowerButton(GAME_WIDTH + 20, 120, 160, 60, TowerType::CARROT),
        TowerButton(GAME_WIDTH + 20, 190, 160, 60, TowerType::POTATO),
        TowerButton(GAME_WIDTH + 20, 260, 160, 60, TowerType::PINEAPPLE),
        TowerButton(GAME_WIDTH + 20, 330, 160, 60, TowerType::BANANA_PEEL),
        TowerButton(GAME_WIDTH + 20, 400, 160, 60, TowerType::CACTUS)
    };

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

        // Handle input
        processInput(window);

        // Only update game elements if not paused and not game over
        if (!isPaused && !isGameOver) {
            // Update placement tower position
            placementTower.pos = Point((float)mouseX, (float)mouseY);
            
            // Place tower when mouse clicked (only on initial press)
            if ((mouseLeftPressed && mouseJustPressed) && !isGameOver) {
                Point clickPos((float)mouseX, (float)mouseY);
                
                // Check if clicked on tower menu buttons when menu is open
                if (towerMenu.isOpen) {
                    bool buttonClicked = false;
                    
                    if (isPointInRect((float)mouseX, (float)mouseY, towerMenu.closeButton)) {
                        towerMenu.close();
                        buttonClicked = true;
                    }
                    else if (isPointInRect((float)mouseX, (float)mouseY, towerMenu.upgradeButton1)) {
                        // Check if tower can be upgraded and player has enough beans
                        int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, towerMenu.selectedTower->damageUpgradeLevel);
                        if (upgradeCost > 0 && beanCount >= upgradeCost) {
                            beanCount -= upgradeCost;
                            towerMenu.selectedTower->upgradeDamage();
                            std::cout << "Upgraded tower! New damage level: " << towerMenu.selectedTower->damageUpgradeLevel << std::endl;
                        } else if (upgradeCost < 0) {
                            std::cout << "Tower is already at maximum level!" << std::endl;
                        } else {
                            std::cout << "Not enough beans to upgrade! Need " << upgradeCost << " beans." << std::endl;
                        }
                        buttonClicked = true;
                    }
                    else if (isPointInRect((float)mouseX, (float)mouseY, towerMenu.upgradeButton2)) {
                        // Check if tower can be upgraded and player has enough beans
                        int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, towerMenu.selectedTower->attackSpeedUpgradeLevel);
                        if (upgradeCost > 0 && beanCount >= upgradeCost) {
                            beanCount -= upgradeCost;
                            towerMenu.selectedTower->upgradeAttackSpeed();
                            std::cout << "Upgraded tower! New attack speed level: " << towerMenu.selectedTower->attackSpeedUpgradeLevel << std::endl;
                        } else if (upgradeCost < 0) {
                            std::cout << "Tower is already at maximum level!" << std::endl;
                        } else {
                            std::cout << "Not enough beans to upgrade! Need " << upgradeCost << " beans." << std::endl;
                        }
                        buttonClicked = true;
                    }
                    else if (isPointInRect((float)mouseX, (float)mouseY, towerMenu.upgradeButton3)) {
                        // Check if tower can be upgraded and player has enough beans
                        int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, towerMenu.selectedTower->rangeUpgradeLevel);
                        if (upgradeCost > 0 && beanCount >= upgradeCost) {
                            beanCount -= upgradeCost;
                            towerMenu.selectedTower->upgradeRange();
                            std::cout << "Upgraded tower! New range level: " << towerMenu.selectedTower->rangeUpgradeLevel << std::endl;
                        } else if (upgradeCost < 0) {
                            std::cout << "Tower is already at maximum level!" << std::endl;
                        } else {
                            std::cout << "Not enough beans to upgrade! Need " << upgradeCost << " beans." << std::endl;
                        }
                        buttonClicked = true;
                    }
                    else if (isPointInRect((float)mouseX, (float)mouseY, towerMenu.sellButton)) {
                        // Calculate sell value (50% of current cost including upgrades)
                        int baseCost = getTowerCost(towerMenu.selectedTower->type);
                        
                        // Add value of each upgrade (50% of upgrade costs)
                        int damageUpgradeValue = 0;
                        int attackSpeedUpgradeValue = 0;
                        int rangeUpgradeValue = 0;
                        
                        // Calculate value of damage upgrades
                        for (int i = 0; i < towerMenu.selectedTower->damageUpgradeLevel; i++) {
                            int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, i);
                            damageUpgradeValue += upgradeCost;
                        }
                        
                        // Calculate value of attack speed upgrades
                        for (int i = 0; i < towerMenu.selectedTower->attackSpeedUpgradeLevel; i++) {
                            int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, i);
                            attackSpeedUpgradeValue += upgradeCost;
                        }
                        
                        // Calculate value of range upgrades
                        for (int i = 0; i < towerMenu.selectedTower->rangeUpgradeLevel; i++) {
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
                            [selectedTower = towerMenu.selectedTower](const Tower& tower) {
                                return &tower == selectedTower;
                            }), towers.end());
                            
                        // Decrement tower count
                        switch(towerMenu.selectedTower->type) {
                            case TowerType::APPLE: appleTowerCount--; break;
                            case TowerType::CARROT: carrotTowerCount--; break;
                            case TowerType::POTATO: potatoTowerCount--; break;
                            case TowerType::PINEAPPLE: pineappleTowerCount--; break;
                            case TowerType::BANANA_PEEL: bananaPeelTowerCount--; break;
                            case TowerType::CACTUS: cactusTowerCount--; break;
                            default: break;
                        }
                        
                        std::cout << "Sold tower for " << sellValue << " beans. Total beans: " << beanCount << std::endl;
                        towerMenu.close();
                        buttonClicked = true;
                    }
                    
                    // Only check for tower selection if no button was clicked
                    if (!buttonClicked && mouseX < GAME_WIDTH) {
                        for (auto& tower : towers) {
                            if (distance(Point(mouseX, mouseY), tower.pos) < TOWER_SIZE/2) {
                                towerMenu.open(&tower);
                                break;
                            }
                        }
                    }
                }
                // Check if clicked on an existing tower
                else if (mouseX < GAME_WIDTH) {  // Only check in game area
                    bool towerClicked = false;
                    for (auto& tower : towers) {
                        if (distance(Point(mouseX, mouseY), tower.pos) < TOWER_SIZE/2) {
                            towerMenu.open(&tower);
                            towerClicked = true;
                            break;
                        }
                    }
                    
                    // Only try to place if no tower was clicked and a tower type is selected
                    if (!towerClicked && placementTower.type != TowerType::NONE) {
                        // Check placement validity
                        bool canPlace = true;
                        
                        // Don't allow placing overlapping with path or water
                        if (!canPlaceTower(clickPos, TOWER_SIZE, placementTower.type)) {
                            canPlace = false;
                            if (placementTower.type == TowerType::BANANA_PEEL) {
                                std::cout << "Banana Peel must be placed directly on the path!" << std::endl;
                            } else {
                                std::cout << "Cannot place tower on water or path!" << std::endl;
                            }
                        }
                        
                        // Don't allow placing towers on top of each other
                        for (const auto& tower : towers) {
                            if (distance(tower.pos, placementTower.pos) < TOWER_SIZE) {
                                canPlace = false;
                                std::cout << "Cannot place tower on another tower!" << std::endl;
                                break;
                            }
                        }
                        
                        // Check if player has enough beans to buy the tower
                        int towerCost = getTowerCost(placementTower.type);
                        if (beanCount < towerCost) {
                            canPlace = false;
                            std::cout << "Not enough beans to buy this tower! Need " << towerCost << " beans." << std::endl;
                        }
                        
                        if (canPlace) {
                            Tower newTower((float)mouseX, (float)mouseY);
                            newTower.setType(placementTower.type);  // Copy the selected tower type
                            towers.push_back(newTower);
                            
                            // Deduct tower cost from bean count
                            beanCount -= towerCost;
                            
                            // Increment tower count for that type
                            switch(newTower.type) {
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
            else if (!mouseLeftPressed) {
                // Reset flag when mouse is released
                mouseJustPressed = false;
            }

            // Update all active enemies
            for (auto& enemy : enemies) {
                updateEnemy(enemy, deltaTime, towers, beanCount);
            }

            // Handle round system
            if (!isGameOver) {
                if (!isRoundActive) {
                    // Countdown to next round
                    roundStartTimer -= deltaTime;
                    if (roundStartTimer <= 0) {
                        startNewRound(enemies, currentRound);
                        // Reset timer for next round - always 5 seconds
                        roundStartTimer = 5.0f;  // Standard 5 second break between all rounds
                    }
                } else {
                    // Handle enemy spawning during active round
                    if (enemiesLeftInRound > 0) {
                        enemySpawnTimer += deltaTime;
                        float spawnRate = 1.0f;  // Fixed spawn rate of 1 enemy per second
                        
                        if (enemySpawnTimer >= spawnRate) {
                            enemySpawnTimer = 0.0f;
                            
                            // Find inactive enemy to spawn
                            for (auto& enemy : enemies) {
                                if (!enemy.isActive) {
                                    enemy.isActive = true;
                                    enemy.currentWaypoint = 0;
                                    enemy.pos = waypoints[0];
                                    
                                    // Spawn boss as the last enemy of a boss round
                                    if (isBossRound(currentRound) && enemiesLeftInRound == 1) {
                                        enemy.type = EnemyType::BOSS;
                                        std::cout << "A BOSS has appeared!" << std::endl;
                                    } else {
                                        // Determine regular enemy type based on round
                                        float skeletonChance = getSkeletonPercentage(currentRound);
                                        float tankChance = getTankPercentage(currentRound);
                                        float ghostChance = getGhostPercentage(currentRound);
                                        float random = static_cast<float>(rand()) / RAND_MAX;
                                        
                                        if (random < skeletonChance) {
                                            enemy.type = EnemyType::SKELETON;
                                        } else if (random < skeletonChance + tankChance) {
                                            enemy.type = EnemyType::TANK;
                                        } else if (random < skeletonChance + tankChance + ghostChance) {
                                            enemy.type = EnemyType::GHOST;
                                        } else {
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
                    if (isRoundComplete(enemies)) {
                        isRoundActive = false;
                        // Give bonus beans for completing a round
                        int roundBonus = currentRound * 5;
                        beanCount += roundBonus;
                        
                        // Regular round completion message
                        std::cout << "Round " << currentRound << " complete! Bonus beans: " << beanCount << std::endl;
                    }
                }
            } else {
                // Game is over - check for mouse click to restart
                if (mouseLeftPressed) {
                    resetGame(enemies, towers, projectiles, beanCount);
                    mouseLeftPressed = false;
                }
            }

            // Update towers and handle shooting
            for (auto& tower : towers) {
                // Only update towers if game is not over
                if (!isGameOver) {
                    tower.shootTimer += deltaTime;
                    
                    // Only shooting towers need to find targets and shoot
                    if (tower.type != TowerType::BANANA_PEEL && tower.type != TowerType::CACTUS) {
                        // Check if tower can shoot
                        if (tower.shootTimer >= 1.0f / tower.attackSpeed) {
                            if (tower.type == TowerType::PINEAPPLE) {
                                // For Pineapple tower, check if any enemy is in range
                                bool enemyInRange = false;
                                for (const auto& enemy : enemies) {
                                    if (enemy.isActive && distance(tower.pos, enemy.pos) <= tower.range) {
                                        enemyInRange = true;
                                        break;
                                    }
                                }
                                
                                if (enemyInRange) {
                                    // Shoot in all 8 directions
                                    spawnProjectilesInAllDirections(projectiles, tower.pos, tower);
                                    tower.shootTimer = 0; // Reset shoot timer
                                }
                            } else {
                                // For other towers, find closest enemy in range
                                Enemy* target = findClosestEnemy(tower.pos, tower.range, enemies);
                                
                                if (target) {
                                    // Spawn projectile aimed at the target
                                    spawnProjectile(projectiles, tower.pos, target->pos, tower);
                                    tower.shootTimer = 0; // Reset shoot timer
                                }
                            }
                        }
                    }
                }
            }
            
            // Update and check projectile collisions
            for (auto& proj : projectiles) {
                if (!proj.active) continue;
                
                proj.update(deltaTime);
                
                // Check for collisions with enemies
                for (auto& enemy : enemies) {
                    if (!enemy.isActive) continue;
                    
                    float enemySize = (enemy.type == EnemyType::BOSS) ? BOSS_SIZE : ENEMY_SIZE;
                    
                    if (distance(proj.pos, enemy.pos) < enemySize / 2) {
                        // Hit! Deactivate projectile
                        proj.active = false;
                        
                        // Apply damage to enemy
                        enemy.health -= proj.damage;
                        
                        // Check if enemy is defeated
                        if (enemy.health <= 0) {
                            // Add beans based on enemy type
                            if (enemy.type == EnemyType::BOSS) {
                                beanCount += BOSS_BEANS;
                                std::cout << "BOSS destroyed! +" << BOSS_BEANS << " beans!" << std::endl;
                            } else {
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
                    proj.pos.y < -50 || proj.pos.y > HEIGHT + 50) {
                    proj.active = false;
                }
            }
            
            // Remove used banana peels and empty cacti
            towers.erase(std::remove_if(towers.begin(), towers.end(),
                [](const Tower& tower) {
                    return (tower.type == TowerType::BANANA_PEEL && tower.isUsed);
                    // No longer remove cacti since they last forever
                }), towers.end());
        }

        // Update tower button hover states
        for (auto& button : towerButtons) {
            button.isHovered = isPointInRect((float)mouseX, (float)mouseY, button.rect);
            if (button.isHovered && mouseLeftPressed && !towerMenu.isOpen) {  // Only allow tower selection if menu is not open
                // If clicking the same tower type that's already selected, deselect it
                if (placementTower.type == button.type) {
                    placementTower.setType(TowerType::NONE);
                } else {
                    placementTower.setType(button.type);
                }
                mouseLeftPressed = false;  // Consume the click
            }
        }

        // Clear screen
        renderer.clearScreen({0.1, 0.2, 0.6, 1});

        // Draw background
        renderer.renderRectangle({0 * scaleX, 0 * scaleY, GAME_WIDTH * scaleX, HEIGHT * scaleY}, backgroundTexture, {1, 1, 1, 1});
        
        // Draw UI panel background
        renderer.renderRectangle(
            {(float)GAME_WIDTH * scaleX, 0 * scaleY, (float)PANEL_WIDTH * scaleX, (float)HEIGHT * scaleY},
            {UI_BACKGROUND.r, UI_BACKGROUND.g, UI_BACKGROUND.b, UI_BACKGROUND.a}
        );
        
        // Draw bean count in top-right corner with larger size and better position
        drawNumber(renderer, beanCount, (GAME_WIDTH - 180) * scaleX, 15 * scaleY, 32.0f * scaleY);  // Moved from -120 to -180 to accommodate larger numbers
        
        // Draw round information in top-left corner
        float roundTextX = 70 * scaleX;
        float roundTextY = 15 * scaleY;
        float roundDigitSize = 24.0f * scaleY;
        
        // Draw improved "R" symbol that's clearly not a P
        float rSize = 30.0f * scaleY;
        float rX = roundTextX - 50 * scaleX;
        float rY = roundTextY;
        float thickness = 5.0f * scaleY;
        
        // Vertical line of R
        renderer.renderRectangle({rX, rY, thickness, rSize}, {1.0f, 1.0f, 1.0f, 1.0f});
        
        // Top horizontal line of R
        renderer.renderRectangle({rX, rY, rSize * 0.7f, thickness}, {1.0f, 1.0f, 1.0f, 1.0f});
        
        // Middle horizontal line of R
        renderer.renderRectangle({rX, rY + rSize/2 - thickness/2, rSize * 0.7f, thickness}, {1.0f, 1.0f, 1.0f, 1.0f});
        
        // Top curve of R
        renderer.renderRectangle({rX + rSize * 0.7f - thickness, rY, thickness, rSize/2}, {1.0f, 1.0f, 1.0f, 1.0f});
        
        // Diagonal leg of R - made more prominent and extended
        for (int i = 0; i < rSize/2; i++) {
            // Create a clear diagonal line that extends further right
            float xPos = rX + rSize * 0.4f + i * 0.5f * scaleX;
            float yPos = rY + rSize/2 + i * scaleY;
            renderer.renderRectangle({xPos, yPos, thickness, thickness/2}, {1.0f, 1.0f, 1.0f, 1.0f});
        }
        
        // Draw round number with background
        std::string roundStr = std::to_string(currentRound);
        float bgWidth = roundDigitSize * 1.2f * roundStr.length() + 20 * scaleX;
        
        renderer.renderRectangle(
            {roundTextX - 5 * scaleX, roundTextY - 5 * scaleY, bgWidth, roundDigitSize + 10 * scaleY},
            {0.0f, 0.0f, 0.0f, 0.5f}
        );
        
        // Draw each digit of the round number
        for (size_t i = 0; i < roundStr.length(); i++) {
            drawDigit(renderer, roundStr[i] - '0', roundTextX + i * (roundDigitSize * 1.2f), roundTextY, roundDigitSize);
        }
        
        // Draw current lives
        float livesX = 200 * scaleX;
        float livesY = 15 * scaleY;
        
        // Draw heart icon for lives
        for (int i = 0; i < lives; i++) {
            renderer.renderRectangle(
                {livesX + i * 30 * scaleX, livesY, 20 * scaleX, 20 * scaleY},
                {1.0f, 0.0f, 0.0f, 1.0f}  // Red heart
            );
        }
        
        // If game is over, show game over screen
        if (isGameOver) {
            // Overlay with semi-transparent black
            renderer.renderRectangle(
                {0, 0, (float)w, (float)h},
                {0.0f, 0.0f, 0.0f, 0.7f}
            );
            
            // Game Over text
            float gameOverX = w / 2 - 150 * scaleX;
            float gameOverY = h / 2 - 100 * scaleY;
            float textHeight = 40.0f * scaleY;
            
            // Draw "GAME OVER" text
            std::string gameOverText = "GAME OVER";
            for (int i = 0; i < 8; i++) {
                float letterX = gameOverX + i * 35 * scaleX;
                float letterWidth = 30.0f * scaleX;
                
                // Draw letter background
                renderer.renderRectangle(
                    {letterX, gameOverY, letterWidth, textHeight},
                    {0.8f, 0.0f, 0.0f, 1.0f}  // Red background
                );
            }
            
            // Draw "Click to restart" message
            float restartY = gameOverY + 80 * scaleY;
            renderer.renderRectangle(
                {w / 2 - 120 * scaleX, restartY, 240 * scaleX, 5 * scaleY},
                {1.0f, 1.0f, 1.0f, 1.0f}
            );
            
            // Show final score (rounds completed)
            std::string finalScoreStr = "ROUNDS: " + std::to_string(currentRound);
            float scoreY = restartY + 40 * scaleY;
            renderer.renderRectangle(
                {w / 2 - 100 * scaleX, scoreY, 200 * scaleX, 30 * scaleY},
                {0.0f, 0.0f, 0.3f, 0.8f}  // Blue score box
            );
            
            // Check for click to restart - use mouseJustPressed instead of mouseLeftPressed
            if (mouseJustPressed) {
                resetGame(enemies, towers, projectiles, beanCount);
                mouseJustPressed = false;
            }
        }
        
        // If game is paused, show pause screen
        if (isPaused) {
            // Overlay with semi-transparent black
            renderer.renderRectangle(
                {0, 0, (float)w, (float)h},
                {0.0f, 0.0f, 0.0f, 0.5f}  // Less dark than game over screen
            );
            
            // Pause text
            float pauseX = w / 2 - 120 * scaleX;
            float pauseY = h / 2 - 50 * scaleY;
            float textHeight = 40.0f * scaleY;
            
            // Draw "PAUSED" text
            std::string pauseText = "PAUSED";
            for (int i = 0; i < 6; i++) {
                float letterX = pauseX + i * 35 * scaleX;
                float letterWidth = 30.0f * scaleX;
                
                // Draw letter background
                renderer.renderRectangle(
                    {letterX, pauseY, letterWidth, textHeight},
                    {0.0f, 0.5f, 0.8f, 1.0f}  // Blue background
                );
            }
            
            // Draw "Press any key to continue" message
            float continueY = pauseY + 80 * scaleY;
            renderer.renderRectangle(
                {w / 2 - 150 * scaleX, continueY, 300 * scaleX, 5 * scaleY},
                {1.0f, 1.0f, 1.0f, 1.0f}
            );
        }
        
        // If we're between rounds, show countdown
        if (!isRoundActive && !isGameOver) {
            int countdown = (int)roundStartTimer + 1;
            std::string countdownStr = std::to_string(countdown);
            float countdownX = w / 2 - 30 * scaleX;
            float countdownY = 100 * scaleY;
            float countdownSize = 60.0f * scaleY;
            
            // Draw large countdown number
            renderer.renderRectangle(
                {countdownX - 10 * scaleX, countdownY - 10 * scaleY, countdownSize + 20 * scaleX, countdownSize + 20 * scaleY},
                {0.0f, 0.0f, 0.0f, 0.7f}
            );
            
            for (size_t i = 0; i < countdownStr.length(); i++) {
                drawDigit(renderer, countdownStr[i] - '0', countdownX + i * (countdownSize * 0.8f), countdownY, countdownSize);
            }
            
            // Draw "NEXT ROUND" text using rectangles
            float textY = countdownY + countdownSize + 20 * scaleY;
            renderer.renderRectangle({w / 2 - 100 * scaleX, textY, 200 * scaleX, 5 * scaleY}, {1.0f, 1.0f, 1.0f, 1.0f});
        }

        // Draw tower selection buttons
        if (!towerMenu.isOpen) {  // Only draw tower selection buttons if tower menu is not open
            for (const auto& button : towerButtons) {
                // Draw button background
                Color buttonColor = button.isHovered ? UI_SELECTED : UI_BACKGROUND;
                if (placementTower.type == button.type) {
                    buttonColor = {0.3f, 0.5f, 0.3f, 1.0f};  // Green tint for selected tower
                }
                
                // Check if player can afford this tower
                bool canAfford = beanCount >= getTowerCost(button.type);
                
                // If can't afford, make button appear darker
                if (!canAfford) {
                    buttonColor.r *= 0.7f;
                    buttonColor.g *= 0.7f;
                    buttonColor.b *= 0.7f;
                }
                // Scale the button position and size
                float bx = button.rect.x * scaleX;
                float by = button.rect.y * scaleY;
                float bw = button.rect.w * scaleX;
                float bh = button.rect.h * scaleY;
                renderer.renderRectangle(
                    {bx, by, bw, bh},
                    {buttonColor.r, buttonColor.g, buttonColor.b, buttonColor.a}
                );
                
                // Draw tower preview with its specific color or texture
                float towerSize = 32.0f * scaleY;
                float towerX = bx + bw/2 - towerSize/2;
                float towerY = by + 10 * scaleY;
                Color previewColor = getTowerColor(button.type);
                
                // Make the tower preview darker if can't afford
                if (!canAfford) {
                    previewColor.r *= 0.7f;
                    previewColor.g *= 0.7f;
                    previewColor.b *= 0.7f;
                }
                
                if (button.type == TowerType::CARROT && carrotTowerTexture.id != 0) {
                    renderer.renderRectangle(
                        {towerX, towerY, towerSize, towerSize},
                        carrotTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f}
                    );
                } else {
                    renderer.renderRectangle(
                        {towerX, towerY, towerSize, towerSize},
                        {previewColor.r, previewColor.g, previewColor.b, previewColor.a}
                    );
                }
                
                // Draw cost indicator
                int cost = getTowerCost(button.type);
                std::string costStr = std::to_string(cost);
                float digitSize = 16.0f * scaleY;
                float costX = bx + 10 * scaleX;
                float costY = by + bh - 20 * scaleY;
                
                // Draw bean icon
                renderer.renderRectangle(
                    {costX, costY, 10.0f * scaleY, 10.0f * scaleY},
                    {0.6f, 0.4f, 0.2f, 1.0f}
                );
                
                // Draw digits for cost with increased spacing
                for (size_t i = 0; i < costStr.length(); i++) {
                    drawDigit(renderer, costStr[i] - '0', costX + 15 * scaleX + i * (digitSize * 1.2f), costY - 5 * scaleY, digitSize);
                }
                
                // Draw tower count indicator
                int towerCount = 0;
                switch(button.type) {
                    case TowerType::APPLE: towerCount = appleTowerCount; break;
                    case TowerType::CARROT: towerCount = carrotTowerCount; break;
                    case TowerType::POTATO: towerCount = potatoTowerCount; break;
                    case TowerType::PINEAPPLE: towerCount = pineappleTowerCount; break;
                    case TowerType::BANANA_PEEL: towerCount = bananaPeelTowerCount; break;
                    case TowerType::CACTUS: towerCount = cactusTowerCount; break;
                }
                
                if (towerCount > 0) {
                    std::string countStr = "x" + std::to_string(towerCount);
                    float countX = bx + bw - 30 * scaleX;
                    float countY = by + 15 * scaleY;
                    
                    renderer.renderRectangle(
                        {countX - 5 * scaleX, countY - 5 * scaleY, 30 * scaleX, 20 * scaleY},
                        {0.0f, 0.0f, 0.0f, 0.5f}
                    );
                    
                    float smallDigitSize = 14.0f * scaleY;
                    for (size_t i = 0; i < countStr.length(); i++) {
                        if (countStr[i] == 'x') continue;
                        drawDigit(renderer, countStr[i] - '0', countX + 10 * scaleX + (i-1) * (smallDigitSize * 0.8f), countY, smallDigitSize);
                    }
                }
            }
        }
        
        // Draw tower menu if open
        if (towerMenu.isOpen) {
            // Draw menu background
            renderer.renderRectangle(
                {(float)GAME_WIDTH * scaleX, 0 * scaleY, (float)PANEL_WIDTH * scaleX, (float)HEIGHT * scaleY},
                {UI_BACKGROUND.r, UI_BACKGROUND.g, UI_BACKGROUND.b, 1.0f}
            );
            
            // Draw selected tower info
            if (towerMenu.selectedTower) {
                // Draw tower name at the top
                float nameY = 50 * scaleY;
                std::string towerName = getTowerTypeName(towerMenu.selectedTower->type);
                
                // Draw tower preview (larger)
                float previewSize = TOWER_SIZE * 1.5f * scaleY;
                float previewY = 100 * scaleY;
                Color towerColor = getTowerColor(towerMenu.selectedTower->type);
                renderer.renderRectangle(
                    {GAME_WIDTH * scaleX + PANEL_WIDTH * scaleX/2 - previewSize/2, previewY, previewSize, previewSize},
                    {towerColor.r, towerColor.g, towerColor.b, 1.0f}
                );

                // Draw upgrade buttons
                std::vector<std::pair<std::string, int*>> upgrades = {
                    {"damage", &towerMenu.selectedTower->damageUpgradeLevel},
                    {"attackSpeed", &towerMenu.selectedTower->attackSpeedUpgradeLevel},
                    {"range", &towerMenu.selectedTower->rangeUpgradeLevel}
                };
                
                for (int i = 0; i < 3; i++) {
                    Rectangle& upgradeButton = (i == 0) ? towerMenu.upgradeButton1 : 
                                             (i == 1) ? towerMenu.upgradeButton2 : 
                                             towerMenu.upgradeButton3;
                    
                    Color upgradeColor(0.4f, 0.1f, 0.1f, 1.0f);  // Default to dark red
                    int upgradeCost = towerMenu.selectedTower->getUpgradeCost(towerMenu.selectedTower->type, *upgrades[i].second);
                    bool canUpgrade = upgradeCost > 0 && beanCount >= upgradeCost;
                    
                    if (*upgrades[i].second >= 3) {
                        upgradeColor = Color(0.0f, 0.8f, 0.0f, 1.0f);  // Green for max level
                    } else if (canUpgrade) {
                        upgradeColor = isPointInRect((float)mouseX, (float)mouseY, upgradeButton)
                            ? Color(0.0f, 0.8f, 0.0f, 1.0f)  // Bright green when hovered
                            : Color(0.0f, 0.6f, 0.0f, 1.0f); // Dark green normally
                    }
                    
                    // Draw button background
                    renderer.renderRectangle(
                        {upgradeButton.x, upgradeButton.y, upgradeButton.w, upgradeButton.h},
                        {upgradeColor.r, upgradeColor.g, upgradeColor.b, upgradeColor.a}
                    );
                    
                    // Draw button label background
                    renderer.renderRectangle(
                        {upgradeButton.x + 5, upgradeButton.y + 5, upgradeButton.w - 10, 20},
                        {0.0f, 0.0f, 0.0f, 0.5f}
                    );
                    
                    // Instead of trying to render text, use color-coded symbols
                    if (i == 0) {  // DAMAGE - red symbol
                        // Draw damage symbol (sword/arrow)
                        renderer.renderRectangle(
                            {upgradeButton.x + 10, upgradeButton.y + 10, 30, 10},
                            {1.0f, 0.3f, 0.3f, 1.0f}  // Red
                        );
                        
                        // Draw cost text only if not maxed out
                        if (*upgrades[i].second < 3) {
                            drawNumber(renderer, upgradeCost, upgradeButton.x + upgradeButton.w - 80, upgradeButton.y + 10, 20.0f);
                        }
                    }
                    else if (i == 1) {  // ATTACK SPEED - blue symbol
                        // Draw speed symbol (lightning)
                        renderer.renderRectangle(
                            {upgradeButton.x + 10, upgradeButton.y + 10, 10, 20},
                            {0.3f, 0.3f, 1.0f, 1.0f}  // Blue
                        );
                        renderer.renderRectangle(
                            {upgradeButton.x + 30, upgradeButton.y + 10, 10, 20},
                            {0.3f, 0.3f, 1.0f, 1.0f}  // Blue
                        );
                        
                        // Draw cost text only if not maxed out
                        if (*upgrades[i].second < 3) {
                            drawNumber(renderer, upgradeCost, upgradeButton.x + upgradeButton.w - 80, upgradeButton.y + 10, 20.0f);
                        }
                    }
                    else {  // RANGE - green symbol
                        // Draw range symbol (circle)
                        renderer.renderRectangle(
                            {upgradeButton.x + 20, upgradeButton.y + 10, 20, 20},
                            {0.3f, 0.8f, 0.3f, 1.0f}  // Green
                        );
                        
                        // Draw cost text only if not maxed out
                        if (*upgrades[i].second < 3) {
                            drawNumber(renderer, upgradeCost, upgradeButton.x + upgradeButton.w - 80, upgradeButton.y + 10, 20.0f);
                        }
                    }
                    
                    // Draw upgrade level indicator at the bottom of button
                    int level = *upgrades[i].second;
                    float indicatorWidth = upgradeButton.w / 3;
                    float indicatorHeight = 5.0f;
                    float indicatorY = upgradeButton.y + upgradeButton.h - 10.0f;
                    
                    for (int j = 0; j < 3; j++) {
                        Color levelColor = (j < level) ? Color(1.0f, 1.0f, 0.0f, 1.0f) : Color(0.5f, 0.5f, 0.5f, 0.5f);
                        renderer.renderRectangle(
                            {upgradeButton.x + 10.0f + (j * indicatorWidth), indicatorY, indicatorWidth - 5.0f, indicatorHeight},
                            {levelColor.r, levelColor.g, levelColor.b, levelColor.a}
                        );
                    }
                }
                
                // Draw sell button
                Color sellColor = isPointInRect((float)mouseX, (float)mouseY, towerMenu.sellButton) 
                    ? Color(0.8f, 0.2f, 0.2f, 1.0f) : Color(0.6f, 0.1f, 0.1f, 1.0f);
                
                renderer.renderRectangle(
                    {towerMenu.sellButton.x, towerMenu.sellButton.y, 
                     towerMenu.sellButton.w, towerMenu.sellButton.h},
                    {sellColor.r, sellColor.g, sellColor.b, sellColor.a}
                );
                
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
                    {0.6f, 0.4f, 0.2f, 1.0f}
                );
                
                // Draw sell value
                for (size_t i = 0; i < sellText.length(); i++) {
                    drawDigit(renderer, sellText[i] - '0', textX + i * (digitSize * 1.2f), textY, digitSize);
                }
                
                // Draw close button (X)
                Color closeColor = isPointInRect((float)mouseX, (float)mouseY, towerMenu.closeButton) 
                    ? Color(1.0f, 0.2f, 0.2f, 1.0f) : Color(0.8f, 0.0f, 0.0f, 1.0f);  // Brighter red on hover
                
                // Draw X shape with thicker lines and better positioning
                float xSize = 16.0f;  // Reduced size from 24.0f to 16.0f
                float xX = GAME_WIDTH * scaleX + PANEL_WIDTH * scaleX - xSize - 10;  // Position in top right
                float xY = 10;  // Small margin from top
                float thickness = 3.0f;  // Reduced thickness from 4.0f to 3.0f
                
                // Background circle for the X
                renderer.renderRectangle(
                    {xX - 3, xY - 3, xSize + 6, xSize + 6},  // Adjusted padding for smaller size
                    {0.2f, 0.0f, 0.0f, 1.0f}  // Dark red background
                );
                
                // Draw diagonal lines for X
                float diagonal = sqrt(2.0f) * xSize;  // Length of diagonal
                
                // First diagonal (top-left to bottom-right)
                for(int i = 0; i < diagonal; i++) {
                    renderer.renderRectangle(
                        {xX + i, xY + i, thickness, thickness},
                        {closeColor.r, closeColor.g, closeColor.b, closeColor.a}
                    );
                }
                
                // Second diagonal (top-right to bottom-left)
                for(int i = 0; i < diagonal; i++) {
                    renderer.renderRectangle(
                        {xX + xSize - i, xY + i, thickness, thickness},
                        {closeColor.r, closeColor.g, closeColor.b, closeColor.a}
                    );
                }
                
                // Update close button hitbox
                towerMenu.closeButton = Rectangle(xX - 3, xY - 3, xSize + 6, xSize + 6);
            }
        }

        // Draw towers
for (const auto& tower : towers) {
    Color towerColor = getTowerColor(tower.type);

    if (tower.type == TowerType::BANANA_PEEL) {
        // ... banana peel drawing code ...
    } else if (tower.type == TowerType::CACTUS) {
        renderCactus(renderer, tower);
    } else if (tower.type == TowerType::CARROT) {
        if (carrotTowerTexture.id != 0) {
            renderer.renderRectangle(
                {tower.pos.x - TOWER_SIZE/2, tower.pos.y - TOWER_SIZE/2, TOWER_SIZE, TOWER_SIZE},
                carrotTowerTexture, {1.0f, 1.0f, 1.0f, 1.0f}
            );
        } else {
            renderer.renderRectangle(
                {tower.pos.x - TOWER_SIZE/2, tower.pos.y - TOWER_SIZE/2, TOWER_SIZE, TOWER_SIZE},
                {towerColor.r, towerColor.g, towerColor.b, towerColor.a}
            );
        }
    } else {
        // Other towers (apple, potato, pineapple, etc.) fallback to color for now
        renderer.renderRectangle(
            {tower.pos.x - TOWER_SIZE/2, tower.pos.y - TOWER_SIZE/2, TOWER_SIZE, TOWER_SIZE},
            {towerColor.r, towerColor.g, towerColor.b, towerColor.a}
        );
    }
}
        
        // Draw placement tower (semi-transparent)
        if (placementTower.type != TowerType::NONE) {
            Color placementColor = getTowerColor(placementTower.type);
            renderer.renderRectangle(
                {placementTower.pos.x - TOWER_SIZE/2, placementTower.pos.y - TOWER_SIZE/2, TOWER_SIZE, TOWER_SIZE},
                {placementColor.r, placementColor.g, placementColor.b, 0.5f}
            );
        }

        // Draw all active enemies
        for (const auto& enemy : enemies) {
            if (enemy.isActive) {
                Color enemyColor = getEnemyColor(enemy.type);
                float enemySize = 0.0f;
                
                switch (enemy.type) {
                    case EnemyType::BOSS:
                        enemySize = BOSS_SIZE;
                        // Use boss texture if available, fall back to colored rectangle if not
                        if (bossTexture.id != 0) {
                            renderer.renderRectangle(
                                {enemy.pos.x - enemySize/2, enemy.pos.y - enemySize/2, enemySize, enemySize},
                                bossTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1}
                            );
                        } else {
                            renderer.renderRectangle(
                                {enemy.pos.x - enemySize/2, enemy.pos.y - enemySize/2, enemySize, enemySize},
                                {enemyColor.r, enemyColor.g, enemyColor.b, enemyColor.a}
                            );
                        }
                        break;
                        
                    case EnemyType::TANK:
                        enemySize = TANK_SIZE;
                        // Use tank texture if available, fall back to colored rectangle if not
                        if (tankTexture.id != 0) {
                            renderer.renderRectangle(
                                {enemy.pos.x - enemySize/2, enemy.pos.y - enemySize/2, enemySize, enemySize},
                                tankTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1}
                            );
                        } else {
                            renderer.renderRectangle(
                                {enemy.pos.x - enemySize/2, enemy.pos.y - enemySize/2, enemySize, enemySize},
                                {enemyColor.r, enemyColor.g, enemyColor.b, enemyColor.a}
                            );
                        }
                        break;
                        
                    case EnemyType::GHOST:
                        enemySize = GHOST_SIZE;
                        // Use ghost texture if available, fall back to colored rectangle if not
                        if (ghostTexture.id != 0) {
                            renderer.renderRectangle(
                                {enemy.pos.x - enemySize/2, enemy.pos.y - enemySize/2, enemySize, enemySize},
                                ghostTexture, {1.0f, 1.0f, 1.0f, enemyColor.a}, {0, 0}, 0, {0, 0, 1, 1}
                            );
                        } else {
                            renderer.renderRectangle(
                                {enemy.pos.x - enemySize/2, enemy.pos.y - enemySize/2, enemySize, enemySize},
                                {enemyColor.r, enemyColor.g, enemyColor.b, enemyColor.a}
                            );
                        }
                        break;
                        
                    case EnemyType::SKELETON:
                        enemySize = ENEMY_SIZE;
                        // Use skeleton texture if available, fall back to colored rectangle if not
                        if (skeletonTexture.id != 0) {
                            renderer.renderRectangle(
                                {enemy.pos.x - enemySize/2, enemy.pos.y - enemySize/2, enemySize, enemySize},
                                skeletonTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1}
                            );
                        } else {
                            renderer.renderRectangle(
                                {enemy.pos.x - enemySize/2, enemy.pos.y - enemySize/2, enemySize, enemySize},
                                {enemyColor.r, enemyColor.g, enemyColor.b, enemyColor.a}
                            );
                        }
                        break;
                        
                    case EnemyType::ZOMBIE:
                    default:
                        enemySize = ENEMY_SIZE;
                        // Use zombie texture if available, fall back to colored rectangle if not
                        if (zombieTexture.id != 0) {
                            renderer.renderRectangle(
                                {enemy.pos.x - enemySize/2, enemy.pos.y - enemySize/2, enemySize, enemySize},
                                zombieTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1}
                            );
                        } else {
                            renderer.renderRectangle(
                                {enemy.pos.x - enemySize/2, enemy.pos.y - enemySize/2, enemySize, enemySize},
                                {enemyColor.r, enemyColor.g, enemyColor.b, enemyColor.a}
                            );
                        }
                        break;
                }
                
                // Draw health bar background
                float healthBarWidth = enemySize;
                float healthBarHeight = 6.0f;
                renderer.renderRectangle(
                    {enemy.pos.x - healthBarWidth/2, enemy.pos.y - enemySize/2 - 10.0f, 
                     healthBarWidth, healthBarHeight},
                    {0.2f, 0.2f, 0.2f, 0.8f}
                );
                
                // Draw health bar fill
                float healthPercent = enemy.health / enemy.maxHealth;
                float healthBarFillWidth = healthBarWidth * healthPercent;
                
                // Color the health bar from red (low health) to green (full health)
                Color healthColor = {1.0f - healthPercent, healthPercent, 0.0f, 1.0f};
                
                renderer.renderRectangle(
                    {enemy.pos.x - healthBarWidth/2, enemy.pos.y - enemySize/2 - 10.0f, 
                     healthBarFillWidth, healthBarHeight},
                    {healthColor.r, healthColor.g, healthColor.b, healthColor.a}
                );
            }
        }

        // Draw projectiles (add before renderer.flush())
        for (const auto& proj : projectiles) {
            if (proj.active) {
                // Set position and size
                float halfSize = PROJECTILE_SIZE / 2.0f;
                
                // Render different projectile based on tower type
                switch (proj.sourceType) {
                    case TowerType::APPLE:
                        // Use apple texture for projectiles if valid, otherwise fallback to red color
                        if (appleTexture.id != 0) {
                            renderer.renderRectangle(
                                {proj.pos.x - halfSize, proj.pos.y - halfSize, PROJECTILE_SIZE, PROJECTILE_SIZE},
                                appleTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 3.14159f, {0, 0, 1, 1}
                            );
                        } else {
                            // Fallback to red color if texture isn't loaded
                            renderer.renderRectangle(
                                {proj.pos.x - halfSize, proj.pos.y - halfSize, PROJECTILE_SIZE, PROJECTILE_SIZE},
                                {1.0f, 0.2f, 0.2f, 1.0f}
                            );
                        }
                        break;
                        
                    case TowerType::CARROT:
                        // Use carrot texture for projectiles if valid, otherwise fallback to orange color
                        if (carrotTexture.id != 0) {
                            renderer.renderRectangle(
                                {proj.pos.x - halfSize, proj.pos.y - halfSize, PROJECTILE_SIZE, PROJECTILE_SIZE},
                                carrotTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1}
                            );
                        } else {
                            // Fallback to orange color if texture isn't loaded
                            renderer.renderRectangle(
                                {proj.pos.x - halfSize, proj.pos.y - halfSize, PROJECTILE_SIZE, PROJECTILE_SIZE},
                                {1.0f, 0.5f, 0.0f, 1.0f}
                            );
                        }
                        break;
                        
                    case TowerType::POTATO:
                        // Use potato texture for projectiles if valid, otherwise fallback to brown color
                        if (potatoTexture.id != 0) {
                            renderer.renderRectangle(
                                {proj.pos.x - halfSize, proj.pos.y - halfSize, PROJECTILE_SIZE, PROJECTILE_SIZE},
                                potatoTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1}
                            );
                        } else {
                            // Fallback to brown color if texture isn't loaded
                            renderer.renderRectangle(
                                {proj.pos.x - halfSize, proj.pos.y - halfSize, PROJECTILE_SIZE, PROJECTILE_SIZE},
                                {0.6f, 0.4f, 0.2f, 1.0f}
                            );
                        }
                        break;
                        
                    case TowerType::PINEAPPLE:
                        // Use pineapple texture for projectiles if valid, otherwise fallback to yellow color
                        if (pineappleTexture.id != 0) {
                            renderer.renderRectangle(
                                {proj.pos.x - halfSize, proj.pos.y - halfSize, PROJECTILE_SIZE, PROJECTILE_SIZE},
                                pineappleTexture, {1.0f, 1.0f, 1.0f, 1.0f}, {0, 0}, 0, {0, 0, 1, 1}
                            );
                        } else {
                            // Fallback to yellow color if texture isn't loaded
                            renderer.renderRectangle(
                                {proj.pos.x - halfSize, proj.pos.y - halfSize, PROJECTILE_SIZE, PROJECTILE_SIZE},
                                {0.8f, 0.8f, 0.0f, 1.0f}
                            );
                        }
                        break;
                        
                    default:
                        // Default black projectile for any other tower type
                        renderer.renderRectangle(
                            {proj.pos.x - halfSize, proj.pos.y - halfSize, PROJECTILE_SIZE, PROJECTILE_SIZE},
                            {0.0f, 0.0f, 0.0f, 1.0f}
                        );
                        break;
                }
            }
        }

        // Draw tower range indicators using rectangles
        for (const auto& tower : towers) {
            // Draw a cross shape to indicate range
            renderer.renderRectangle(
                {tower.pos.x - tower.range, tower.pos.y - 2, tower.range * 2, 4},
                {1.0f, 1.0f, 1.0f, 0.2f}  // Semi-transparent white
            );
            renderer.renderRectangle(
                {tower.pos.x - 2, tower.pos.y - tower.range, 4, tower.range * 2},
                {1.0f, 1.0f, 1.0f, 0.2f}  // Semi-transparent white
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

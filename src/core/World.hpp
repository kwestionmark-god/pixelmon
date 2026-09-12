#pragma once
#include <string>
#include <vector>
#include <array>
#include <optional>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/MapStore.hpp"

namespace pm {

// Tile size in pixels (standard 16x16 for retro feel, scaled at render time).
constexpr int TILE_SIZE = 16;

// ---- Player character ------------------------------------------------------

enum class Direction : uint8_t {
    Down = 0,
    Left = 1,
    Right = 2,
    Up = 3,
};

struct Player {
    std::string name = "Red";
    int x = 0;           // tile position
    int y = 0;
    Direction facing = Direction::Down;
    bool moving = false;
    float moveProgress = 0.f;  // 0..1 during tile transition
    int moveSpeed = 8;   // tiles per second (for interpolation)

    // Party (references to Monster species by num).
    std::vector<int> party; // species nums (1-150)

    // Reset position.
    void setPosition(int tx, int ty);
};

// ---- World / Camera --------------------------------------------------------

struct Camera {
    float x = 0.f;  // pixel offset (top-left of view)
    float y = 0.f;
    float targetX = 0.f;
    float targetY = 0.f;
    float lerpSpeed = 10.f; // camera follow speed

    void follow(const Player& player, int mapWidthPx, int mapHeightPx, int viewW, int viewH, float dt, int tileSizePx);
    void snapTo(const Player& player, int mapWidthPx, int mapHeightPx, int viewW, int viewH, int tileSizePx);
};

// Main world state: current map, player, camera, tileset texture.
struct World {
    Map currentMap;
    Player player;
    Camera camera;
    sf::Texture* tileset = nullptr;  // shared tileset texture
    int tilesetColumns = 16;         // tiles per row in tileset
    int renderScale = 3;             // render scale factor (16 -> 48)
    bool inBattle = false;
    std::string pendingBattleType;   // "wild" or "trainer"
    int pendingEncounterSpecies = 0; // for wild
    int pendingTrainerIndex = -1;    // for trainer

    MapStore mapStore_;

    // pokered wLastMap memory: the last OUTDOOR map (and the door tile used).
    // Exit warps with target "last_map" return here; interior stairs don't
    // touch this memory.
    std::string lastMapId;
    int lastMapWarpX = 0;
    int lastMapWarpY = 0;

    // Load a map by ID from the MapStore.
    // placePlayer: set the default spawn (starting map / --map). Warps and
    // map connections pass false and position the player themselves.
    bool loadMap(const std::string& mapId, bool placePlayer = true);

    // Update world (movement, camera, encounter checks).
    void update(float dt);

    // Initialize the MapStore with the given data directory.
    void initMapStore(const std::string& dataDir) { mapStore_.init(dataDir); }

    // Try to move player in a direction. Returns true if move started.
    bool tryMove(Direction dir);

    // Check for wild encounter at player's current position.
    std::optional<int> checkWildEncounter() const;

    // Check for trainer interaction (A button press).
    std::optional<int> checkTrainerInteraction() const;

    // Check for warp at player position.
    std::optional<Map::Warp> checkWarp() const;

    // Get map dimensions in scaled pixels.
    int mapWidthPx() const { return currentMap.width * TILE_SIZE * renderScale; }
    int mapHeightPx() const { return currentMap.height * TILE_SIZE * renderScale; }
};

}  // namespace pm
#pragma once
#include <string>
#include <vector>
#include <array>
#include <optional>

#include "json/json.hpp"

namespace pm {

// Re-export TileType from World.hpp so Map can be used independently.
enum class TileType : uint8_t {
    Grass = 0,
    TallGrass = 1,
    Dirt = 2,
    Sand = 3,
    Water = 4,
    Tree = 5,
    Rock = 6,
    Building = 7,
    Wall = 8,
    Door = 9,
    Floor = 10,
    Stairs = 11,
    Sign = 12,
    NPC = 13,
};

inline bool isWalkable(TileType t) {
    switch (t) {
        case TileType::Grass:
        case TileType::TallGrass:
        case TileType::Dirt:
        case TileType::Sand:
        case TileType::Floor:
        case TileType::Stairs:
            return true;
        default:
            return false;
    }
}

inline bool isSolid(TileType t) { return !isWalkable(t); }

struct Tile {
    TileType type = TileType::Grass;
    int spriteIndex = 0;
    bool solid = false;
};

using MapLayer = std::vector<std::vector<Tile>>;

struct Map {
    std::string id;
    std::string name;
    int width = 0;
    int height = 0;
    std::string tileset;
    std::string tilesetFile;  // gfx file stem for the per-tileset atlas (e.g. "overworld")

    MapLayer ground;
    MapLayer objects;
    MapLayer overlay;

    struct EncounterZone {
        int x = 0, y = 0;
        int width = 0, height = 0;
        std::vector<std::pair<int, std::string>> table; // {weight, speciesId}
        bool isWater = false;
    };
    std::vector<EncounterZone> encounterZones;

    struct Trainer {
        std::string id;
        std::string name;
        int x = 0, y = 0;
        int sightRange = 3;
        std::vector<int> party;
        bool defeated = false;
    };
    std::vector<Trainer> trainers;

    struct Warp {
        int x = 0, y = 0;
        std::string targetMap;
        int warpId = 0;  // index into destination map's warp list (data-driven path)
        int targetX = 0, targetY = 0;  // legacy hardcoded fallback
    };
    std::vector<Warp> warps;

    struct NPC {
        int x = 0, y = 0;
        std::string sprite;
        std::string movement;
        std::string textId;
    };
    std::vector<NPC> npcs;

    struct Sign {
        int x = 0, y = 0;
        std::string textId;
    };
    std::vector<Sign> signs;

    struct Connection {
        std::string direction;  // north/south/east/west
        std::string targetMap;
        int offset = 0;
    };
    std::vector<Connection> connections;

    bool isSolid(int x, int y) const;
    const EncounterZone* getEncounterZone(int x, int y) const;
    const Trainer* getTrainerAt(int x, int y) const;
    const Warp* getWarpAt(int x, int y) const;
    const Connection* getConnection(const std::string& direction) const;
};

// Build a Map from a JSON file on disk.
bool buildFromJson(Map& map, const std::string& path);

}  // namespace pm

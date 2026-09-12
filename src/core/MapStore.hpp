#pragma once
#include <string>
#include <unordered_map>

#include "core/Map.hpp"

namespace pm {

// Loads maps from src/data/maps/ JSON files and caches them.
// Resolves warp target positions by looking up the destination map's warp entry.
struct MapStore {
    // Initialize with the data directory (maps live in dataDir/maps/).
    void init(const std::string& dataDir);

    // Load index.json from dataDir/maps/ and cache all maps.
    bool loadIndex();

    // Load a single map by id. Returns true on success.
    bool loadMap(const std::string& mapId, Map& out);

    // Get a pointer to a cached map, or nullptr if not loaded.
    const Map* get(const std::string& mapId) const;

    // Resolve a warp to an absolute (targetMap, targetX, targetY).
    // targetX/targetY are the position of the destination warp tile on the
    // target map (Gen 1 door-warp semantics: player lands on the warp tile).
    struct WarpTarget {
        std::string targetMap;
        int x = 0, y = 0;
    };
    WarpTarget resolveWarp(const Map& sourceMap, const Map::Warp& warp);

private:
    std::string mapsDir_;
    std::unordered_map<std::string, Map> cache_;
    // id → relative path within mapsDir_
    std::unordered_map<std::string, std::string> index_;
};

}  // namespace pm

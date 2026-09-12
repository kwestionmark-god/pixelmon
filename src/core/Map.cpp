#include "core/Map.hpp"

#include <fstream>
#include <iostream>

namespace pm {

bool Map::isSolid(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return true;
    if (!objects.empty() && y < (int)objects.size() && x < (int)objects[y].size()) {
        if (objects[y][x].solid) return true;
    }
    if (!ground.empty() && y < (int)ground.size() && x < (int)ground[y].size()) {
        if (ground[y][x].solid) return true;
    }
    return false;
}

const Map::EncounterZone* Map::getEncounterZone(int x, int y) const {
    for (const auto& zone : encounterZones) {
        if (x >= zone.x && x < zone.x + zone.width &&
            y >= zone.y && y < zone.y + zone.height) {
            return &zone;
        }
    }
    return nullptr;
}

const Map::Trainer* Map::getTrainerAt(int x, int y) const {
    for (const auto& t : trainers) {
        if (!t.defeated && t.x == x && t.y == y) return &t;
    }
    return nullptr;
}

const Map::Warp* Map::getWarpAt(int x, int y) const {
    for (const auto& w : warps) {
        if (w.x == x && w.y == y) return &w;
    }
    return nullptr;
}

const Map::Connection* Map::getConnection(const std::string& direction) const {
    for (const auto& c : connections) {
        if (c.direction == direction) return &c;
    }
    return nullptr;
}

namespace {

TileType jsonToTileType(int v) {
    return static_cast<TileType>(v);
}

}  // namespace

bool buildFromJson(Map& map, const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Map::buildFromJson: cannot open " << path << "\n";
        return false;
    }
    nlohmann::json j;
    in >> j;

    map.id = j.value("id", "");
    map.name = j.value("name", "");
    map.width = j.value("width", 0);
    map.height = j.value("height", 0);
    map.tileset = j.value("tileset", "OVERWORLD");
    map.tilesetFile = j.value("tilesetFile", "");

    // Ground layer
    auto jGround = j["ground"];
    map.ground.assign(map.height, std::vector<Tile>(map.width));
    for (int y = 0; y < map.height; ++y) {
        if (y >= (int)jGround.size()) break;
        auto row = jGround[y];
        for (int x = 0; x < map.width; ++x) {
            if (x >= (int)row.size()) break;
            auto& t = row[x];
            map.ground[y][x].type = jsonToTileType(t.value("tileType", 0));
            map.ground[y][x].spriteIndex = t.value("spriteIndex", 0);
            map.ground[y][x].solid = t.value("solid", false);
        }
    }

    // Objects layer (copy from ground as default, may be overridden)
    auto jObjects = j["objects"];
    map.objects.assign(map.height, std::vector<Tile>(map.width));
    for (int y = 0; y < map.height; ++y) {
        if (y >= (int)jObjects.size()) break;
        auto row = jObjects[y];
        for (int x = 0; x < map.width; ++x) {
            if (x >= (int)row.size()) break;
            auto& t = row[x];
            map.objects[y][x].type = jsonToTileType(t.value("tileType", 0));
            map.objects[y][x].spriteIndex = t.value("spriteIndex", 0);
            map.objects[y][x].solid = t.value("solid", false);
        }
    }

    // Overlay layer (all transparent/walkable by default)
    map.overlay.assign(map.height, std::vector<Tile>(map.width));
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x)
            map.overlay[y][x] = { TileType::Grass, 0, false };

    // Warps
    auto jWarps = j["warps"];
    map.warps.clear();
    for (const auto& w : jWarps) {
        Map::Warp warp;
        warp.x = w.value("x", 0);
        warp.y = w.value("y", 0);
        warp.targetMap = w.value("targetMap", "");
        warp.warpId = w.value("warpId", 0);
        map.warps.push_back(warp);
    }

    // NPCs
    auto jNpcs = j["npcs"];
    map.npcs.clear();
    for (const auto& n : jNpcs) {
        Map::NPC npc;
        npc.x = n.value("x", 0);
        npc.y = n.value("y", 0);
        npc.sprite = n.value("sprite", "");
        npc.movement = n.value("movement", "");
        npc.textId = n.value("textId", "");
        map.npcs.push_back(npc);
    }

    // Signs
    auto jSigns = j["signs"];
    map.signs.clear();
    for (const auto& s : jSigns) {
        Map::Sign sign;
        sign.x = s.value("x", 0);
        sign.y = s.value("y", 0);
        sign.textId = s.value("textId", "");
        map.signs.push_back(sign);
    }

    // Connections
    auto jConns = j["connections"];
    map.connections.clear();
    for (const auto& c : jConns) {
        Map::Connection conn;
        conn.direction = c.value("direction", "");
        conn.targetMap = c.value("targetMap", "");
        conn.offset = c.value("offset", 0);
        map.connections.push_back(conn);
    }

    return true;
}

}  // namespace pm

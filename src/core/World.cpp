#include "core/World.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

namespace pm {

// ---- Player implementation -------------------------------------------------

void Player::setPosition(int tx, int ty) {
    x = tx;
    y = ty;
    moving = false;
    moveProgress = 0.f;
}

// ---- Camera implementation -------------------------------------------------

void Camera::snapTo(const Player& player, int mapWidthPx, int mapHeightPx, int viewW, int viewH, int tileSizePx) {
    // Center camera on player.
    targetX = player.x * tileSizePx - viewW / 2.f + tileSizePx / 2.f;
    targetY = player.y * tileSizePx - viewH / 2.f + tileSizePx / 2.f;

    // Clamp to map bounds; center if the map is smaller than the view.
    auto clampAxis = [](float target, float mapPx, float viewPx) {
        if (mapPx <= viewPx) return (mapPx - viewPx) / 2.f;
        return std::clamp(target, 0.f, mapPx - viewPx);
    };
    targetX = clampAxis(targetX, float(mapWidthPx), float(viewW));
    targetY = clampAxis(targetY, float(mapHeightPx), float(viewH));

    x = targetX;
    y = targetY;
}

void Camera::follow(const Player& player, int mapWidthPx, int mapHeightPx, int viewW, int viewH, float dt, int tileSizePx) {
    // Target is player center.
    targetX = player.x * tileSizePx - viewW / 2.f + tileSizePx / 2.f;
    targetY = player.y * tileSizePx - viewH / 2.f + tileSizePx / 2.f;

    // Clamp target to map bounds; center if the map is smaller than the view.
    auto clampAxis = [](float target, float mapPx, float viewPx) {
        if (mapPx <= viewPx) return (mapPx - viewPx) / 2.f;
        return std::clamp(target, 0.f, mapPx - viewPx);
    };
    targetX = clampAxis(targetX, float(mapWidthPx), float(viewW));
    targetY = clampAxis(targetY, float(mapHeightPx), float(viewH));

    // Smooth follow (lerp) with actual dt.
    const float lerpFactor = 1.f - std::exp(-lerpSpeed * dt);
    x += (targetX - x) * lerpFactor;
    y += (targetY - y) * lerpFactor;
}

// ---- World implementation --------------------------------------------------

// ---- World::loadMap implementation ----

bool World::loadMap(const std::string& mapId, bool placePlayer) {
    if (!mapStore_.loadMap(mapId, currentMap)) {
        std::cerr << "World::loadMap: failed to load '" << mapId << "'\n";
        return false;
    }
    if (player.party.empty()) player.party = {1};
    if (placePlayer) {
        if (mapId == "pallet_town") {
            player.setPosition(5, 6);  // in front of Red's house door
        } else if (!currentMap.warps.empty()) {
            // --map start: land on this map's first warp (entry point).
            player.setPosition(currentMap.warps.front().x, currentMap.warps.front().y);
        }
    }
    int tileSizePx = TILE_SIZE * renderScale;
    camera.snapTo(player, mapWidthPx(), mapHeightPx(), 640, 480, tileSizePx);
    return true;
}

namespace {

const char* dirToConnection(Direction dir) {
    switch (dir) {
        case Direction::Up:    return "north";
        case Direction::Down:  return "south";
        case Direction::Left:  return "west";
        case Direction::Right: return "east";
    }
    return "";
}

}  // namespace

bool World::tryMove(Direction dir) {
    if (player.moving) return false;

    int dx = 0, dy = 0;
    switch (dir) {
        case Direction::Up:    dy = -1; break;
        case Direction::Down:  dy = 1; break;
        case Direction::Left:  dx = -1; break;
        case Direction::Right: dx = 1; break;
    }

    int newX = player.x + dx;
    int newY = player.y + dy;

    const bool oob = newX < 0 || newX >= currentMap.width ||
                     newY < 0 || newY >= currentMap.height;
    if (oob) {
        // Map::isSolid treats out-of-bounds as solid. Allow the step only
        // when this edge has a map connection (Pallet Town → Route 1, etc.).
        if (!currentMap.getConnection(dirToConnection(dir))) {
            player.facing = dir;
            return false;
        }
    } else if (currentMap.isSolid(newX, newY)) {
        player.facing = dir; // Face direction even if blocked.
        return false;
    }

    // Start movement.
    player.facing = dir;
    player.moving = true;
    player.moveProgress = 0.f;
    return true;
}

void World::update(float dt) {
    if (inBattle) return;

    // Update player movement interpolation.
    if (player.moving) {
        player.moveProgress += dt * player.moveSpeed;
        if (player.moveProgress >= 1.f) {
            player.moveProgress = 1.f;
            player.moving = false;
            // Snap to new tile position.
            int dx = 0, dy = 0;
            switch (player.facing) {
                case Direction::Up:    dy = -1; break;
                case Direction::Down:  dy = 1; break;
                case Direction::Left:  dx = -1; break;
                case Direction::Right: dx = 1; break;
            }
            player.x += dx;
            player.y += dy;

            // Walking off the map edge → connected overworld map.
            const bool oob = player.x < 0 || player.x >= currentMap.width ||
                             player.y < 0 || player.y >= currentMap.height;
            if (oob) {
                const char* connDir = dirToConnection(player.facing);
                const auto* conn = currentMap.getConnection(connDir);
                if (conn) {
                    Map target;
                    if (mapStore_.loadMap(conn->targetMap, target) &&
                        target.width > 0 && target.height > 0) {
                        int newx = player.x;
                        int newy = player.y;
                        // pokered connection offset is in blocks; spawn shift is offset * -2 tiles.
                        if (std::string(connDir) == "north" || std::string(connDir) == "south") {
                            newx = player.x - conn->offset * 2;
                            newy = (std::string(connDir) == "north") ? target.height - 1 : 0;
                        } else {
                            newy = player.y - conn->offset * 2;
                            newx = (std::string(connDir) == "west") ? target.width - 1 : 0;
                        }
                        newx = std::clamp(newx, 0, target.width - 1);
                        newy = std::clamp(newy, 0, target.height - 1);
                        if (loadMap(conn->targetMap, /*placePlayer=*/false)) {
                            player.setPosition(newx, newy);
                            int tileSizePx = TILE_SIZE * renderScale;
                            camera.snapTo(player, mapWidthPx(), mapHeightPx(), 640, 480, tileSizePx);
                            std::cout << "Connected to " << conn->targetMap << " at " << newx << "," << newy << "\n";
                        }
                    }
                } else {
                    player.x = std::clamp(player.x, 0, std::max(0, currentMap.width - 1));
                    player.y = std::clamp(player.y, 0, std::max(0, currentMap.height - 1));
                }
            } else {
                // Check for wild encounter after moving into grass.
                if (auto species = checkWildEncounter()) {
                    pendingBattleType = "wild";
                    pendingEncounterSpecies = *species;
                    inBattle = true;
                }
            }

            // Check for warp after moving.
            if (auto warp = checkWarp()) {
                auto resolved = mapStore_.resolveWarp(currentMap, *warp);
                // pokered semantics: wLastMap remembers the last OUTDOOR map
                // (and the door tile used); LAST_MAP = "go back outside".
                const std::string fromMapId = currentMap.id;
                const bool fromOutdoor = (currentMap.tileset == "OVERWORLD");
                if (resolved.targetMap == "last_map") {
                    if (lastMapId.empty()) {
                        resolved.targetMap.clear();  // nothing to return to
                    } else {
                        resolved.targetMap = lastMapId;
                        resolved.x = lastMapWarpX;
                        resolved.y = lastMapWarpY;
                    }
                }
                if (!resolved.targetMap.empty() && loadMap(resolved.targetMap, /*placePlayer=*/false)) {
                    if (fromOutdoor) {
                        lastMapId = fromMapId;
                        lastMapWarpX = warp->x;
                        lastMapWarpY = warp->y;
                    }
                    player.setPosition(resolved.x, resolved.y);
                    int tileSizePx = TILE_SIZE * renderScale;
                    camera.snapTo(player, mapWidthPx(), mapHeightPx(), 640, 480, tileSizePx);
                    std::cout << "Warped to " << resolved.targetMap << " at " << resolved.x << "," << resolved.y << "\n";
                } else {
                    std::cerr << "Failed to load map: " << resolved.targetMap << "\n";
                }
            }
        }
    }

    // Update camera.
    int tileSizePx = TILE_SIZE * renderScale;
    camera.follow(player, mapWidthPx(), mapHeightPx(), 640, 480, dt, tileSizePx);
}

std::optional<int> World::checkWildEncounter() const {
    const auto* zone = currentMap.getEncounterZone(player.x, player.y);
    if (!zone) return std::nullopt;

    // Only trigger on tall grass / water tiles.
    TileType tileType = TileType::Grass;
    if (player.y < (int)currentMap.ground.size() && player.x < (int)currentMap.ground[player.y].size()) {
        tileType = currentMap.ground[player.y][player.x].type;
    }
    if (tileType != TileType::TallGrass && tileType != TileType::Water) return std::nullopt;

    // Encounter rate: ~10% per step in tall grass.
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 100);
    if (dist(gen) > 10) return std::nullopt;

    // Pick species from encounter table (weighted).
    int totalWeight = 0;
    for (const auto& entry : zone->table) totalWeight += entry.first;
    std::uniform_int_distribution<> pickDist(1, totalWeight);
    int roll = pickDist(gen);
    int accum = 0;
    for (const auto& entry : zone->table) {
        accum += entry.first;
        if (roll <= accum) {
            // Parse species num from ID like "PM-016".
            return std::stoi(entry.second.substr(3));
        }
    }
    return std::nullopt;
}

std::optional<int> World::checkTrainerInteraction() const {
    // Check adjacent tiles in facing direction for trainer.
    int dx = 0, dy = 0;
    switch (player.facing) {
        case Direction::Up:    dy = -1; break;
        case Direction::Down:  dy = 1; break;
        case Direction::Left:  dx = -1; break;
        case Direction::Right: dx = 1; break;
    }

    int checkX = player.x + dx;
    int checkY = player.y + dy;

    const auto* trainer = currentMap.getTrainerAt(checkX, checkY);
    if (trainer && !trainer->defeated) {
        // Find trainer index.
        for (size_t i = 0; i < currentMap.trainers.size(); ++i) {
            if (currentMap.trainers[i].id == trainer->id) return (int)i;
        }
    }
    return std::nullopt;
}

std::optional<Map::Warp> World::checkWarp() const {
    const auto* warp = currentMap.getWarpAt(player.x, player.y);
    if (warp) return *warp;
    return std::nullopt;
}

}  // namespace pm
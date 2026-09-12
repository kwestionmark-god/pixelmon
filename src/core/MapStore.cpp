#include "core/MapStore.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace pm {

namespace {

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

}  // namespace

void MapStore::init(const std::string& dataDir) {
    mapsDir_ = dataDir + "/maps";
    // Fallback: if dataDir/maps doesn't exist, try src/data/maps (project layout).
    if (!std::filesystem::exists(mapsDir_ + "/index.json")) {
        if (std::filesystem::exists("src/data/maps/index.json")) {
            mapsDir_ = "src/data/maps";
        }
    }
 
}

bool MapStore::loadIndex() {
    std::ifstream in(mapsDir_ + "/index.json");
    if (!in) {
        std::cerr << "MapStore: cannot open " << mapsDir_ << "/index.json\n";
        return false;
    }
    nlohmann::json j;
    in >> j;
    if (!j.is_array()) {
        std::cerr << "MapStore: index.json is not an array\n";
        return false;
    }
    for (const auto& entry : j) {
        std::string id = toLower(entry.value("id", ""));
        std::string path = entry.value("path", "");
        if (id.empty() || path.empty()) continue;
        index_[id] = path;
    }
    return true;
}

bool MapStore::loadMap(const std::string& mapId, Map& out) {
    if (index_.empty() && !loadIndex()) return false;
    std::string id = toLower(mapId);
    auto it = cache_.find(id);
    if (it != cache_.end()) {
        out = it->second;
        return true;
    }
    auto pit = index_.find(id);
    if (pit == index_.end()) {
        std::cerr << "MapStore: unknown map '" << id << "'\n";
        return false;
    }
    Map m;
    if (!buildFromJson(m, mapsDir_ + "/" + pit->second)) return false;
    cache_[id] = m;
    out = m;
    return true;
}

const Map* MapStore::get(const std::string& mapId) const {
    auto it = cache_.find(toLower(mapId));
    return it != cache_.end() ? &it->second : nullptr;
}

MapStore::WarpTarget MapStore::resolveWarp(const Map&, const Map::Warp& warp) {
    WarpTarget result;
    result.targetMap = toLower(warp.targetMap);
    if (result.targetMap == "last_map") return result;  // handled by World

    // Load the target map now so its warp list is available.
    Map target;
    if (!loadMap(result.targetMap, target)) {
        std::cerr << "MapStore: resolveWarp: cannot load target map '"
                  << result.targetMap << "'\n";
        return result;
    }
    if (warp.warpId >= 0 && warp.warpId < (int)target.warps.size()) {
        result.x = target.warps[warp.warpId].x;
        result.y = target.warps[warp.warpId].y;
    } else {
        // Fallback: first warp on target map
        if (!target.warps.empty()) {
            result.x = target.warps[0].x;
            result.y = target.warps[0].y;
        }
    }
    return result;
}

}  // namespace pm

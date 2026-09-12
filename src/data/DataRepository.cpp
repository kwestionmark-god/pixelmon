#include "data/DataRepository.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>

#include "json/json.hpp"

using json = nlohmann::json;

namespace pm {

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Derive the lookup key used for moves: lowercase, spaces -> hyphens.
//   "Mega Drain" -> "mega-drain"  (matches learnset move slugs)
static std::string moveSlug(const std::string& name) {
    std::string s = lower(name);
    for (char& c : s) if (c == ' ') c = '-';
    return s;
}

bool DataRepository::load(const std::string& baseDir) {
    const std::string monsterPath = baseDir + "/monsters.json";
    const std::string movePath = baseDir + "/moves.json";

    std::ifstream mf(monsterPath);
    if (!mf.good()) return false;
    std::ifstream vf(movePath);
    if (!vf.good()) return false;

    // Moves are loaded first so the slug index exists when monsters reference them.
    {
        json j;
        vf >> j;
        if (!j.is_array()) { std::cerr << "data: moves.json is not an array\n"; return false; }

        moves_.reserve(j.size());
        for (const auto& m : j) {
            Move mv;
            mv.id = m.value("id", std::string{});
            mv.name = m.value("name", std::string{});
            mv.type = typeFromName(m.value("type", std::string("normal")));
            mv.category = categoryFromName(m.value("category", std::string("status")));
            mv.power = m.value("power", 0);
            mv.accuracy = m.value("accuracy", 0);
            mv.pp = m.value("pp", 0);
            mv.flavor = m.value("flavor", std::string{});
            moves_.push_back(mv);
            bySlug_[moveSlug(mv.name)] = static_cast<int>(moves_.size() - 1);
        }
    }

    // Monsters.
    {
        json j;
        mf >> j;
        if (!j.is_array()) { std::cerr << "data: monsters.json is not an array\n"; return false; }

        monsters_.reserve(j.size());
        for (const auto& m : j) {
            Monster mon;
            mon.id = m.value("id", std::string{});
            mon.num = m.value("num", 0);
            mon.name = m.value("name", std::string{});
            mon.growthRate = m.value("growthRate", std::string{});
            mon.catchRate = m.value("catchRate", 0);
            mon.baseFriendship = m.value("baseFriendship", 0);
            mon.baseExp = m.value("baseExp", 0);
            mon.height = m.value("height", 0.f);
            mon.weight = m.value("weight", 0.f);
            mon.genderRatio = m.value("genderRatio", -1.f);

            for (const auto& t : m.value("types", std::vector<std::string>{}))
                mon.types.push_back(typeFromName(t));

            const auto& bs = m.value("baseStats", json::object());
            mon.baseStats.hp = bs.value("hp", 0);
            mon.baseStats.attack = bs.value("attack", 0);
            mon.baseStats.defense = bs.value("defense", 0);
            mon.baseStats.spAttack = bs.value("spAttack", 0);
            mon.baseStats.spDefense = bs.value("spDefense", 0);
            mon.baseStats.speed = bs.value("speed", 0);

            const auto& ey = m.value("evYield", json::object());
            mon.evYield.hp = ey.value("hp", 0);
            mon.evYield.attack = ey.value("attack", 0);
            mon.evYield.defense = ey.value("defense", 0);
            mon.evYield.spAttack = ey.value("spAttack", 0);
            mon.evYield.spDefense = ey.value("spDefense", 0);
            mon.evYield.speed = ey.value("speed", 0);

            mon.abilities = m.value("abilities", std::vector<std::string>{});
            mon.eggGroups = m.value("eggGroups", std::vector<std::string>{});

            for (const auto& l : m.value("learnset", std::vector<json>{}))
                mon.learnset.push_back({ l.value("level", 0), l.value("move", std::string{}) });

            for (const auto& e : m.value("evolution", std::vector<json>{})) {
                Evolution ev;
                ev.to = e.value("to", std::string{});
                ev.method = e.value("method", std::string{});
                ev.requirement = e.value("requirement", std::string{});
                if (e.contains("condition")) ev.condition = e.value("condition", std::string{});
                mon.evolution.push_back(ev);
            }

            const auto& dex = m.value("pokedex", json::object());
            mon.category = dex.value("category", std::string{});
            mon.dexEntry = dex.value("entry", std::string{});

            const auto& spr = m.value("sprites", json::object());
            mon.frontSprite = spr.value("front", std::string{});
            mon.backSprite = spr.value("back", std::string{});
            mon.iconSprite = spr.value("icon", std::string{});

            monsters_.push_back(mon);
            byName_[lower(mon.name)] = static_cast<int>(monsters_.size() - 1);
            byNum_[mon.num] = static_cast<int>(monsters_.size() - 1);
        }
    }

    return true;
}

const Monster* DataRepository::monsterById(const std::string& id) const {
    auto it = byId_.find(id);
    return it == byId_.end() ? nullptr : &monsters_[it->second];
}

const Monster* DataRepository::monsterByName(const std::string& name) const {
    auto it = byName_.find(lower(name));
    return it == byName_.end() ? nullptr : &monsters_[it->second];
}

const Monster* DataRepository::monsterByNum(int num) const {
    auto it = byNum_.find(num);
    return it == byNum_.end() ? nullptr : &monsters_[it->second];
}

const Move* DataRepository::moveBySlug(const std::string& slug) const {
    auto it = bySlug_.find(moveSlug(slug));
    return it == bySlug_.end() ? nullptr : &moves_[it->second];
}

}  // namespace pm

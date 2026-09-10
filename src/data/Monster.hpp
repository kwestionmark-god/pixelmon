#pragma once
#include <optional>
#include <string>
#include <vector>

#include "types.hpp"

namespace pm {

// Per-stat block. Base stats come from the species template; instance stats
// are computed (base + IVs + EVs + nature + level).
struct Stats {
    int hp = 0;
    int attack = 0;
    int defense = 0;
    int spAttack = 0;
    int spDefense = 0;
    int speed = 0;

    int operator[](int i) const;  // 0=hp .. 5=speed
};

// A level-up learnset entry.
struct LevelUpMove {
    int level = 0;
    std::string move;  // move slug, e.g. "tackle"
};

// An evolution step (GDD §2.3 / factory spec).
struct Evolution {
    std::string to;          // target species id/name
    std::string method;      // level, stone, trade, time, location, move
    std::string requirement; // level number, item name, move name, etc.
    std::optional<std::string> condition;  // day, night, snow
};

// Static species template (what PokeAPI describes about a species).
struct Monster {
    std::string id;          // "PM-001"
    int num = 0;             // PokeAPI id (1..150)
    std::string name;        // "bulbasaur"

    std::vector<Type> types;
    Stats baseStats;
    Stats evYield;           // only non-zero EVs are populated

    std::vector<std::string> abilities;  // primary first; [1] optional
    float height = 0.f;   // metres
    float weight = 0.f;   // kg
    float genderRatio = -1.f;  // female %; -1 = genderless
    std::vector<std::string> eggGroups;
    std::string growthRate;  // raw name from PokeAPI
    int catchRate = 0;
    int baseFriendship = 0;
    int baseExp = 0;

    std::vector<LevelUpMove> learnset;
    std::vector<Evolution> evolution;

    std::string category;    // pokedex category ("Seed")
    std::string dexEntry;    // pokedex flavor text

    std::string frontSprite;
    std::string backSprite;
    std::string iconSprite;

    // Convenience: is this species dual-typed?
    bool isDualType() const { return types.size() > 1; }
};

// An individual captured creature (GDD §2.3).
struct MonsterInstance {
    std::string uid;
    std::string monsterId;    // references Monster::id
    std::optional<std::string> nickname;
    int level = 1;
    long exp = 0;

    Stats stats;              // calculated current stats
    Stats ivs;                // 0..31 per stat
    Stats evs;                // 0..255 per stat (510 cap applied elsewhere)
    Nature nature = Nature::Hardy;
    std::string ability;      // active ability

    // Up to 4 known moves (by move slug).
    struct Slot { std::string move; int pp = 0; int maxPp = 0; };
    std::vector<Slot> moves;

    int currentHp = 0;
    bool shiny = false;
};

}  // namespace pm

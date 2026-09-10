#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "data/Monster.hpp"
#include "data/Move.hpp"

namespace pm {

// Loads the generated JSON databases and indexes them for fast lookup.
// Data files (monsters.json / moves.json) are produced by scripts/generate-data.mjs
// and copied next to the executable by CMake.
class DataRepository {
public:
    // Returns false if either file is missing or malformed.
    bool load(const std::string& baseDir);

    const std::vector<Monster>& monsters() const { return monsters_; }
    const std::vector<Move>& moves() const { return moves_; }

    const Monster* monsterById(const std::string& id) const;
    const Monster* monsterByName(const std::string& name) const;  // case-insensitive
    const Monster* monsterByNum(int num) const;
    const Move* moveBySlug(const std::string& slug) const;

    int count() const { return static_cast<int>(monsters_.size()); }

private:
    std::vector<Monster> monsters_;
    std::vector<Move> moves_;
    std::unordered_map<std::string, int> byId_;      // id -> index
    std::unordered_map<std::string, int> bySlug_;    // move slug -> index
    std::unordered_map<std::string, int> byName_;    // lowercase name -> index
    std::unordered_map<int, int> byNum_;             // pokeapi num -> index
};

}  // namespace pm

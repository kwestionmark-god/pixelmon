#pragma once
#include <string>

#include "types.hpp"

namespace pm {

// A move definition (PokeAPI move data).
struct Move {
    std::string id;          // "MV-0001"
    std::string name;        // "Tackle"
    Type type = Type::Normal;
    MoveCategory category = MoveCategory::Physical;
    int power = 0;           // 0 for status moves
    int accuracy = 0;        // 0 = never misses / N/A
    int pp = 0;
    std::string flavor;      // English flavor text
};

}  // namespace pm

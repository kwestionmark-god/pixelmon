#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pm {

// The 18 battle types (Kanto uses the original 15; Dark/Fairy/Steel exist in
// the chart for richer matchups). Stored as a compact enum for OOP clarity.
enum class Type : uint8_t {
    Normal, Fire, Water, Grass, Electric, Ice, Fighting, Poison, Ground,
    Flying, Psychic, Bug, Rock, Ghost, Dragon, Dark, Fairy, Steel,
};

// Move damage category (matches PokeAPI damage_class).
enum class MoveCategory : uint8_t { Physical, Special, Status };

// Growth rates drive the EXP-up table (which level => how much cumulative EXP).
enum class GrowthRate : uint8_t {
    FixedSlow,      // slow
    PeriodicMid,    // medium-slow
    FixedMedium,    // medium-fast
    FixedFast,      // fast
    PeriodicLow,    // erratic
    PeriodicHigh,   // fluctuating
};

// Natures: +1 stat, -1 stat (GDD §3.4).
enum class Nature : uint8_t {
    Hardy, Lonely, Brave, Adamant, Naughty, Bold, Docile, Relaxed, Impish,
    Lax, Timid, Serious, Jolly, Naive, Modest, Mild, Quiet, Bashful, Rash,
    Calm, Gentle, Sassy, Careful, Quirky,
};

// ---- string <-> enum mapping ----------------------------------------------
Type typeFromName(std::string_view name);
std::string_view typeName(Type t);

MoveCategory categoryFromName(std::string_view name);
std::string_view categoryName(MoveCategory c);

GrowthRate growthFromName(std::string_view name);
std::string_view growthName(GrowthRate g);

// Full type-effectiveness matrix (row attacks column). 1.0 = normal, 2.0 =
// super effective, 0.5 = not very effective, 0.0 = no effect.
double typeMultiplier(Type attacker, Type defender);

// 18x18 matrix, indexed [attacker][defender].
const double (&typeChart())[18][18];

}  // namespace pm

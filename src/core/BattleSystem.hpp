#pragma once
#include "data/Monster.hpp"
#include "data/Move.hpp"

namespace pm {

struct DamageResult {
    double damage;      // estimate (may be fractional for demo clarity)
    double effectiveness;  // type multiplier applied (0 = no effect)
    bool isNoEffect;    // true when the move can't hurt the defender
    bool stab;          // same-type-attack bonus applied
};

// Gen-1-style damage estimate for a single attack (no RNG), demonstrating the
// type chart + base stats at runtime. Zero-power (status) moves return damage 0.
DamageResult computeDamage(const Monster& attacker, const Monster& defender,
                           const Move& move, int attackerLevel);

}  // namespace pm

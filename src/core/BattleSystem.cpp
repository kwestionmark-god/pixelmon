#include "core/BattleSystem.hpp"

#include "types.hpp"

namespace pm {

DamageResult computeDamage(const Monster& attacker, const Monster& defender,
                           const Move& move, int attackerLevel) {
    DamageResult r{};
    r.damage = 0.0;
    r.effectiveness = 1.0;

    // Stack type multipliers across the defender's type(s).
    for (Type t : defender.types)
        r.effectiveness *= typeMultiplier(move.type, t);
    r.isNoEffect = r.effectiveness <= 0.0;

    // Same-type attack bonus.
    r.stab = false;
    for (Type t : attacker.types) {
        if (t == move.type) { r.stab = true; break; }
    }

    // Zero-power moves are status-only -> no damage.
    if (move.power <= 0) return r;

    // Gen-1 style: special moves use the Special stat (spAtk vs spDef).
    int atkStat, defStat;
    if (move.category == MoveCategory::Special) {
        atkStat = attacker.baseStats.spAttack;
        defStat = defender.baseStats.spDefense;
    } else {
        atkStat = attacker.baseStats.attack;
        defStat = defender.baseStats.defense;
    }

    if (defStat <= 0 || atkStat <= 0) return r;

    double base = (2.0 * attackerLevel / 5.0 + 2.0) * move.power *
                  (atkStat / static_cast<double>(defStat)) / 50.0 + 2.0;
    double stabMult = r.stab ? 1.5 : 1.0;
    r.damage = base * stabMult * r.effectiveness;
    return r;
}

}  // namespace pm

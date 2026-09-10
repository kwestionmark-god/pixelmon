#include "core/BattleSystem.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>

#include "data/DataRepository.hpp"
#include "types.hpp"

namespace pm {

// ---- Damage computation ----------------------------------------------------

DamageResult computeDamage(const Monster& attacker, const Monster& defender,
                           const Move& move, int attackerLevel) {
    DamageResult r{};
    r.damage = 0.0;
    r.effectiveness = 1.0;

    for (Type t : defender.types)
        r.effectiveness *= typeMultiplier(move.type, t);
    r.isNoEffect = r.effectiveness <= 0.0;

    r.stab = false;
    for (Type t : attacker.types) {
        if (t == move.type) { r.stab = true; break; }
    }

    if (move.power <= 0) return r;

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

// ---- BattleMon helpers -----------------------------------------------------

bool BattleMon::canAct() const {
    if (isFainted()) return false;
    if (status == StatusCondition::Sleep) return false;
    if (status == StatusCondition::Freeze) return false;
    // Paralysis handled in turn queue (speed quartered) + 25% full paralysis chance
    return true;
}

// ---- Stat calculation (simplified Gen-1 style) -----------------------------

static int calcStat(int base, int iv, int ev, int level, Nature nature, bool isHp) {
    // Gen-1 style: ((base + iv) * 2 + ev/4) * level / 100 + level + 10 (HP) or +5 (others)
    // Simplified: no nature for now, no IV/EV randomness for demo.
    double val = (base * 2.0 + iv + ev / 4.0) * level / 100.0;
    if (isHp) return static_cast<int>(val) + level + 10;
    return static_cast<int>(val) + 5;
}

static Stats calcBattleStats(const Monster& species, int level) {
    Stats s;
    // For demo: perfect IVs (31), no EVs, neutral nature
    s.hp = calcStat(species.baseStats.hp, 31, 0, level, Nature::Hardy, true);
    s.attack = calcStat(species.baseStats.attack, 31, 0, level, Nature::Hardy, false);
    s.defense = calcStat(species.baseStats.defense, 31, 0, level, Nature::Hardy, false);
    s.spAttack = calcStat(species.baseStats.spAttack, 31, 0, level, Nature::Hardy, false);
    s.spDefense = calcStat(species.baseStats.spDefense, 31, 0, level, Nature::Hardy, false);
    s.speed = calcStat(species.baseStats.speed, 31, 0, level, Nature::Hardy, false);
    return s;
}

// ---- Battle creation -------------------------------------------------------

struct BattleMonInit {
    const Monster* species = nullptr;
    std::string nickname;
    int level = 1;
    std::vector<std::string> moveSlugs;  // up to 4
};

static BattleMon createBattleMon(const BattleMonInit& init, const DataRepository& data) {
    BattleMon bm;
    bm.species = init.species;
    bm.nickname = init.nickname.empty() ? init.species->name : init.nickname;
    bm.level = init.level;
    bm.stats = calcBattleStats(*init.species, init.level);
    bm.maxHp = bm.stats.hp;
    bm.currentHp = bm.maxHp;

    for (const std::string& slug : init.moveSlugs) {
        const Move* mv = data.moveBySlug(slug);
        if (mv) {
            bm.moves.push_back({mv, mv->pp, mv->pp});
        }
    }
    // If fewer than 4 moves, pad with Struggle (we'll handle struggle separately)
    return bm;
}

BattleContext createDemoBattle(const DataRepository& data,
                               int playerSpeciesNum, int enemySpeciesNum, int level) {
    BattleContext ctx;

    const Monster* playerSpecies = data.monsterByNum(playerSpeciesNum);
    const Monster* enemySpecies = data.monsterByNum(enemySpeciesNum);

    if (!playerSpecies || !enemySpecies) {
        // Fallback to first available
        if (data.count() > 0) {
            playerSpecies = &data.monsters()[0];
            enemySpecies = &data.monsters()[std::min(1, data.count() - 1)];
        }
    }

    // Pick up to 4 moves from learnset at this level
    auto pickMoves = [&](const Monster* sp) {
        std::vector<std::string> slugs;
        for (const auto& lm : sp->learnset) {
            if (lm.level <= level && slugs.size() < 4) {
                slugs.push_back(lm.move);
            }
        }
        // If no level-up moves, give Tackle as fallback
        if (slugs.empty()) slugs.push_back("tackle");
        return slugs;
    };

    BattleMonInit pInit{playerSpecies, playerSpecies->name, level, pickMoves(playerSpecies)};
    BattleMonInit eInit{enemySpecies, enemySpecies->name, level, pickMoves(enemySpecies)};

    ctx.playerTeam.push_back(createBattleMon(pInit, data));
    ctx.enemyTeam.push_back(createBattleMon(eInit, data));

    ctx.playerActiveIdx = 0;
    ctx.enemyActiveIdx = 0;
    ctx.phase = BattlePhase::TurnStart;
    ctx.result = BattleResult::Ongoing;
    ctx.turnNumber = 0;

    log(ctx, "A wild " + enemySpecies->name + " appeared!");
    log(ctx, "Go! " + playerSpecies->name + "!", true);

    return ctx;
}

// ---- Logging ---------------------------------------------------------------

void log(BattleContext& ctx, std::string_view text, bool isPlayer) {
    ctx.log.push_back({std::string(text), isPlayer});
}

// ---- Status helpers --------------------------------------------------------

std::string_view statusName(StatusCondition s) {
    switch (s) {
        case StatusCondition::Poison: return "PSN";
        case StatusCondition::Burn:   return "BRN";
        case StatusCondition::Paralysis: return "PAR";
        case StatusCondition::Sleep:  return "SLP";
        case StatusCondition::Freeze: return "FRZ";
        case StatusCondition::Fainted: return "FNT";
        default: return "";
    }
}

// ---- Turn queue ------------------------------------------------------------

static int effectiveSpeed(const BattleMon& mon) {
    int spd = mon.stats.speed;
    if (mon.status == StatusCondition::Paralysis) spd /= 4;
    return spd;
}

static std::vector<TurnEntry> buildTurnQueue(const BattleContext& ctx) {
    std::vector<TurnEntry> queue;
    const BattleMon* pMon = &ctx.playerTeam[ctx.playerActiveIdx];
    const BattleMon* eMon = &ctx.enemyTeam[ctx.enemyActiveIdx];

    if (pMon->canAct()) {
        queue.push_back({const_cast<BattleMon*>(pMon), true, effectiveSpeed(*pMon)});
    }
    if (eMon->canAct()) {
        queue.push_back({const_cast<BattleMon*>(eMon), false, effectiveSpeed(*eMon)});
    }

    // Sort by speed descending (higher speed acts first)
    std::sort(queue.begin(), queue.end(),
              [](const TurnEntry& a, const TurnEntry& b) {
                  return a.speed > b.speed;
              });
    return queue;
}

// ---- Status tick -----------------------------------------------------------

void tickStatus(BattleContext& ctx) {
    BattleMon& pMon = ctx.playerTeam[ctx.playerActiveIdx];
    BattleMon& eMon = ctx.enemyTeam[ctx.enemyActiveIdx];

    // Player mon status tick
    if (pMon.status == StatusCondition::Poison) {
        int dmg = std::max(1, pMon.maxHp / 16);
        pMon.currentHp = std::max(0, pMon.currentHp - dmg);
        log(ctx, pMon.nickname + " is hurt by poison!");
    } else if (pMon.status == StatusCondition::Burn) {
        int dmg = std::max(1, pMon.maxHp / 16);
        pMon.currentHp = std::max(0, pMon.currentHp - dmg);
        log(ctx, pMon.nickname + " is hurt by its burn!");
    } else if (pMon.status == StatusCondition::Sleep) {
        if (pMon.sleepTurns > 0) {
            pMon.sleepTurns--;
            log(ctx, pMon.nickname + " is fast asleep.");
        } else {
            pMon.status = StatusCondition::None;
            log(ctx, pMon.nickname + " woke up!");
        }
    } else if (pMon.status == StatusCondition::Freeze) {
        // 20% chance to thaw in Gen 1
        static std::mt19937 rng(std::random_device{}());
        if (std::uniform_int_distribution<>(1, 100)(rng) <= 20) {
            pMon.status = StatusCondition::None;
            log(ctx, pMon.nickname + " thawed out!");
        } else {
            log(ctx, pMon.nickname + " is frozen solid!");
        }
    }

    // Enemy mon status tick
    if (eMon.status == StatusCondition::Poison) {
        int dmg = std::max(1, eMon.maxHp / 16);
        eMon.currentHp = std::max(0, eMon.currentHp - dmg);
        log(ctx, "Enemy " + eMon.nickname + " is hurt by poison!");
    } else if (eMon.status == StatusCondition::Burn) {
        int dmg = std::max(1, eMon.maxHp / 16);
        eMon.currentHp = std::max(0, eMon.currentHp - dmg);
        log(ctx, "Enemy " + eMon.nickname + " is hurt by its burn!");
    } else if (eMon.status == StatusCondition::Sleep) {
        if (eMon.sleepTurns > 0) {
            eMon.sleepTurns--;
            log(ctx, "Enemy " + eMon.nickname + " is fast asleep.");
        } else {
            eMon.status = StatusCondition::None;
            log(ctx, "Enemy " + eMon.nickname + " woke up!");
        }
    } else if (eMon.status == StatusCondition::Freeze) {
        static std::mt19937 rng(std::random_device{}());
        if (std::uniform_int_distribution<>(1, 100)(rng) <= 20) {
            eMon.status = StatusCondition::None;
            log(ctx, "Enemy " + eMon.nickname + " thawed out!");
        } else {
            log(ctx, "Enemy " + eMon.nickname + " is frozen solid!");
        }
    }
}

// ---- Move execution --------------------------------------------------------

int applyDamage(BattleContext& ctx, bool targetIsPlayer, int damage) {
    BattleMon& target = targetIsPlayer
        ? ctx.playerTeam[ctx.playerActiveIdx]
        : ctx.enemyTeam[ctx.enemyActiveIdx];

    int actualDmg = std::min(damage, target.currentHp);
    target.currentHp -= actualDmg;
    if (target.currentHp < 0) target.currentHp = 0;
    return actualDmg;
}

bool tryInflictStatus(BattleContext& ctx, bool targetIsPlayer, StatusCondition status) {
    BattleMon& target = targetIsPlayer
        ? ctx.playerTeam[ctx.playerActiveIdx]
        : ctx.enemyTeam[ctx.enemyActiveIdx];

    if (target.status != StatusCondition::None || target.isFainted()) return false;

    // Type immunities
    if (status == StatusCondition::Poison) {
        for (Type t : target.species->types) {
            if (t == Type::Poison || t == Type::Steel) return false;
        }
    } else if (status == StatusCondition::Burn) {
        for (Type t : target.species->types) {
            if (t == Type::Fire) return false;
        }
    } else if (status == StatusCondition::Paralysis) {
        // Gen 1: no type immunity for paralysis (Electric types CAN be paralyzed)
    } else if (status == StatusCondition::Freeze) {
        for (Type t : target.species->types) {
            if (t == Type::Ice) return false;
        }
    }

    target.status = status;
    if (status == StatusCondition::Sleep) {
        static std::mt19937 rng(std::random_device{}());
        target.sleepTurns = std::uniform_int_distribution<>(1, 3)(rng); // 1-3 turns
    }
    return true;
}

// Move effect: returns damage (0 for status moves), handles status infliction
static int executeMoveInternal(BattleContext& ctx, BattleMon& attacker, BattleMon& defender,
                               bool attackerIsPlayer, const Move& move, const DataRepository& data) {
    // Find the move slot to decrement PP
    BattleMon::MoveSlot* moveSlot = nullptr;
    for (auto& s : attacker.moves) {
        if (s.move && s.move->id == move.id) {
            moveSlot = &s;
            break;
        }
    }
    if (!moveSlot || moveSlot->pp <= 0) return -1; // no PP

    moveSlot->pp--;

    // Log move use
    std::string attackerName = attackerIsPlayer ? attacker.nickname : "Enemy " + attacker.nickname;
    std::string defenderName = attackerIsPlayer ? "Enemy " + defender.nickname : defender.nickname;
    log(ctx, attackerName + " used " + move.name + "!");

    // Accuracy check
    if (move.accuracy > 0) {
        static std::mt19937 rng(std::random_device{}());
        int roll = std::uniform_int_distribution<>(1, 100)(rng);
        if (roll > move.accuracy) {
            log(ctx, "But it missed!");
            return 0;
        }
    }

    // Status moves
    if (move.power <= 0) {
        // Handle specific status moves by name (simplified)
        std::string lname = move.name;
        std::transform(lname.begin(), lname.end(), lname.begin(), ::tolower);

        StatusCondition inflict = StatusCondition::None;
        if (lname.find("poison") != std::string::npos || lname == "toxic") inflict = StatusCondition::Poison;
        else if (lname == "thunder wave" || lname == "stun spore") inflict = StatusCondition::Paralysis;
        else if (lname == "sleep powder" || lname == "hypnosis" || lname == "sing") inflict = StatusCondition::Sleep;
        // Note: moves like Ember/Flamethrower/Ice Beam have power > 0, handled below

        if (inflict != StatusCondition::None) {
            bool applied = tryInflictStatus(ctx, !attackerIsPlayer, inflict);
            if (applied) {
                log(ctx, defenderName + " was afflicted with " + std::string(statusName(inflict)) + "!");
            } else {
                log(ctx, "But it failed!");
            }
        }
        return 0;
    }

    // Damaging move
    DamageResult dmg = computeDamage(*attacker.species, *defender.species, move, attacker.level);

    if (dmg.isNoEffect) {
        log(ctx, "It doesn't affect " + defenderName + "...");
        return 0;
    }

    std::string effText;
    if (dmg.effectiveness > 1.0) effText = "It's super effective!";
    else if (dmg.effectiveness < 1.0 && dmg.effectiveness > 0.0) effText = "It's not very effective...";

    int finalDmg = static_cast<int>(std::round(dmg.damage));
    finalDmg = std::max(1, finalDmg); // minimum 1 damage

    int actual = applyDamage(ctx, !attackerIsPlayer, finalDmg);

    if (!effText.empty()) log(ctx, effText);
    log(ctx, defenderName + " took " + std::to_string(actual) + " damage!");

    // Secondary effects (simplified: 10% chance for common status)
    if (move.name == "Thunderbolt" || move.name == "Thunder") {
        static std::mt19937 rng(std::random_device{}());
        if (std::uniform_int_distribution<>(1, 100)(rng) <= 10) {
            if (tryInflictStatus(ctx, !attackerIsPlayer, StatusCondition::Paralysis)) {
                log(ctx, defenderName + " was paralyzed!");
            }
        }
    } else if (move.name == "Flamethrower" || move.name == "Fire Blast" || move.name == "Ember") {
        static std::mt19937 rng(std::random_device{}());
        if (std::uniform_int_distribution<>(1, 100)(rng) <= 10) {
            if (tryInflictStatus(ctx, !attackerIsPlayer, StatusCondition::Burn)) {
                log(ctx, defenderName + " was burned!");
            }
        }
    } else if (move.name == "Ice Beam" || move.name == "Blizzard") {
        static std::mt19937 rng(std::random_device{}());
        if (std::uniform_int_distribution<>(1, 100)(rng) <= 10) {
            if (tryInflictStatus(ctx, !attackerIsPlayer, StatusCondition::Freeze)) {
                log(ctx, defenderName + " was frozen!");
            }
        }
    } else if (move.name == "Sludge" || move.name == "Sludge Bomb" || move.name == "Poison Sting") {
        static std::mt19937 rng(std::random_device{}());
        if (std::uniform_int_distribution<>(1, 100)(rng) <= 30) {
            if (tryInflictStatus(ctx, !attackerIsPlayer, StatusCondition::Poison)) {
                log(ctx, defenderName + " was poisoned!");
            }
        }
    }

    return actual;
}

// Public executeMove - finds the move slot and executes
bool executeMove(BattleContext& ctx, bool isPlayer, int moveSlotIdx, const DataRepository& data) {
    BattleMon& attacker = isPlayer
        ? ctx.playerTeam[ctx.playerActiveIdx]
        : ctx.enemyTeam[ctx.enemyActiveIdx];
    BattleMon& defender = isPlayer
        ? ctx.enemyTeam[ctx.enemyActiveIdx]
        : ctx.playerTeam[ctx.playerActiveIdx];

    if (moveSlotIdx < 0 || moveSlotIdx >= static_cast<int>(attacker.moves.size())) return false;
    if (attacker.moves[moveSlotIdx].pp <= 0) return false;

    const Move* move = attacker.moves[moveSlotIdx].move;
    if (!move) return false;

    executeMoveInternal(ctx, attacker, defender, isPlayer, *move, data);
    return true;
}

// ---- Faint handling --------------------------------------------------------

void checkFaintsAndSwitch(BattleContext& ctx) {
    BattleMon& pMon = ctx.playerTeam[ctx.playerActiveIdx];
    BattleMon& eMon = ctx.enemyTeam[ctx.enemyActiveIdx];

    bool pFainted = pMon.isFainted();
    bool eFainted = eMon.isFainted();

    if (pFainted && eFainted) {
        log(ctx, pMon.nickname + " fainted!");
        log(ctx, "Enemy " + eMon.nickname + " fainted!");
        ctx.result = BattleResult::PlayerLost;
        ctx.phase = BattlePhase::BattleEnd;
        return;
    }

    if (eFainted) {
        log(ctx, "Enemy " + eMon.nickname + " fainted!");
        ctx.result = BattleResult::PlayerWon;
        ctx.phase = BattlePhase::BattleEnd;
        return;
    }

    if (pFainted) {
        log(ctx, pMon.nickname + " fainted!");
        ctx.result = BattleResult::PlayerLost;
        ctx.phase = BattlePhase::BattleEnd;
        return;
    }
}

// ---- AI --------------------------------------------------------------------

int pickEnemyMove(const BattleContext& ctx) {
    const BattleMon& eMon = ctx.enemyTeam[ctx.enemyActiveIdx];
    int bestIdx = -1;
    double bestScore = -1.0;

    for (int i = 0; i < static_cast<int>(eMon.moves.size()); ++i) {
        const auto& slot = eMon.moves[i];
        if (slot.pp <= 0) continue;
        if (!slot.move) continue;

        // Score by power (simple AI)
        double score = slot.move->power;
        if (slot.move->category == MoveCategory::Status) score = 10; // prefer status sometimes

        if (score > bestScore) {
            bestScore = score;
            bestIdx = i;
        }
    }
    return bestIdx;
}

// ---- Battle step -----------------------------------------------------------

bool battleStep(BattleContext& ctx, const DataRepository& data) {
    if (ctx.result != BattleResult::Ongoing) return false;

    switch (ctx.phase) {
        case BattlePhase::Setup: {
            ctx.phase = BattlePhase::TurnStart;
            break;
        }
        case BattlePhase::TurnStart: {
            ctx.turnNumber++;
            log(ctx, "--- Turn " + std::to_string(ctx.turnNumber) + " ---");
            tickStatus(ctx);
            checkFaintsAndSwitch(ctx);
            if (ctx.result != BattleResult::Ongoing) break;

            // Get actions
            ctx.playerAction = BattleContext::PendingAction{true, -1, false, -1};
            ctx.enemyAction = BattleContext::PendingAction{false, -1, false, -1};

            // For demo: auto-pick player move (prefer damaging moves) and enemy AI move
            BattleMon& pMon = ctx.playerTeam[ctx.playerActiveIdx];
            int bestDamagingMove = -1;
            int firstMove = -1;
            for (int i = 0; i < static_cast<int>(pMon.moves.size()); ++i) {
                if (pMon.moves[i].pp > 0) {
                    if (firstMove == -1) firstMove = i;
                    if (pMon.moves[i].move && pMon.moves[i].move->power > 0) {
                        bestDamagingMove = i;
                        break; // prefer first damaging move
                    }
                }
            }
            ctx.playerAction->moveSlot = (bestDamagingMove != -1) ? bestDamagingMove : firstMove;
            if (ctx.playerAction->moveSlot == -1) {
                ctx.playerAction->moveSlot = -1; // Struggle
            }

            ctx.enemyAction->moveSlot = pickEnemyMove(ctx);
            if (ctx.enemyAction->moveSlot == -1) {
                ctx.enemyAction->moveSlot = -1; // Struggle
            }

            ctx.phase = BattlePhase::ExecuteMove;
            break;
        }
        case BattlePhase::ExecuteMove: {
            // Build turn queue and execute in speed order
            auto queue = buildTurnQueue(ctx);

            for (const auto& entry : queue) {
                if (ctx.result != BattleResult::Ongoing) break;

                bool isPlayer = entry.isPlayer;
                BattleMon& attacker = isPlayer
                    ? ctx.playerTeam[ctx.playerActiveIdx]
                    : ctx.enemyTeam[ctx.enemyActiveIdx];

                // Check if this mon can still act (might have been statused by earlier move)
                if (!attacker.canAct()) continue;

                int moveSlot = isPlayer
                    ? (ctx.playerAction ? ctx.playerAction->moveSlot : -1)
                    : (ctx.enemyAction ? ctx.enemyAction->moveSlot : -1);

                if (moveSlot >= 0) {
                    executeMove(ctx, isPlayer, moveSlot, data);
                } else {
                    // Struggle: typeless 50 power physical, recoil 1/4 dmg
                    log(ctx, (isPlayer ? attacker.nickname : "Enemy " + attacker.nickname) + " has no moves left! Struggle!");
                    Move struggle;
                    struggle.name = "Struggle";
                    struggle.type = Type::Normal;
                    struggle.category = MoveCategory::Physical;
                    struggle.power = 50;
                    struggle.accuracy = 100;
                    executeMoveInternal(ctx, attacker,
                        isPlayer ? ctx.enemyTeam[ctx.enemyActiveIdx] : ctx.playerTeam[ctx.playerActiveIdx],
                        isPlayer, struggle, data);
                    // Recoil
                    int recoil = std::max(1, attacker.maxHp / 4);
                    attacker.currentHp = std::max(0, attacker.currentHp - recoil);
                    log(ctx, (isPlayer ? attacker.nickname : "Enemy " + attacker.nickname) + " took " + std::to_string(recoil) + " recoil damage!");
                }

                checkFaintsAndSwitch(ctx);
            }

            ctx.phase = BattlePhase::TurnStart;
            break;
        }
        case BattlePhase::FaintCheck: {
            checkFaintsAndSwitch(ctx);
            ctx.phase = BattlePhase::TurnStart;
            break;
        }
        case BattlePhase::BattleEnd: {
            if (ctx.result == BattleResult::PlayerWon) {
                log(ctx, "You won!");
            } else if (ctx.result == BattleResult::PlayerLost) {
                log(ctx, "You lost...");
            }
            return false;
        }
        case BattlePhase::InputWait:
        default:
            break;
    }

    return ctx.result == BattleResult::Ongoing;
}

}  // namespace pm

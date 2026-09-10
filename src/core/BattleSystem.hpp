#pragma once
#include "data/Monster.hpp"
#include "data/Move.hpp"
#include <vector>
#include <string>
#include <optional>

namespace pm {

// Forward declaration
class DataRepository;

// ---- Battle state machine --------------------------------------------------

enum class BattlePhase {
    Setup,          // initializing participants, sending out first mons
    TurnStart,      // new turn: speed tie-break, status tick
    InputWait,      // waiting for player move selection (AI for enemy)
    ExecuteMove,    // resolve move: damage, status, PP, messages
    FaintCheck,     // check for faints, handle switches/end
    BattleEnd,      // victory/defeat/forfeit
};

enum class BattleResult {
    Ongoing,
    PlayerWon,
    PlayerLost,
    Forfeit,
};

enum class StatusCondition : uint8_t {
    None = 0,
    Poison,
    Burn,
    Paralysis,
    Sleep,
    Freeze,
    Fainted,
};

// ---- Damage computation (existing) ----------------------------------------

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

// ---- Battle participants ---------------------------------------------------

// In-battle view of a monster (mutable HP, PP, status, stat stages).
struct BattleMon {
    const Monster* species = nullptr;  // template
    std::string nickname;
    int level = 1;
    int currentHp = 0;
    int maxHp = 0;
    Stats stats;        // calculated battle stats (with IV/EV/nature/level)
    StatusCondition status = StatusCondition::None;
    int sleepTurns = 0; // remaining sleep turns

    // Known moves with current/max PP.
    struct MoveSlot {
        const Move* move = nullptr;
        int pp = 0;
        int maxPp = 0;
    };
    std::vector<MoveSlot> moves;

    bool isFainted() const { return currentHp <= 0; }
    bool canAct() const;  // false if fainted, asleep, frozen, fully paralyzed
};

// ---- Turn queue ------------------------------------------------------------

struct TurnEntry {
    BattleMon* mon = nullptr;
    bool isPlayer = false;  // true = player's mon, false = enemy
    int speed = 0;          // effective speed (after paralysis, etc.)
};

// ---- Battle context (holds all mutable state) ------------------------------

struct BattleContext {
    std::vector<BattleMon> playerTeam;   // up to 6
    std::vector<BattleMon> enemyTeam;    // up to 6
    int playerActiveIdx = 0;             // index in playerTeam
    int enemyActiveIdx = 0;              // index in enemyTeam

    BattlePhase phase = BattlePhase::Setup;
    BattleResult result = BattleResult::Ongoing;
    int turnNumber = 0;

    // Pending move selections for this turn.
    struct PendingAction {
        bool isPlayer = false;
        int moveSlot = -1;        // 0-3, -1 = struggle/none
        bool isSwitch = false;
        int switchTarget = -1;    // index in team
    };
    std::optional<PendingAction> playerAction;
    std::optional<PendingAction> enemyAction;

    // Battle log for demo/UI.
    struct LogEntry { std::string text; bool isPlayer = true; };
    std::vector<LogEntry> log;
};

// ---- Public API ------------------------------------------------------------

// Initialize a battle from two Monster species (demo: 1v1 at level 50).
BattleContext createDemoBattle(const DataRepository& data,
                               int playerSpeciesNum, int enemySpeciesNum, int level = 50);

// Run one battle step (phase transition). Returns false when battle ends.
bool battleStep(BattleContext& ctx, const DataRepository& data);

// Execute a selected move for the active mon. Applies damage/status/PP.
// Returns true if move executed, false if invalid (no PP, etc.).
bool executeMove(BattleContext& ctx, bool isPlayer, int moveSlot, const DataRepository& data);

// Apply damage to a BattleMon, handling faint. Returns actual damage dealt.
int applyDamage(BattleContext& ctx, bool targetIsPlayer, int damage);

// Attempt to inflict a status condition. Returns true if applied.
bool tryInflictStatus(BattleContext& ctx, bool targetIsPlayer, StatusCondition status);

// Status tick at turn start (poison/burn damage, sleep countdown, etc.).
void tickStatus(BattleContext& ctx);

// Check for faints after a move; handle auto-switch or battle end.
void checkFaintsAndSwitch(BattleContext& ctx);

// AI: pick a move for enemy (simple: highest damage move with PP).
int pickEnemyMove(const BattleContext& ctx);

// Add a line to the battle log.
void log(BattleContext& ctx, std::string_view text, bool isPlayer = true);

// String helpers for status.
std::string_view statusName(StatusCondition s);

}  // namespace pm

# PIXELMON GAME DESIGN DOCUMENT v1.0
## Full-Featured Pokémon-Style RPG Implementation Plan
**Stack:** TypeScript + React + Vite + Zustand | **Target:** Web (Mobile/Desktop)
**For:** Agentic Workflow Implementation (DeepSeek V4 Flash / Nemotron 3 Ultra)

---

## 1. EXECUTIVE SUMMARY

**Pixelmon** is a full-featured, single-player, offline-first monster-catching RPG inspired by Pokémon Gold/Silver (Gen 2). Players explore the **Aurelia Region**, catch and train 150 unique pixel-art creatures, battle 8 Gym Leaders, defeat the Elite 4, and become Champion.

**Core Pillars:**
1. **Collection:** 150 creatures across 10 biomes with day/night variations
2. **Strategy:** Deep type-matchup system, 165 TMs, abilities, held items
3. **Exploration:** Non-linear world with HM-gated progression, hidden areas
4. **Progression:** 8 Gyms → Elite 4 → Champion → Post-Game (Battle Tower)

---

## 2. TECHNICAL ARCHITECTURE

### 2.1 Project Structure
```
pixelmon/
├── public/
│   ├── sprites/
│   │   ├── monsters/          # PM-001 to PM-150 (front/back/icon)
│   │   ├── overworld/         # Player NPCs, tiles
│   │   ├── items/             # Held items, TMs, key items
│   │   └── trainers/          # Gym leaders, elite 4 sprites
│   ├── audio/
│   │   ├── bgm/               # Route themes, battle music
│   │   └── sfx/               # cries, hits, UI sounds
│   └── data/                  # JSON databases (monsters, moves, maps)
├── src/
│   ├── core/                  # Game engine
│   │   ├── engine/            # Game loop, state machine
│   │   ├── battle/            # Battle system logic
│   │   ├── overworld/         # Map rendering, collision
│   │   └── ai/                # Trainer AI, wild encounter AI
│   ├── components/            # React UI components
│   │   ├── battle/            # Battle UI (HP bars, menus)
│   │   ├── overworld/         # Game viewport, dialogue boxes
│   │   ├── menu/              # Party, bag, Pokedex, save
│   │   └── ui/                # Reusable primitives
│   ├── data/                  # TypeScript data definitions
│   │   ├── monsters.ts        # Generated from PIXELMON_FACTORY
│   │   ├── moves.ts           # All 165 TMs + unique moves
│   │   ├── items.ts           # Consumables, TMs, key items
│   │   └── maps.ts            # Map definitions
│   ├── systems/               # Game logic
│   │   ├── party.ts           # Party management
│   │   ├── storage.ts         # PC box system
│   │   ├── breeding.ts        # Day care mechanics
│   │   └── trading.ts         # Link-cable emulation (localStorage)
│   ├── utils/                 # Math, RNG, helpers
│   └── types/                 # Shared TypeScript interfaces
├── docs/
│   ├── PIXELMON_FACTORY.md    # Monster generation spec
│   └── GDD.md                 # This file
└── tests/                     # Unit tests for battle math
```

### 2.2 State Management (Zustand)
```typescript
// Global game state
interface GameState {
  // Player
  player: {
    name: string;
    id: string;                    // UUID for save files
    position: { x: number; y: number; mapId: string };
    direction: 'up' | 'down' | 'left' | 'right';
    badges: Badge[];               // 8 gym badges
    money: number;
    playTime: number;              // Seconds
  };

  // Progression
  story: {
    flags: Record<string, boolean>; // "defeated_brock", "got_bike", etc.
    gymBadges: number;             // 0-8
    eliteFourDefeated: boolean;
    championDefeated: boolean;
  };

  // Creatures
  party: MonsterInstance[];        // Max 6
  storage: MonsterInstance[][];    // PC boxes (30 per box, 20 boxes)
  daycare: MonsterInstance[];      // Max 2

  // Inventory
  bag: {
    items: Record<ItemID, number>;       // Stack counts
    tms: TMNumber[];
    keyItems: KeyItemID[];
  };

  // World
  world: {
    time: GameTime;                // Day/night cycle (real-time or accelerated)
    weather: WeatherType;
    mapId: string;
    encounters: number;            // Steps since last encounter
  };
}

// Monster instance (individual creature)
interface MonsterInstance {
  uid: string;                     // Unique instance ID
  monsterId: MonsterID;            // PM-001, etc.
  nickname?: string;
  level: number;
  exp: number;
  stats: Stats;                    // Calculated from base + IVs + EVs
  ivs: Stats;                      // 0-31 per stat
  evs: Stats;                      // 0-255 per stat, 510 total
  nature: Nature;
  ability: AbilityID;
  moves: MoveInstance[];           // Max 4
  hp: number;                      // Current HP
  status: StatusCondition | null;
  friendship: number;              // 0-255
  met: {
    location: string;
    level: number;
    date: string;                  // ISO date
  };
  pokerus: boolean;
  shiny: boolean;                  // 1/4096 chance
}
```

### 2.3 Rendering Strategy
- **Overworld:** HTML5 Canvas with tile-based rendering (32×32px tiles)
- **Battle:** React components with CSS animations (HP bars, shake effects)
- **UI:** DOM-based menus (crisper text rendering)
- **Performance:** Object pooling for sprites, requestAnimationFrame game loop

---

## 3. CORE SYSTEMS

### 3.1 Battle System

#### 3.1.1 Battle Structure
```typescript
interface BattleState {
  type: 'wild' | 'trainer' | 'gym' | 'elite4' | 'champion';
  weather: Weather;
  terrain: Terrain;
  turn: number;
  activePlayer: ActiveMonster;
  activeEnemy: ActiveMonster;
  queue: BattleAction[];           // Priority queue for turn order
}

interface ActiveMonster {
  instance: MonsterInstance;
  statStages: StatStages;          // -6 to +6 per stat
  volatileStatus: VolatileStatus[]; // Confusion, flinch, etc.
  substituted: boolean;
  trapped: boolean;
}
```

#### 3.1.2 Turn Order Algorithm
```pseudocode
FUNCTION calculateTurnOrder(actions: BattleAction[]):
  FOR EACH action IN actions:
    priority = action.move.priority
    IF action.type == "switch" THEN priority = 6
    IF action.type == "item" THEN priority = 6
    IF action.type == "run" THEN priority = -7

    speed = action.user.effectiveSpeed
    IF action.user.ability == "Quick Feet" AND status != null:
      speed *= 1.5

    action.priorityValue = (priority * 1000) + speed

  RETURN SORT(actions, DESC by priorityValue, tie-break by random)
```

#### 3.1.3 Damage Formula (Gen 5+)
```pseudocode
FUNCTION calculateDamage(attacker, defender, move, battleState):
  // Base calculation
  level = attacker.level
  power = move.power
  attackStat = IF move.category == "Physical" THEN attacker.attack ELSE attacker.spAttack
  defenseStat = IF move.category == "Physical" THEN defender.defense ELSE defender.spDefense

  base = FLOOR( FLOOR( (2 * level / 5 + 2) * power * attackStat / defenseStat ) / 50 ) + 2

  // Modifiers
  STAB = IF move.type IN attacker.types THEN 1.5 ELSE 1.0
  effectiveness = TYPE_CHART[move.type][defender.types[0]] * 
                  IF defender.types[1] THEN TYPE_CHART[move.type][defender.types[1]] ELSE 1
  random = RANDOM(0.85, 1.0)
  critical = IF RANDOM() < critChance THEN 1.5 ELSE 1.0

  // Weather
  weatherMod = 1.0
  IF battleState.weather == "Rain" AND move.type == "Water" THEN weatherMod = 1.5
  IF battleState.weather == "Rain" AND move.type == "Fire" THEN weatherMod = 0.5
  IF battleState.weather == "Sun" THEN INVERSE

  // Burn
  burnMod = IF attacker.status == "Burn" AND move.category == "Physical" THEN 0.5 ELSE 1.0

  damage = FLOOR(base * STAB * effectiveness * random * critical * weatherMod * burnMod)
  RETURN MAX(1, damage)
```

#### 3.1.4 Type Effectiveness Chart (18×18)
Implement as 2D lookup table. Key matchups:
- **Super Effective (2×):** Fire→Grass, Water→Fire, Electric→Water, Grass→Water
- **Immune (0×):** Electric→Ground, Ghost→Normal, Normal→Ghost, Ground→Flying
- **Resist (0.5×):** Fire→Fire, Water→Water, etc.

#### 3.1.5 Status Conditions
```typescript
type StatusCondition = 
  | 'Burn'      // 1/16 HP per turn, 50% attack reduction (physical)
  | 'Poison'    // 1/8 HP per turn
  | 'BadlyPoisoned' // 1/16, 2/16, 3/16... (toxic)
  | 'Paralysis' // 25% chance to not move, speed * 0.5
  | 'Sleep'     // 1-3 turns, cannot move
  | 'Freeze'    // 20% chance to thaw each turn, cannot move
  | 'Confusion' // 50% chance to hit self, 1-4 turns
  | 'Flinch'    // Cannot move this turn
  | 'Trap'      // Cannot switch, 1/16 HP per turn
  | 'LeechSeed' // 1/8 HP drained to opponent
  | 'Curse';    // 1/4 HP per turn, +25% attack/defense
```

### 3.2 Capture System

#### 3.2.1 Catch Rate Formula (Gen 3-5)
```pseudocode
FUNCTION calculateCatchRate(wildMonster, ballType, battleState):
  // Status bonus
  statusMod = 1.0
  IF status IN [Sleep, Freeze] THEN statusMod = 2.0
  IF status IN [Paralysis, Poison, Burn, BadlyPoisoned] THEN statusMod = 1.5

  // HP factor (lower HP = easier)
  hpFactor = (3 * maxHP - 2 * currentHP) / (3 * maxHP)

  // Ball modifiers
  ballMod = BALL_MODIFIERS[ballType]  // Poke=1, Great=1.5, Ultra=2, Master=255

  // Formula
  a = ( (3 * maxHP - 2 * currentHP) * catchRate * ballMod * statusMod ) / (3 * maxHP)

  IF a >= 255 THEN return CAUGHT
  b = 1048560 / SQRT( SQRT( 16711680 / a ) )

  // Shake checks (4 shakes)
  shakes = 0
  FOR i = 1 to 4:
    IF RANDOM(0, 65535) < b THEN shakes++

  IF shakes == 4 THEN return CAUGHT
  ELSE return shakes  // 0-3 shakes then break
```

### 3.3 Experience & Leveling

#### 3.3.1 XP Yield Formula
```pseudocode
FUNCTION calculateXPYield(defeatedMonster, winner, participants, isTrainerBattle):
  base = defeatedMonster.baseExp
  level = defeatedMonster.level

  // Base formula
  exp = (base * level) / 5

  // Trainer bonus
  IF isTrainerBattle THEN exp *= 1.5

  // Lucky Egg
  IF winner.heldItem == "LuckyEgg" THEN exp *= 1.5

  // Trade bonus
  IF winner.traded THEN exp *= 1.5

  // Exp Share (Gen 6+ style: full to participants, 50% to non-participants)
  IF winner.participated THEN return FLOOR(exp / participants)
  ELSE return FLOOR(exp / 2)
```

#### 3.3.2 Growth Rate Formulas
```typescript
// Experience needed for level n
const GROWTH_RATES = {
  Erratic: (n) => n <= 50 ? (100*n**3)/50 : n <= 68 ? (100*n**3)/100 : n <= 98 ? (100*n**3)/500 : (100*n**3)/1000,
  Fast: (n) => (4*n**3)/5,
  MediumFast: (n) => n**3,
  MediumSlow: (n) => (6/5)*n**3 - 15*n**2 + 100*n - 140,
  Slow: (n) => (5*n**3)/4,
  Fluctuating: (n) => n <= 15 ? n**3 * ((24 + FLOOR((n+1)/3))/50) : n <= 36 ? n**3 * ((14+n)/50) : n**3 * ((32+FLOOR(n/2))/50)
};
```

### 3.4 Stat Calculation (Gen 3+)
```pseudocode
FUNCTION calculateStats(monster):
  // HP
  hp = FLOOR( (2 * base.hp + ivs.hp + FLOOR(evs.hp / 4)) * level / 100 ) + level + 10

  // Other stats
  FOR stat IN [attack, defense, spAttack, spDefense, speed]:
    base = FLOOR( (2 * base[stat] + ivs[stat] + FLOOR(evs[stat] / 4)) * level / 100 ) + 5
    natureMod = NATURE_MODIFIERS[monster.nature][stat]  // 1.1, 0.9, or 1.0
    stats[stat] = FLOOR(base * natureMod)

  RETURN stats
```

### 3.5 Evolution System
```pseudocode
FUNCTION checkEvolution(monster):
  FOR evo IN monster.evolution:
    SWITCH evo.method:
      CASE "level":
        IF monster.level >= evo.requirement: return evo.to
      CASE "stone":
        IF itemUsed == evo.requirement: return evo.to
      CASE "trade":
        IF traded: return evo.to
      CASE "friendship":
        IF monster.friendship >= evo.requirement: return evo.to
      CASE "time":
        IF monster.level >= evo.requirement AND timeOfDay == evo.condition: return evo.to
      CASE "move":
        IF monster.knowsMove(evo.requirement): return evo.to
  RETURN null
```

---

## 4. WORLD DESIGN

### 4.1 Aurelia Region Map

**Geography:** Island region with central mountain, coastal cities, northern tundra.

```
[Map Layout - Simplified]
┌─────────────────────────────────────┐
│  TUNDRA CITY (Ice Gym)              │
│  [PM-140s]                          │
├──────────┬──────────────────────────┤
│ MT. FROST│ ROUTE 9                 │
│ [PM-130s]│ [PM-120s]               │
├──────────┴──────────┬───────────────┤
│                     │ CAPE CITY     │
│   CENTRAL PEAK      │ (Water Gym)   │
│   [PM-147-149]      │ [PM-110s]     │
│                     ├───────────────┤
│  DESERT TOWN        │ ROUTE 8       │
│  (Ground Gym)       │ [PM-100s]     │
│  [PM-090s]          │               │
├──────────┬──────────┴───────────────┤
│ ROUTE 7  │ METROPOLIS               │
│ [PM-080s]│ (Elite 4 Location)       │
├──────────┴──────────┬───────────────┤
│ FOREST CITY         │ PORT TOWN     │
│ (Grass Gym)         │ (Flying Gym)  │
│ [PM-070s]           │ [PM-060s]     │
├──────────┬──────────┴───────────────┤
│ ROUTE 4  │ MINING TOWN              │
│ [PM-050s]│ (Rock Gym)               │
│          │ [PM-050s]                │
├──────────┴──────────┬───────────────┤
│ START TOWN          │ EMBER CITY    │
│ (Player Start)      │ (Fire Gym)    │
│ [PM-001-020]        │ [PM-040s]     │
└─────────────────────┴───────────────┘
```

### 4.2 City/Town Specifications

**START TOWN (Pallet Town analog)**
- **Population:** 10 NPCs
- **Buildings:** Player House, Rival House, Prof. Lab, 2 generic houses
- **Wild:** None (safe zone)
- **Special:** Starting event, first partner selection

**METROPOLIS (Saffron City analog)**
- **Population:** 50+ NPCs
- **Buildings:** Gym (Elite 4 HQ), Department Store, Game Corner, Hotel, 10 houses
- **Special:** Endgame content, Battle Tower access

### 4.3 Route Design Template
```yaml
route:
  id: ROUTE-01
  name: "Pallet Path"
  connections: [START_TOWN, FOREST_CITY]
  size: {width: 30, height: 20}  # Tiles
  terrain: grass
  encounter_rate: 10%  # Per step in tall grass
  wild_monsters:
    - id: PM-010  # Pidove
      level_range: [2, 5]
      weight: 40
    - id: PM-013  # Rattata
      level_range: [2, 4]
      weight: 30
    - id: PM-017  # Pichu
      level_range: [3, 5]
      weight: 20
    - id: PM-020  # Caterpie
      level_range: [3, 5]
      weight: 10
  trainers:
    - id: TR-001
      class: Youngster
      party: [PM-013 Lv5]
  items:
    - {position: [5, 10], item: POTION}
    - {position: [15, 15], item: POKEBALL, hidden: true}
```

### 4.4 Tilemap Specifications
- **Tile Size:** 32×32 pixels
- **Layers:** 
  1. Ground (grass, path, water)
  2. Decoration (flowers, rocks, trees)
  3. Collision (invisible)
  4. Overlay (bridges, roofs)
- **Autotiling:** 4-bit autotile system for natural terrain edges

---

## 5. PROGRESSION & STORY

### 5.1 Main Story Beats
```yaml
act_1: # Beginnings
  - Get starter from Professor
  - Battle rival
  - Catch first wild monster
  - Reach Forest City, defeat Grass Gym (Badge 1)

act_2: # Expansion
  - Traverse forest, reach Mining Town
  - Defeat Rock Gym (Badge 2)
  - Team Rocket equivalent introduced (Team Eclipse)
  - Defeat Fire Gym (Badge 3)

act_3: # Conflict
  - Team Eclipse steals legendary artifact
  - Chase through desert, defeat Ground Gym (Badge 4)
  - Confront Team Eclipse at Mt. Frost
  - Defeat Ice Gym (Badge 5)

act_4: # Rising
  - Coastal journey, defeat Water Gym (Badge 6)
  - Sky tower, defeat Flying Gym (Badge 7)
  - Final Gym in Metropolis (Badge 8)

act_5: # Climax
  - Elite 4 Challenge (Dark, Psychic, Steel, Dragon)
  - Champion battle
  - Hall of Fame
  - Post-game: Battle Tower, Legendary hunt
```

### 5.2 Gym Leader Roster
| Order | City | Type | Leader | Signature | Level Range |
|-------|------|------|--------|-----------|-------------|
| 1 | Forest City | Grass | Lief | PM-027 (Breloom) | 12-14 |
| 2 | Mining Town | Rock | Brock | PM-081 (Armaldo) | 18-21 |
| 3 | Ember City | Fire | Blaine | PM-006 (Skeledirge) | 24-28 |
| 4 | Desert Town | Ground | Giovanni | PM-XXX (Garchomp) | 30-34 |
| 5 | Tundra City | Ice | Pryce | PM-XXX (Mamoswine) | 36-40 |
| 6 | Cape City | Water | Misty | PM-XXX (Gyarados) | 42-46 |
| 7 | Port Town | Flying | Skyla | PM-XXX (Skarmory) | 48-52 |
| 8 | Metropolis | Mixed | Blue | Mixed team | 55-60 |

### 5.3 Elite 4 & Champion
| Position | Type | Trainer | Ace |
|----------|------|---------|-----|
| Elite 1 | Dark | Karen | PM-XXX (Hydreigon) Lv62 |
| Elite 2 | Psychic | Will | PM-XXX (Metagross) Lv63 |
| Elite 3 | Steel | Bruno | PM-XXX (Lucario) Lv64 |
| Elite 4 | Dragon | Lance | PM-079 (Salamence) Lv65 |
| Champion | Mixed | Red | PM-150 (Lugia) Lv70 |

---

## 6. ITEMS & EQUIPMENT

### 6.1 Item Categories
```typescript
type ItemCategory = 
  | 'PokeBall'      // Capture devices
  | 'Healing'       // Potions, status heals
  | 'Battle'        // X-Attack, Dire Hit
  | 'TM'            // Technical Machines
  | 'KeyItem'       // Bike, Silph Scope
  | 'HeldItem'      // Choice Band, Leftovers
  | 'Evolution'     // Fire Stone, etc.
  | 'Berry';        // Oran, Sitrus, etc.

interface Item {
  id: ItemID;
  name: string;
  category: ItemCategory;
  price: number;        // 0 if unsellable
  description: string;
  effect: ItemEffect;   // Pseudocode
  sprite: string;       // 16x16 icon
}
```

### 6.2 Key Items
- **Bicycle:** 2× overworld speed
- **Itemfinder:** Reveals hidden items
- **Silph Scope:** Reveals ghosts in tower
- **Super Rod:** Fish in any water
- **Town Map:** View region map
- **VS Seeker:** Rebattle trainers

### 6.3 Held Items (Competitive)
- **Choice Band:** +50% Attack, locked into one move
- **Choice Scarf:** +50% Speed, locked
- **Leftovers:** 1/16 HP per turn
- **Life Orb:** +30% damage, 10% recoil
- **Focus Sash:** Survive at 1 HP if full HP
- **Eviolite:** +50% Def/SpD if not fully evolved

---

## 7. ADVANCED SYSTEMS

### 7.1 Breeding (Day Care)
```pseudocode
FUNCTION breed(parent1, parent2):
  IF NOT compatible(parent1, parent2): return null
  IF parent1.gender == parent2.gender: return null

  baby = new MonsterInstance()
  baby.species = babySpecies(parent1, parent2)  // Lowest evolution of mother

  // Inherit moves
  baby.moves = inheritEggMoves(parent1, parent2)

  // Inherit IVs (3 random from parents)
  FOR i = 1 to 3:
    stat = RANDOM_STAT()
    baby.ivs[stat] = RANDOM_CHOICE([parent1.ivs[stat], parent2.ivs[stat]])

  // Inherit nature (50% if Everstone)
  IF parent1.heldItem == "Everstone" AND RANDOM() < 0.5:
    baby.nature = parent1.nature

  // Ability (80% mother, 20% hidden)
  baby.ability = RANDOM() < 0.8 ? mother.ability : mother.hiddenAbility

  RETURN baby
```

### 7.2 Day/Night System
- **Real-time:** Uses system clock (configurable for testing)
- **Spawns:** Some monsters only appear at night (e.g., Ghost types)
- **Evolutions:** Eevee → Espeon (day) / Umbreon (night)
- **Events:** Specific NPCs only appear at certain times

### 7.3 Weather System
- **Routes:** Some have permanent weather (desert = sandstorm)
- **Battle:** Weather affects damage, accuracy, abilities
- **Forecast:** Weather changes every in-game day

---

## 8. USER INTERFACE

### 8.1 Screen Layouts

**Overworld:**
```
┌─────────────────────────────┐
│  [Viewport - 15×10 tiles]   │
│                             │
│                             │
├─────────────────────────────┤
│ [Dialogue Box when active]  │
└─────────────────────────────┘
```

**Battle:**
```
┌─────────────────────────────┐
│  Enemy Sprite      [HP Bar] │
│  Enemy Name Lv.X            │
│                             │
│  [Weather/Field]            │
│                             │
│  Player Sprite     [HP Bar] │
│  Player Name Lv.X           │
│                             │
│ ┌─────────────────────────┐ │
│ │ FIGHT | BAG | RUN | SWAP│ │
│ └─────────────────────────┘ │
└─────────────────────────────┘
```

### 8.2 Menu System
- **Start Menu:** Party, Bag, Save, Options, Pokedex
- **Party Screen:** 6 slots, summary, switch, item use
- **Bag:** Categorized tabs, use/give/toss
- **Pokedex:** Search by type, number, name; seen/caught status

---

## 9. SAVE SYSTEM

### 9.1 Save Structure (localStorage/IndexedDB)
```typescript
interface SaveFile {
  version: string;           // "1.0.0"
  timestamp: number;         // Unix timestamp
  checksum: string;          // MD5 for validation

  player: PlayerData;
  party: MonsterInstance[];
  storage: MonsterInstance[][];
  bag: BagData;
  story: StoryFlags;
  world: WorldState;
  records: {
    hallOfFame: HallOfFameEntry[];
    linkBattles: {wins: number, losses: number};
  };
}

// Save slots: 3 available
// Autosave: Every map transition, gym battle victory
```

### 9.2 Backup & Recovery
- **Export:** Download save as JSON file
- **Import:** Upload to restore
- **Cloud:** Optional Firebase sync (future feature)

---

## 10. AUDIO DESIGN

### 10.1 Music (Chiptune/8-bit)
- **Overworld:** Different theme per biome
- **Battle:** Wild, Trainer, Gym, Elite 4, Champion themes
- **Town:** City, town, dungeon themes
- **Generation:** Use BeepBox or similar for authentic GB sound

### 10.2 Sound Effects
- **Monster Cries:** 8-bit synthesized (unique per monster, generated from ID hash)
- **Moves:** Hit, miss, super effective, status
- **UI:** Menu navigation, selection, error

---

## 11. DEVELOPMENT ROADMAP

### Phase 1: Foundation (Week 1-2)
- [ ] Project setup (Vite, TypeScript, Zustand)
- [ ] Core data structures (Monster, Move, Item types)
- [ ] Battle engine prototype (damage calc, turn order)
- [ ] Basic UI shell

### Phase 2: Battle System (Week 3-4)
- [ ] Full battle state machine
- [ ] All 18 types implemented
- [ ] Status conditions
- [ ] AI opponent logic
- [ ] Battle animations

### Phase 3: Overworld (Week 5-6)
- [ ] Tilemap renderer
- [ ] Player movement (4-directional, collision)
- [ ] Map transitions
- [ ] NPC interaction
- [ ] Wild encounters

### Phase 4: Progression (Week 7-8)
- [ ] Party management
- [ ] Experience/leveling
- [ ] Evolution system
- [ ] Gym battles (8 leaders)
- [ ] Badge checks (HM gating)

### Phase 5: Content (Week 9-10)
- [ ] All 150 monsters implemented
- [ ] All moves/TMs
- [ ] Story events
- [ ] Elite 4 + Champion
- [ ] Post-game content

### Phase 6: Polish (Week 11-12)
- [ ] Audio implementation
- [ ] Save/load system
- [ ] UI polish
- [ ] Balance testing
- [ ] Performance optimization
- [ ] Mobile touch controls

---

## 12. GIT WORKFLOW FOR AGENTIC CODING

### 12.1 Branch Strategy
```
main                    # Production-ready, protected
├── develop             # Integration branch
├── feat/monster/PM-001-sprigatito
├── feat/system/battle-engine
├── feat/map/route-01
├── fix/battle-critical-hit
├── docs/update-gdd
└── asset/sprites-batch-01
```

### 12.2 Commit Convention (Conventional Commits)
```
feat(monster): add PM-001 Sprigatito with evolution line
feat(battle): implement type effectiveness chart
fix(save): resolve localStorage quota exceeded error
docs(gdd): update gym leader levels
asset(sprites): generate PM-010 to PM-020 front sprites
refactor(engine): extract damage calculation to pure function
test(battle): add unit tests for STAB calculation
```

### 12.3 Agentic Coding Protocol
```yaml
agent_instructions:
  before_starting:
    - Read PIXELMON_FACTORY.md and GDD.md
    - Check current branch: MUST be feature branch
    - Run: npm test (must pass)

  during_implementation:
    - One monster per commit
    - One system per branch
    - Follow TypeScript interfaces exactly
    - Add JSDoc comments for all public functions

  before_commit:
    - Run: npm run lint
    - Run: npm run typecheck
    - Run: npm test
    - Update CHANGELOG.md

  commit_message_format: |
    type(scope): description

    [optional body]

    🤖 Generated with [Agent Name]
    Co-authored-by: [Agent] <agent@pixelmon.dev>

  pull_request:
    title: "[PM-XXX] Monster Name Implementation"
    body: |
      ## Changes
      - Added PM-XXX [Name]
      - Base stats: [BST]
      - Evolution: [Method]

      ## Checklist
      - [ ] Sprite generated
      - [ ] Data validated
      - [ ] Tests passing
```

### 12.4 File Ownership (CODEOWNERS)
```
# Auto-assign based on path
/src/data/monsters/PM-0*.ts    @agent-monster-gen
/src/data/moves/               @agent-systems
/src/core/battle/              @agent-battle
/public/sprites/               @agent-assets
/docs/                         @agent-docs
```

---

## 13. TESTING STRATEGY

### 13.1 Unit Tests (Vitest)
```typescript
describe('Battle System', () => {
  it('calculates STAB correctly', () => {
    const charizard = createMonster('PM-006', 50); // Fire type
    const damage = calculateDamage(charizard, target, fireMove);
    expect(damage).toBeGreaterThan(nonStabDamage);
  });

  it('handles type immunity', () => {
    // Electric vs Ground should deal 0
    expect(effectiveness('Electric', 'Ground')).toBe(0);
  });
});
```

### 13.2 Integration Tests
- Full battle simulation (100 turns, no crashes)
- Save/load cycle (data integrity)
- Evolution chain (level up → evolve → learn moves)

### 13.3 Balance Testing
- Automated: Simulate 1000 battles, check win rates
- Manual: Playtest gym battles at intended levels

---

## 14. PERFORMANCE TARGETS

| Metric | Target |
|--------|--------|
| Initial Load | < 3s on 3G |
| Battle Start | < 500ms |
| Map Transition | < 200ms |
| Frame Rate | 60 FPS (desktop), 30 FPS (mobile) |
| Memory | < 500MB |
| Save Size | < 5MB per save |

---

## 15. FUTURE ROADMAP (Post-Launch)

- **v1.1:** Online trading (WebRTC)
- **v1.2:** PvP battles (ranked ladder)
- **v1.3:** New region (Johto-style expansion)
- **v2.0:** User-generated content (custom monsters)
- **v2.1:** Mobile app (React Native port)
- **v3.0:** MMO features (shared world zones)

---

**END OF GAME DESIGN DOCUMENT**

# PIXELMON FACTORY v1.0
## Creature Generation & Data Schema Specification
**Target:** 150 Unique Monsters | 5 Evolution Stages | 18 Types
**For:** Agentic Workflow Implementation (DeepSeek V4 Flash / Nemotron 3 Ultra Compatible)

---

## 1. DATA ARCHITECTURE

### 1.1 Core Type Definitions (TypeScript)

```typescript
// PSEUDOCODE CONTRACT: All fields must be implemented exactly as specified
// VALIDATION: Zod schema enforcement at build time

type MonsterID = string; // Format: "PM-001" to "PM-150"
type MonsterName = string; // Max 12 chars, TitleCase
type Type = 
  | 'Normal' | 'Fire' | 'Water' | 'Electric' | 'Grass'
  | 'Ice' | 'Fighting' | 'Poison' | 'Ground' | 'Flying'
  | 'Psychic' | 'Bug' | 'Rock' | 'Ghost' | 'Dragon'
  | 'Dark' | 'Steel' | 'Fairy';

type EggGroup = 
  | 'Monster' | 'Water1' | 'Water2' | 'Water3' | 'Bug'
  | 'Flying' | 'Field' | 'Fairy' | 'Grass' | 'HumanLike'
  | 'Mineral' | 'Amorphous' | 'Dragon' | 'Undiscovered';

type GrowthRate = 'Erratic' | 'Fast' 'MediumFast' | 'MediumSlow' | 'Slow' | 'Fluctuating';

interface Stats {
  hp: number;        // Base 1-255
  attack: number;    // Base 5-190
  defense: number;   // Base 5-230
  spAttack: number;  // Base 10-194
  spDefense: number; // Base 20-230
  speed: number;     // Base 5-180
}

interface Evolution {
  to: MonsterID;
  method: 'level' | 'stone' | 'trade' | 'friendship' | 'time' | 'location' | 'move';
  requirement: number | string; // Level number, ItemID, or MoveID
  condition?: 'day' | 'night' | 'rain' | 'snow'; // For time/location evos
}

interface Monster {
  id: MonsterID;
  name: MonsterName;
  types: [Type] | [Type, Type]; // Primary, Secondary
  baseStats: Stats;
  evYield: Partial<Stats>; // EVs granted on defeat (total 1-3)
  abilities: AbilityID[]; // 1-3 abilities, first is primary
  hiddenAbility?: AbilityID;
  height: number; // Meters, 0.1-20.0
  weight: number; // Kilograms, 0.1-999.9
  genderRatio: number; // Female % (0-100, -1 = genderless)
  eggGroups: EggGroup[]; // 1-2 groups
  hatchSteps: number; // 256-10240
  growthRate: GrowthRate;
  catchRate: number; // 3-255 (lower = harder)
  baseFriendship: number; // 0-255, default 70
  baseExp: number; // 36-255, determines XP yield
  learnset: LevelUpMove[];
  tmMoves: TMNumber[]; // 0-100
  eggMoves: MoveID[];
  evolution?: Evolution[];
  pokedex: {
    category: string; // "Mouse Pokémon", "Flame Pokémon"
    entry: string; // 2-3 sentence flavor text
  };
  sprites: {
    front: SpriteRef; // 32x32px
    back: SpriteRef;  // 32x32px
    icon: SpriteRef;  // 16x16px
    overworld: SpriteRef; // 16x16px, 4-directional
  };
}

interface LevelUpMove {
  level: number; // 1-100, or 0 for evolution-only
  move: MoveID;
}

// SPRITE GENERATION CONTRACT
interface SpriteRef {
  prompt: string; // AI generation prompt (see Section 4)
  palette: string[]; // 4 hex colors max
  style: 'gen2-styled'; // 32x32, 4-color, black outline
}
```

### 1.2 Move Schema

```typescript
interface Move {
  id: MoveID;
  name: string;
  type: Type;
  category: 'Physical' | 'Special' | 'Status';
  power: number; // 0 for status moves
  accuracy: number; // 1-100, or 0 for always-hit
  pp: number; // 5-40
  priority: number; // -7 to +7
  target: 'self' | 'enemy' | 'all' | 'ally' | 'random';
  effect?: MoveEffect; // Secondary effects (burn, paralyze, etc.)
  description: string;
}

// EXAMPLE MOVES (Implement all 165 TMs + 50 unique moves)
const EXAMPLE_MOVES: Move[] = [
  {
    id: 'MV-TACKLE',
    name: 'Tackle',
    type: 'Normal',
    category: 'Physical',
    power: 40,
    accuracy: 100,
    pp: 35,
    priority: 0,
    target: 'enemy',
    description: 'A full-body charge attack.'
  },
  {
    id: 'MV-EMBER',
    name: 'Ember',
    type: 'Fire',
    category: 'Special',
    power: 40,
    accuracy: 100,
    pp: 25,
    priority: 0,
    target: 'enemy',
    effect: { status: 'burn', chance: 0.10 },
    description: 'An attack that may inflict a burn.'
  }
];
```

### 1.3 Ability Schema

```typescript
interface Ability {
  id: AbilityID;
  name: string;
  description: string;
  effect: AbilityEffect; // Pseudocode trigger
}

// EXAMPLE ABILITIES
const ABILITIES: Ability[] = [
  { id: 'AB-OVERGROW', name: 'Overgrow', description: 'Powers up Grass moves in a pinch.', effect: 'IF hp < 1/3 THEN grass_moves *= 1.5' },
  { id: 'AB-BLAZE', name: 'Blaze', description: 'Powers up Fire moves in a pinch.', effect: 'IF hp < 1/3 THEN fire_moves *= 1.5' },
  { id: 'AB-TORRENT', name: 'Torrent', description: 'Powers up Water moves in a pinch.', effect: 'IF hp < 1/3 THEN water_moves *= 1.5' },
  { id: 'AB-SWARM', name: 'Swarm', description: 'Powers up Bug moves in a pinch.', effect: 'IF hp < 1/3 THEN bug_moves *= 1.5' },
  { id: 'AB-INTIMIDATE', name: 'Intimidate', description: 'Lowers the foe's Attack.', effect: 'ON_SWITCH_IN: enemy.attack *= 0.67' },
  { id: 'AB-LEVITATE', name: 'Levitate', description: 'Gives immunity to Ground moves.', effect: 'IMMUNE_TO: ground' }
];
```

---

## 2. THE 150 MONSTER GENERATION MATRIX

### 2.1 Ecological Distribution Strategy
To ensure 150 unique, balanced creatures, we use a **10×15 Matrix**:
- **10 Biome Archetypes** (Columns)
- **15 Progression Tiers** (Rows: 3 per evolution stage)

| Biome | Tier 1 (1-10) | Tier 2 (11-20) | Tier 3 (21-30) | Tier 4 (31-40) | Tier 5 (41-50) |
|-------|---------------|----------------|----------------|----------------|----------------|
| **Grassland** | PM-001 | PM-011 | PM-021 | PM-031 | PM-041 |
| **Forest** | PM-002 | PM-012 | PM-022 | PM-032 | PM-042 |
| **Mountain** | PM-003 | PM-013 | PM-023 | PM-033 | PM-043 |
| **Ocean** | PM-004 | PM-014 | PM-024 | PM-034 | PM-044 |
| **Cave** | PM-005 | PM-015 | PM-025 | PM-035 | PM-045 |
| **Urban** | PM-006 | PM-016 | PM-026 | PM-036 | PM-046 |
| **Desert** | PM-007 | PM-017 | PM-027 | PM-037 | PM-047 |
| **Tundra** | PM-008 | PM-018 | PM-028 | PM-038 | PM-048 |
| **Swamp** | PM-009 | PM-019 | PM-029 | PM-039 | PM-049 |
| **Volcanic** | PM-010 | PM-020 | PM-030 | PM-040 | PM-050 |

*Continue pattern for PM-051 through PM-150 (Rows 6-15)*

### 2.2 Starter Trios (PM-001 to PM-009)

**GRASSLAND STARTER LINE**
```yaml
PM-001: 
  Name: Sprigatito
  Types: [Grass]
  BaseStats: {hp: 45, attack: 49, defense: 49, spAttack: 65, spDefense: 65, speed: 45}
  Evolution: [{to: PM-002, method: level, requirement: 16}]
  Learnset: [{level: 1, move: Tackle}, {level: 5, move: Absorb}, {level: 12, move: Razor Leaf}]
  SpritePrompt: "Small green feline creature, leaf collar, 32x32 pixel art, 4 colors, Pokemon style"

PM-002:
  Name: Floragato
  Types: [Grass]
  BaseStats: {hp: 61, attack: 80, defense: 63, spAttack: 90, spDefense: 70, speed: 83}
  Evolution: [{to: PM-003, method: level, requirement: 36}]
  SpritePrompt: "Elegant green cat, flower petals, standing pose, pixel art"

PM-003:
  Name: Meowscarada
  Types: [Grass, Dark]
  BaseStats: {hp: 76, attack: 110, defense: 70, spAttack: 81, spDefense: 70, speed: 123}
  SpritePrompt: "Magician cat, green and black, mask, dramatic pose, pixel art"
```

**FIRE STARTER LINE**
```yaml
PM-004:
  Name: Fuecoco
  Types: [Fire]
  BaseStats: {hp: 67, attack: 45, defense: 59, spAttack: 63, spDefense: 40, speed: 36}
  Evolution: [{to: PM-005, method: level, requirement: 16}]
  SpritePrompt: "Red crocodile, fire belly, cute, pixel art"

PM-005:
  Name: Crocalor
  Types: [Fire]
  BaseStats: {hp: 81, attack: 55, defense: 78, spAttack: 90, spDefense: 58, speed: 49}
  Evolution: [{to: PM-006, method: level, requirement: 36}]
  SpritePrompt: "Bipedal red gator, fire egg on head, pixel art"

PM-006:
  Name: Skeledirge
  Types: [Fire, Ghost]
  BaseStats: {hp: 104, attack: 75, defense: 100, spAttack: 110, spDefense: 66, speed: 66}
  SpritePrompt: "Large crocodile, skeletal patterns, fire aura, singer, pixel art"
```

**WATER STARTER LINE**
```yaml
PM-007:
  Name: Quaxly
  Types: [Water]
  BaseStats: {hp: 55, attack: 65, defense: 45, spAttack: 50, spDefense: 45, speed: 50}
  Evolution: [{to: PM-008, method: level, requirement: 16}]
  SpritePrompt: "White duckling, blue beret, cute, pixel art"

PM-008:
  Name: Quaxwell
  Types: [Water]
  BaseStats: {hp: 70, attack: 85, defense: 65, spAttack: 65, spDefense: 60, speed: 65}
  Evolution: [{to: PM-009, method: level, requirement: 36}]
  SpritePrompt: "Blue duck, dancer pose, water droplets, pixel art"

PM-009:
  Name: Quaquaval
  Types: [Water, Fighting]
  BaseStats: {hp: 85, attack: 120, defense: 80, spAttack: 85, spDefense: 75, speed: 85}
  SpritePrompt: "Muscular duck, water wings, carnival dancer, pixel art"
```

### 2.3 Early Route Monsters (PM-010 to PM-030)

**Route 1 Commons (Catch Rate: 255-190)**
```yaml
PM-010: # Pidgey-analog
  Name: Pidove  
  Types: [Normal, Flying]
  BaseStats: {hp: 50, attack: 55, defense: 50, spAttack: 36, spDefense: 30, speed: 43}
  Evolution: [{to: PM-011, method: level, requirement: 21}]
  SpritePrompt: "Small gray pigeon, heart-shaped chest, pixel art"

PM-011: # Staraptor-analog
  Name: Staravia
  Types: [Normal, Flying]
  BaseStats: {hp: 55, attack: 75, defense: 50, spAttack: 40, spDefense: 40, speed: 62}
  Evolution: [{to: PM-012, method: level, requirement: 34}]
  SpritePrompt: "Gray bird, fierce eyebrows, pixel art"

PM-012:
  Name: Staraptor
  Types: [Normal, Flying]
  BaseStats: {hp: 85, attack: 120, defense: 70, spAttack: 50, spDefense: 60, speed: 100}
  SpritePrompt: "Large gray raptor bird, red chest, aggressive pose, pixel art"

PM-013: # Rattata-analog
  Name: Rattata
  Types: [Normal]
  BaseStats: {hp: 30, attack: 56, defense: 35, spAttack: 25, spDefense: 35, speed: 72}
  Evolution: [{to: PM-014, method: level, requirement: 20}]
  SpritePrompt: "Purple rat, large teeth, pixel art"

PM-014:
  Name: Raticate
  Types: [Normal]
  BaseStats: {hp: 55, attack: 81, defense: 60, spAttack: 50, spDefense: 70, speed: 97}
  SpritePrompt: "Large rat, cream belly, whiskers, pixel art"

PM-015: # Sentret-analog
  Name: Sentret
  Types: [Normal]
  BaseStats: {hp: 50, attack: 48, defense: 43, spAttack: 46, spDefense: 45, speed: 48}
  Evolution: [{to: PM-016, method: level, requirement: 15}]
  SpritePrompt: "Brown ferret, ring tail, standing, pixel art"

PM-016:
  Name: Furret
  Types: [Normal]
  BaseStats: {hp: 85, attack: 76, defense: 64, spAttack: 45, spDefense: 55, speed: 90}
  SpritePrompt: "Long cream ferret, standing tall, pixel art"
```

**Route 1 Uncommon (Catch Rate: 190-120)**
```yaml
PM-017: # Pikachu-analog
  Name: Pichu
  Types: [Electric]
  BaseStats: {hp: 20, attack: 40, defense: 15, spAttack: 35, spDefense: 35, speed: 60}
  Evolution: [{to: PM-018, method: friendship, requirement: 220}]
  SpritePrompt: "Tiny yellow mouse, black-tipped ears, pixel art"

PM-018:
  Name: Pikachu
  Types: [Electric]
  BaseStats: {hp: 35, attack: 55, defense: 40, spAttack: 50, spDefense: 50, speed: 90}
  Evolution: [{to: PM-019, method: stone, requirement: ITEM-THUNDERSTONE}]
  SpritePrompt: "Yellow mouse, red cheeks, lightning tail, pixel art"

PM-019:
  Name: Raichu
  Types: [Electric]
  BaseStats: {hp: 60, attack: 90, defense: 55, spAttack: 90, spDefense: 80, speed: 110}
  SpritePrompt: "Orange mouse, long ears, lightning bolt tail, pixel art"
```

**Forest Area (PM-020 to PM-035)**
```yaml
PM-020: # Caterpie-analog
  Name: Caterpie
  Types: [Bug]
  BaseStats: {hp: 45, attack: 30, defense: 35, spAttack: 20, spDefense: 20, speed: 45}
  Evolution: [{to: PM-021, method: level, requirement: 7}]
  SpritePrompt: "Green caterpillar, red antenna, yellow belly, pixel art"

PM-021:
  Name: Metapod
  Types: [Bug]
  BaseStats: {hp: 50, attack: 20, defense: 55, spAttack: 25, spDefense: 25, speed: 30}
  Evolution: [{to: PM-022, method: level, requirement: 10}]
  SpritePrompt: "Green chrysalis, hard shell, pixel art"

PM-022:
  Name: Butterfree
  Types: [Bug, Flying]
  BaseStats: {hp: 60, attack: 45, defense: 50, spAttack: 90, spDefense: 80, speed: 70}
  SpritePrompt: "Butterfly, purple wings, compound eyes, pixel art"

PM-023: # Weedle-analog
  Name: Weedle
  Types: [Bug, Poison]
  BaseStats: {hp: 40, attack: 35, defense: 30, spAttack: 20, spDefense: 20, speed: 50}
  Evolution: [{to: PM-024, method: level, requirement: 7}]
  SpritePrompt: "Brown caterpillar, stinger, pixel art"

PM-024:
  Name: Kakuna
  Types: [Bug, Poison]
  BaseStats: {hp: 45, attack: 25, defense: 50, spAttack: 25, spDefense: 25, speed: 35}
  Evolution: [{to: PM-025, method: level, requirement: 10}]
  SpritePrompt: "Yellow chrysalis, pixel art"

PM-025:
  Name: Beedrill
  Types: [Bug, Poison]
  BaseStats: {hp: 65, attack: 90, defense: 40, spAttack: 45, spDefense: 80, speed: 75}
  SpritePrompt: "Large wasp, twin stingers, pixel art"

PM-026: # Shroomish-analog
  Name: Shroomish
  Types: [Grass]
  BaseStats: {hp: 60, attack: 40, defense: 60, spAttack: 40, spDefense: 60, speed: 35}
  Evolution: [{to: PM-027, method: level, requirement: 23}]
  SpritePrompt: "Mushroom creature, green, pixel art"

PM-027:
  Name: Breloom
  Types: [Grass, Fighting]
  BaseStats: {hp: 60, attack: 130, defense: 80, spAttack: 60, spDefense: 60, speed: 70}
  SpritePrompt: "Mushroom kangaroo, green, boxing pose, pixel art"
```

*Continue this pattern for PM-028 through PM-150...*

### 2.4 Mid-Game Monsters (PM-036 to PM-080)

**Pseudo-Legendary Line (PM-077 to PM-079)**
```yaml
PM-077:
  Name: Bagon
  Types: [Dragon]
  BaseStats: {hp: 45, attack: 75, defense: 60, spAttack: 40, spDefense: 30, speed: 50}
  Evolution: [{to: PM-078, method: level, requirement: 30}]
  SpritePrompt: "Small blue dragon, head bumps, pixel art"

PM-078:
  Name: Shelgon
  Types: [Dragon]
  BaseStats: {hp: 65, attack: 95, defense: 100, spAttack: 60, spDefense: 50, speed: 50}
  Evolution: [{to: PM-079, method: level, requirement: 50}]
  SpritePrompt: "Blue turtle shell, dragon inside, pixel art"

PM-079:
  Name: Salamence
  Types: [Dragon, Flying]
  BaseStats: {hp: 95, attack: 135, defense: 80, spAttack: 110, spDefense: 80, speed: 100}
  SpritePrompt: "Blue dragon, red wings, angry, pixel art"
```

**Fossil Monsters (PM-080 to PM-085)**
```yaml
PM-080:
  Name: Anorith
  Types: [Rock, Bug]
  BaseStats: {hp: 45, attack: 95, defense: 50, spAttack: 40, spDefense: 50, speed: 75}
  Evolution: [{to: PM-081, method: level, requirement: 40}]
  SpritePrompt: "Ancient shrimp, green, pixel art"

PM-081:
  Name: Armaldo
  Types: [Rock, Bug]
  BaseStats: {hp: 75, attack: 125, defense: 100, spAttack: 70, spDefense: 80, speed: 45}
  SpritePrompt: "Large crustacean, gray armor, pixel art"
```

### 2.5 Late-Game & Legendaries (PM-140 to PM-150)

**Legendary Trio (PM-147 to PM-149)**
```yaml
PM-147:
  Name: Raikou
  Types: [Electric]
  BaseStats: {hp: 90, attack: 85, defense: 75, spAttack: 115, spDefense: 100, speed: 115}
  CatchRate: 3
  SpritePrompt: "Purple sabertooth tiger, lightning mane, pixel art"

PM-148:
  Name: Entei
  Types: [Fire]
  BaseStats: {hp: 115, attack: 115, defense: 85, spAttack: 90, spDefense: 75, speed: 100}
  CatchRate: 3
  SpritePrompt: "Brown lion, fire mane, pixel art"

PM-149:
  Name: Suicune
  Types: [Water]
  BaseStats: {hp: 100, attack: 75, defense: 115, spAttack: 90, spDefense: 115, speed: 85}
  CatchRate: 3
  SpritePrompt: "Blue panther, white mane, water aura, pixel art"
```

**Box Legendary (PM-150)**
```yaml
PM-150:
  Name: Lugia
  Types: [Psychic, Flying]
  BaseStats: {hp: 106, attack: 90, defense: 130, spAttack: 90, spDefense: 154, speed: 110}
  CatchRate: 3
  SpritePrompt: "Silver bird dragon, blue belly, pixel art"
```

---

## 3. SPRITE GENERATION PROTOCOL

### 3.1 Pixel Art Specifications
- **Canvas:** 32×32 pixels (battle sprites), 16×16 (overworld)
- **Palette:** 4 colors maximum + transparency
  - 1 Base color
  - 1 Shadow color (darker)
  - 1 Highlight color (lighter)
  - 1 Detail color (eyes, special features)
- **Outline:** 1px black outline mandatory
- **Style:** Game Boy Color / early GBA era

### 3.2 AI Generation Prompt Template
```
"32x32 pixel art sprite, [description], 4-color palette, 
black outline, transparent background, Pokemon style, 
front view, centered, no anti-aliasing"
```

### 3.3 Batch Generation Strategy
For agentic workflows:
1. **Generate in batches of 10** (fits in context window)
2. **File naming:** `PM-001_front.png`, `PM-001_back.png`, `PM-001_icon.png`
3. **Validation:** Check 32×32 dimensions, 4-color limit, black outline presence
4. **Storage:** `/public/sprites/monsters/[id]/[view].png`

---

## 4. STAT BALANCE FORMULAS

### 4.1 Base Stat Total (BST) Progression
- **Stage 1 (Unevolved):** 180-320 BST
- **Stage 2 (Mid-Evolution):** 340-450 BST  
- **Stage 3 (Fully Evolved):** 460-600 BST
- **Pseudo-Legendary:** 600 BST
- **Legendary:** 580-680 BST

### 4.2 Stat Distribution Archetypes
```typescript
type Archetype = 
  | 'Physical Sweeper'  // High Atk/Spe, Low Def
  | 'Special Sweeper'   // High SpA/Spe, Low Def  
  | 'Physical Tank'     // High HP/Def, Low Spe
  | 'Special Tank'      // High HP/SpD, Low Spe
  | 'Mixed Attacker'    // Balanced Atk/SpA
  | 'Support'           // High HP/Def, Status moves
  | 'Glass Cannon'      // Very High Atk/SpA, Very Low Def/HP
  | 'Bulky Attacker';   // High HP + One Attack stat

// STAT ALLOCATION FORMULA
function allocateStats(archetype: Archetype, bst: number): Stats {
  const weights = ARCHETYPE_WEIGHTS[archetype];
  return distribute(bst, weights); // Pseudocode: Proportional distribution
}
```

### 4.3 Evolution Stat Boosts
- **Level Evolution:** +80 to +120 BST total
- **Stone Evolution:** +100 to +140 BST total (immediate power spike)
- **Trade Evolution:** +120 to +160 BST total (reward for multiplayer)

---

## 5. MOVE LEARNSET GENERATION

### 5.1 Level-Up Move Distribution
- **Level 1:** 2-4 starting moves (basic tackle/growl equivalents)
- **Every 3-5 levels:** New move or upgrade
- **Evolution level:** Signature move (e.g., Charizard learns Flamethrower at 36)
- **Level 50+:** Powerful STAB moves

### 5.2 TM Compatibility Rules
- **Type Matching:** Monster can learn TMs matching its type
- **Physical/Special Split:** Physical monsters learn physical TMs
- **Universal TMs:** Protect, Rest, Sleep Talk, Substitute (all monsters)
- **Elemental TMs:** Fire Blast, Thunder, Blizzard (80% accuracy, high power)

### 5.3 Egg Move Inheritance
- Father passes egg moves
- Compatible if shared egg group
- Chain breeding for rare moves

---

## 6. IMPLEMENTATION CHECKLIST

### Phase 1: Data Layer (Week 1)
- [ ] Implement TypeScript interfaces
- [ ] Create Zod validation schemas
- [ ] Build MonsterFactory class (generates from YAML/JSON specs)
- [ ] Implement MoveDex (all 165 TMs + 50 unique moves)
- [ ] Implement AbilityDex (50 abilities)

### Phase 2: Content Generation (Week 2-3)
- [ ] Generate PM-001 to PM-010 (Starters + early routes)
- [ ] Generate PM-011 to PM-050 (Common monsters)
- [ ] Generate PM-051 to PM-100 (Mid-game)
- [ ] Generate PM-101 to PM-150 (Late-game + Legendaries)
- [ ] Generate all sprites (batch process)
- [ ] Balance pass: Check BST distributions

### Phase 3: Integration (Week 4)
- [ ] Export as JSON for game engine
- [ ] Build Pokedex UI component
- [ ] Implement search/filter (type, generation, stats)
- [ ] Team builder tool

---

## 7. YAML TEMPLATE FOR AGENTIC GENERATION

```yaml
# Template for generating new monsters
monster:
  id: PM-XXX
  name: [Name]
  types: [Type1, Type2?]
  archetype: [Archetype]
  bst_target: [Number]
  evolution_stage: [1|2|3]
  evolves_to: PM-XXX | null
  evolution_method: level | stone | trade | friendship
  sprite_description: "[Detailed visual description for AI generation]"
  learnset:
    - level: 1
      move: Tackle
    - level: [X]
      move: [MoveName]
  tm_compatibility: [Fire, Normal, etc.]
  egg_groups: [Group1, Group2?]
  pokedex_entry: "[Flavor text]"
```

**END OF MONSTER FACTORY SPECIFICATION**

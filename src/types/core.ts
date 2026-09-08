/**
 * Primitive / literal type definitions shared across Pixelmon.
 *
 * Mirrors the contract in PIXELMON_FACTORY.md §1.1 and the GDD data schema.
 * These are string-literal union types (not TS enums) so data files can use
 * them as plain strings and stay JSON-serializable.
 */

/** The 18 battle types (GDD §3.1.4). */
export const TYPES = [
  "Normal",
  "Fire",
  "Water",
  "Electric",
  "Grass",
  "Ice",
  "Fighting",
  "Poison",
  "Ground",
  "Flying",
  "Psychic",
  "Bug",
  "Rock",
  "Ghost",
  "Dragon",
  "Dark",
  "Steel",
  "Fairy",
] as const;

export type Type = (typeof TYPES)[number];

/** Egg groups (PIXELMON_FACTORY.md §1.1). */
export const EGG_GROUPS = [
  "Monster",
  "Water1",
  "Water2",
  "Water3",
  "Bug",
  "Flying",
  "Field",
  "Fairy",
  "Grass",
  "HumanLike",
  "Mineral",
  "Amorphous",
  "Dragon",
  "Undiscovered",
] as const;

export type EggGroup = (typeof EGG_GROUPS)[number];

/** Growth rates (GDD §3.3.2). */
export const GROWTH_RATES = [
  "Erratic",
  "Fast",
  "MediumFast",
  "MediumSlow",
  "Slow",
  "Fluctuating",
] as const;

export type GrowthRate = (typeof GROWTH_RATES)[number];

/** Natures: +1 stat, -1 stat (GDD §3.4). */
export const NATURES = [
  "Hardy",
  "Lonely",
  "Brave",
  "Adamant",
  "Naughty",
  "Bold",
  "Docile",
  "Relaxed",
  "Impish",
  "Lax",
  "Timid",
  "Hasty",
  "Serious",
  "Jolly",
  "Naive",
  "Modest",
  "Mild",
  "Quiet",
  "Bashful",
  "Rash",
  "Calm",
  "Gentle",
  "Sassy",
  "Careful",
  "Quirky",
] as const;

export type Nature = (typeof NATURES)[number];

/** Move categories (PIXELMON_FACTORY.md §1.2). */
export const MOVE_CATEGORIES = ["Physical", "Special", "Status"] as const;
export type MoveCategory = (typeof MOVE_CATEGORIES)[number];

/** Move targets (PIXELMON_FACTORY.md §1.2). */
export const MOVE_TARGETS = ["self", "enemy", "all", "ally", "random"] as const;
export type MoveTarget = (typeof MOVE_TARGETS)[number];

/** Evolution methods (PIXELMON_FACTORY.md §1.1). */
export const EVOLUTION_METHODS = [
  "level",
  "stone",
  "trade",
  "friendship",
  "time",
  "location",
  "move",
] as const;
export type EvolutionMethod = (typeof EVOLUTION_METHODS)[number];

/** Status conditions (GDD §3.1.5). */
export const STATUS_CONDITIONS = [
  "Burn",
  "Poison",
  "BadlyPoisoned",
  "Paralysis",
  "Sleep",
  "Freeze",
  "Confusion",
  "Flinch",
  "Trap",
  "LeechSeed",
  "Curse",
] as const;
export type StatusCondition = (typeof STATUS_CONDITIONS)[number];

/** Weather states (GDD §3.1.1 / §7.3). */
export const WEATHERS = [
  "Clear",
  "Rain",
  "Sun",
  "Sandstorm",
  "Hail",
  "Snow",
  "Fog",
] as const;
export type Weather = (typeof WEATHERS)[number];

/** Battle kinds (GDD §3.1.1). */
export const BATTLE_TYPES = ["wild", "trainer", "gym", "elite4", "champion"] as const;
export type BattleType = (typeof BATTLE_TYPES)[number];

/** Overworld movement directions (GDD §2.2). */
export const DIRECTIONS = ["up", "down", "left", "right"] as const;
export type Direction = (typeof DIRECTIONS)[number];

/** A gym badge (GDD §2.2). */
export const BADGE_TYPES = [
  "Grass",
  "Rock",
  "Fire",
  "Ground",
  "Ice",
  "Water",
  "Flying",
  "Psychic",
] as const;
export type BadgeType = (typeof BADGE_TYPES)[number];

/** ID aliases used throughout the data layer. */
export type MonsterID = string; // "PM-001" … "PM-150"
export type MoveID = string; // "MV-TACKLE"
export type AbilityID = string; // "AB-OVERGROW"
export type ItemID = string; // "ITEM-POTION"
export type TMNumber = number; // 0–100
export type SpriteRef = string;

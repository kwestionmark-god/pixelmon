/**
 * Damage calculation (GDD §3.1.3, Gen 5+ formula).
 *
 * base = FLOOR( FLOOR( (2*level/5 + 2) * power * attackStat / defenseStat ) / 50 ) + 2
 * damage = FLOOR( base * STAB * effectiveness * random * crit * weather * burn )
 * final damage is clamped to a minimum of 1.
 */
import type { Move, Stats, StatusCondition, Type, Weather } from "@/types";
import { getEffectiveness } from "./typeChart.js";
import { rollBelow } from "@/utils/rng.js";

/** Base critical-hit probability (1 in 24, Gen 5). */
export const CRIT_CHANCE = 1 / 24;

export interface DamageActor {
  level: number;
  stats: Stats;
  types: readonly Type[];
  ability?: string;
  status: StatusCondition | null;
}

export interface DamageContext {
  attacker: DamageActor;
  defender: DamageActor;
  move: Move;
  weather: Weather | null;
  /** Seeded RNG; defaults to Math.random (nondeterministic). */
  rng?: () => number;
}

export interface DamageBreakdown {
  rawDamage: number; // after all multipliers, before min-clamp
  finalDamage: number; // clamped to >= 1
  crit: boolean;
  effectiveness: number;
  stab: number;
  weatherMod: number;
  burnMod: number;
  random: number;
}

/**
 * Compute damage for a single move. Returns a full breakdown so callers
 * (and tests) can inspect each multiplier.
 */
export function calculateDamage(ctx: DamageContext): DamageBreakdown {
  const { attacker, defender, move } = ctx;
  const rng = ctx.rng ?? Math.random;
  const isSpecial = move.category === "Special";

  // Physical vs Special stat selection.
  const attackStat = isSpecial ? attacker.stats.spAttack : attacker.stats.attack;
  const defenseStat = isSpecial ? defender.stats.spDefense : defender.stats.defense;

  const base =
    Math.floor(
      Math.floor(
        ((2 * attacker.level) / 5 + 2) *
          move.power *
          attackStat /
          defenseStat,
      ) / 50,
    ) + 2;

  // STAB.
  const stab = attacker.types.includes(move.type) ? 1.5 : 1;

  // Type effectiveness against every defender type.
  const effectiveness = getEffectiveness(move.type, defender.types);

  // Immunity (0×) deals no damage; the min-1 clamp below only applies to
  // otherwise-real hits whose rounded value would drop below 1.
  if (effectiveness === 0) {
    return {
      rawDamage: 0,
      finalDamage: 0,
      crit: false,
      effectiveness: 0,
      stab,
      weatherMod: 1,
      burnMod: 1,
      random: 0,
    };
  }

  // Random factor in [0.85, 1.0].
  const random = 0.85 + rng() * 0.15;

  // Critical hit.
  const crit = rollBelow(rng, CRIT_CHANCE);
  const critMod = crit ? 1.5 : 1;

  const weatherMod = getWeatherMod(ctx.weather, move.type);
  const burnMod = getBurnMod(attacker.status, move.category);

  const rawDamage =
    base * stab * effectiveness * random * critMod * weatherMod * burnMod;
  const finalDamage = Math.max(1, Math.floor(rawDamage));

  return {
    rawDamage,
    finalDamage,
    crit,
    effectiveness,
    stab,
    weatherMod,
    burnMod,
    random,
  };
}

/**
 * Weather damage modifiers (GDD §3.1.3). Rain powers Water, weakens Fire;
 * Sun does the inverse.
 */
export function getWeatherMod(weather: Weather | null, moveType: Type): number {
  switch (weather) {
    case "Rain":
      return moveType === "Water" ? 1.5 : moveType === "Fire" ? 0.5 : 1;
    case "Sun":
      return moveType === "Fire" ? 1.5 : moveType === "Water" ? 0.5 : 1;
    default:
      return 1;
  }
}

/** Burn halves Physical attack power (GDD §3.1.3). */
export function getBurnMod(
  attackerStatus: StatusCondition | null,
  category: Move["category"],
): number {
  return attackerStatus === "Burn" && category === "Physical" ? 0.5 : 1;
}

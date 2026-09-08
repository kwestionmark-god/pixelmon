/**
 * Experience & leveling (GDD §3.3).
 *
 *  §3.3.1  XP yield when a monster is defeated.
 *  §3.3.2  Growth-rate curves: XP required to reach a given level.
 */
import type { GrowthRate } from "@/types";

/** XP curves (GDD §3.3.2). `level` is the target level. */
export function experienceToLevel(
  level: number,
  growthRate: GrowthRate,
): number {
  const n = level;
  switch (growthRate) {
    case "Fast":
      return Math.floor((4 * n ** 3) / 5);
    case "MediumFast":
      return n ** 3;
    case "Slow":
      return Math.floor((5 * n ** 3) / 4);
    case "Erratic":
      if (n <= 50) return Math.floor((100 * n ** 3) / 50);
      if (n <= 68) return Math.floor((100 * n ** 3) / 100);
      if (n <= 98) return Math.floor((100 * n ** 3) / 500);
      return Math.floor((100 * n ** 3) / 1000);
    case "MediumSlow":
      return Math.max(
        0,
        Math.floor(
          (6 / 5) * n ** 3 - 15 * n ** 2 + 100 * n - 140,
        ),
      );
    case "Fluctuating":
      if (n <= 15)
        return Math.floor(n ** 3 * ((24 + Math.floor((n + 1) / 3)) / 50));
      if (n <= 36) return Math.floor(n ** 3 * ((14 + n) / 50));
      return Math.floor(n ** 3 * ((32 + Math.floor(n / 2)) / 50));
    default:
      throw new Error(`Unknown growth rate: ${growthRate as string}`);
  }
}

export interface XpYieldParams {
  /** The defeated monster's base XP value (Monster.baseExp). */
  baseExp: number;
  /** The defeated monster's level. */
  defeatedLevel: number;
  /** Whether the battle was against a trainer (×1.5). */
  isTrainerBattle: boolean;
  /** Whether the winner holds a Lucky Egg (×1.5). */
  holdsLuckyEgg: boolean;
  /** Whether the winner was traded (×1.5). */
  wasTraded: boolean;
  /**
   * Number of monsters that participated in the battle. When the winner
   * participated, XP is divided among participants; otherwise the
   * non-participant "Exp Share" halving applies.
   */
  participants: number;
  /** Did the winner participate directly? */
  participated: boolean;
}

/**
 * Calculate XP awarded to a winner (GDD §3.3.1).
 */
export function calculateXpYield(
  params: XpYieldParams,
): number {
  const {
    baseExp,
    defeatedLevel,
    isTrainerBattle,
    holdsLuckyEgg,
    wasTraded,
    participants,
    participated,
  } = params;

  let xp = (baseExp * defeatedLevel) / 5;

  if (isTrainerBattle) xp *= 1.5;
  if (holdsLuckyEgg) xp *= 1.5;
  if (wasTraded) xp *= 1.5;

  if (participated) {
    return Math.floor(xp / Math.max(1, participants));
  }
  return Math.floor(xp / 2);
}

/**
 * Given current exp and growth rate, return the level the monster has reached
 * (largest level whose cumulative XP requirement is <= current exp).
 */
export function levelFromExp(
  exp: number,
  growthRate: GrowthRate,
): number {
  let level = 1;
  while (experienceToLevel(level + 1, growthRate) <= exp) {
    level += 1;
  }
  return level;
}

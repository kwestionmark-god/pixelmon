/**
 * Stat calculation (GDD §3.4, Gen 3+ formula).
 *
 *   HP  = FLOOR( (2*base.hp + ivs.hp + FLOOR(evs.hp/4)) * level / 100 ) + level + 10
 *   Other stats follow the same shape, adding +5 before the nature modifier.
 *   Then apply the nature modifier (±10% or none).
 */
import type { EVs, Nature, Stats } from "@/types";

/** Nature stat modifiers: [increased, decreased, neutral] mapped per stat. */
const NATURE_MODIFIERS: Record<Nature, Partial<Record<keyof Stats, number>>> = {
  Hardy: {},
  Lonely: { attack: 1.1, defense: 0.9 },
  Brave: { attack: 1.1, speed: 0.9 },
  Adamant: { attack: 1.1, spAttack: 0.9 },
  Naughty: { attack: 1.1, spDefense: 0.9 },
  Bold: { defense: 1.1, attack: 0.9 },
  Docile: {},
  Relaxed: { defense: 1.1, speed: 0.9 },
  Impish: { defense: 1.1, spAttack: 0.9 },
  Lax: { defense: 1.1, spDefense: 0.9 },
  Timid: { speed: 1.1, attack: 0.9 },
  Hasty: { speed: 1.1, defense: 0.9 },
  Serious: { speed: 1.1, spAttack: 0.9 },
  Jolly: { speed: 1.1, spDefense: 0.9 },
  Naive: { speed: 1.1, attack: 0.9 },
  Modest: { spAttack: 1.1, attack: 0.9 },
  Mild: { spAttack: 1.1, defense: 0.9 },
  Quiet: { spAttack: 1.1, speed: 0.9 },
  Bashful: {},
  Rash: { spAttack: 1.1, defense: 0.9 },
  Calm: { spDefense: 1.1, attack: 0.9 },
  Gentle: { spDefense: 1.1, defense: 0.9 },
  Sassy: { spDefense: 1.1, speed: 0.9 },
  Careful: { spDefense: 1.1, spAttack: 0.9 },
  Quirky: {},
};

/**
 * Calculate a fully evolved stat set from base stats, IVs, EVs, level and
 * nature. `ivs` and `evs` are partial; missing fields default to 0.
 */
export function calculateStats(params: {
  baseStats: Stats;
  ivs: EVs;
  evs: EVs;
  level: number;
  nature: Nature;
}): Stats {
  const { baseStats, ivs, evs, level, nature } = params;
  const natureMod = NATURE_MODIFIERS[nature] ?? {};

  const hp =
    Math.floor(
      (2 * baseStats.hp + (ivs.hp ?? 0) + Math.floor((evs.hp ?? 0) / 4)) *
        level /
        100,
    ) + level + 10;

  const buildStat = (key: keyof Stats): number => {
    const raw =
      Math.floor(
        (2 * baseStats[key] + (ivs[key] ?? 0) + Math.floor((evs[key] ?? 0) / 4)) *
          level /
          100,
      ) + 5;
    const modifier = natureMod[key];
    return modifier ? Math.floor(raw * modifier) : raw;
  };

  return {
    hp,
    attack: buildStat("attack"),
    defense: buildStat("defense"),
    spAttack: buildStat("spAttack"),
    spDefense: buildStat("spDefense"),
    speed: buildStat("speed"),
  };
}

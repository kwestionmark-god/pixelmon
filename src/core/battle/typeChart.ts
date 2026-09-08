/**
 * Type effectiveness chart (GDD §3.1.4).
 *
 * A canonical 18×18 Gen-5-style matrix. `typeChart[attackType][defenseType]`
 * is the damage multiplier applied to an attack of `attackType` against a
 * defender of `defenseType`: 2 = super effective, 1 = normal, 0.5 = resisted,
 * 0 = no effect (immune).
 */
import type { Type } from "@/types";
import { TYPES } from "@/types";

export const typeChart: Record<Type, Record<Type, number>> = {
  Normal: {
    Normal: 1, Fire: 0.5, Water: 1, Electric: 1, Grass: 1, Ice: 1,
    Fighting: 1, Poison: 1, Ground: 1, Flying: 0.5, Psychic: 1, Bug: 1,
    Rock: 0.5, Ghost: 0, Dragon: 1, Dark: 1, Steel: 0.5, Fairy: 0.5,
  },
  Fire: {
    Normal: 1, Fire: 0.5, Water: 2, Electric: 1, Grass: 2, Ice: 2,
    Fighting: 1, Poison: 1, Ground: 1, Flying: 1, Psychic: 1, Bug: 0.5,
    Rock: 2, Ghost: 1, Dragon: 1, Dark: 1, Steel: 2, Fairy: 0.5,
  },
  Water: {
    Normal: 1, Fire: 2, Water: 0.5, Electric: 1, Grass: 0.5, Ice: 1,
    Fighting: 1, Poison: 1, Ground: 2, Flying: 1, Psychic: 1, Bug: 1,
    Rock: 2, Ghost: 1, Dragon: 0.5, Dark: 1, Steel: 0.5, Fairy: 1,
  },
  Electric: {
    Normal: 1, Fire: 1, Water: 2, Electric: 0, Grass: 0.5, Ice: 1,
    Fighting: 1, Poison: 1, Ground: 0, Flying: 2, Psychic: 1, Bug: 1,
    Rock: 1, Ghost: 1, Dragon: 0.5, Dark: 1, Steel: 0.5, Fairy: 1,
  },
  Grass: {
    Normal: 1, Fire: 0.5, Water: 2, Electric: 2, Grass: 0.5, Ice: 1,
    Fighting: 1, Poison: 0.5, Ground: 2, Flying: 0.5, Psychic: 1, Bug: 2,
    Rock: 0.5, Ghost: 0.5, Dragon: 0.5, Dark: 1, Steel: 0.5, Fairy: 0.5,
  },
  Ice: {
    Normal: 1, Fire: 0.5, Water: 0.5, Electric: 1, Grass: 2, Ice: 0.5,
    Fighting: 1, Poison: 1, Ground: 2, Flying: 2, Psychic: 1, Bug: 1,
    Rock: 1, Ghost: 1, Dragon: 2, Dark: 1, Steel: 0.5, Fairy: 1,
  },
  Fighting: {
    Normal: 2, Fire: 1, Water: 1, Electric: 1, Grass: 1, Ice: 2,
    Fighting: 1, Poison: 0.5, Ground: 0.5, Flying: 0, Psychic: 0.5, Bug: 0.5,
    Rock: 2, Ghost: 0, Dragon: 1, Dark: 2, Steel: 0.5, Fairy: 0.5,
  },
  Poison: {
    Normal: 1, Fire: 1, Water: 1, Electric: 1, Grass: 2, Ice: 1,
    Fighting: 1, Poison: 0.5, Ground: 0.5, Flying: 1, Psychic: 1, Bug: 2,
    Rock: 0.5, Ghost: 0.5, Dragon: 1, Dark: 1, Steel: 0, Fairy: 2,
  },
  Ground: {
    Normal: 1, Fire: 2, Water: 1, Electric: 2, Grass: 0.5, Ice: 1,
    Fighting: 1, Poison: 2, Ground: 1, Flying: 0, Psychic: 1, Bug: 0.5,
    Rock: 2, Ghost: 1, Dragon: 1, Dark: 1, Steel: 2, Fairy: 1,
  },
  Flying: {
    Normal: 1, Fire: 1, Water: 1, Electric: 0.5, Grass: 2, Ice: 1,
    Fighting: 0.5, Poison: 1, Ground: 0.5, Flying: 1, Psychic: 1, Bug: 2,
    Rock: 0.5, Ghost: 1, Dragon: 1, Dark: 1, Steel: 0, Fairy: 1,
  },
  Psychic: {
    Normal: 1, Fire: 1, Water: 1, Electric: 1, Grass: 1, Ice: 1,
    Fighting: 2, Poison: 2, Ground: 1, Flying: 1, Psychic: 0.5, Bug: 1,
    Rock: 1, Ghost: 0, Dragon: 1, Dark: 0, Steel: 0.5, Fairy: 1,
  },
  Bug: {
    Normal: 1, Fire: 0.5, Water: 1, Electric: 1, Grass: 2, Ice: 1,
    Fighting: 0.5, Poison: 0.5, Ground: 1, Flying: 1, Psychic: 2, Bug: 1,
    Rock: 0.5, Ghost: 0.5, Dragon: 1, Dark: 2, Steel: 0.5, Fairy: 0.5,
  },
  Rock: {
    Normal: 1, Fire: 2, Water: 1, Electric: 1, Grass: 0.5, Ice: 0.5,
    Fighting: 2, Poison: 1, Ground: 0.5, Flying: 2, Psychic: 0.5, Bug: 2,
    Rock: 1, Ghost: 1, Dragon: 1, Dark: 1, Steel: 0.5, Fairy: 1,
  },
  Ghost: {
    Normal: 0, Fire: 1, Water: 1, Electric: 1, Grass: 1, Ice: 1,
    Fighting: 1, Poison: 1, Ground: 1, Flying: 1, Psychic: 2, Bug: 1,
    Rock: 1, Ghost: 2, Dragon: 1, Dark: 0, Steel: 0.5, Fairy: 1,
  },
  Dragon: {
    Normal: 1, Fire: 1, Water: 1, Electric: 1, Grass: 1, Ice: 1,
    Fighting: 1, Poison: 1, Ground: 1, Flying: 1, Psychic: 1, Bug: 1,
    Rock: 1, Ghost: 1, Dragon: 2, Dark: 1, Steel: 0.5, Fairy: 0,
  },
  Dark: {
    Normal: 1, Fire: 1, Water: 1, Electric: 1, Grass: 1, Ice: 1,
    Fighting: 1, Poison: 1, Ground: 1, Flying: 1, Psychic: 2, Bug: 1,
    Rock: 0.5, Ghost: 2, Dragon: 1, Dark: 0, Steel: 0.5, Fairy: 0.5,
  },
  Steel: {
    Normal: 1, Fire: 0.5, Water: 0.5, Electric: 1, Grass: 1, Ice: 2,
    Fighting: 1, Poison: 1, Ground: 1, Flying: 1, Psychic: 1, Bug: 1,
    Rock: 1, Ghost: 1, Dragon: 1, Dark: 1, Steel: 0.5, Fairy: 2,
  },
  Fairy: {
    Normal: 1, Fire: 1, Water: 1, Electric: 0.5, Grass: 1, Ice: 1,
    Fighting: 2, Poison: 0.5, Ground: 1, Flying: 1, Psychic: 1, Bug: 1,
    Rock: 1, Ghost: 1, Dragon: 0, Dark: 2, Steel: 0.5, Fairy: 0.5,
  },
};

/**
 * Total effectiveness multiplier of an attack against a defender's type list.
 * Multiplies the per-type multiplier; an immune type (0) zeroes the result.
 */
export function getEffectiveness(
  attackType: Type,
  defenseTypes: readonly Type[],
): number {
  let multiplier = 1;
  for (const defenseType of defenseTypes) {
    multiplier *= typeChart[attackType][defenseType];
  }
  return multiplier;
}

/** Human-readable interpretation of a damage multiplier. */
export function interpretEffectiveness(multiplier: number):
  | "no effect"
  | "not very effective"
  | "super effective"
  | "normal" {
  if (multiplier === 0) return "no effect";
  if (multiplier < 1) return "not very effective";
  if (multiplier > 1) return "super effective";
  return "normal";
}

/** All 18 battle types, in canonical order. */
export { TYPES };

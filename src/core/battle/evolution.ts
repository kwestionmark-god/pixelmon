/**
 * Evolution resolution (GDD §3.5).
 *
 * Walks a monster's evolution list and returns the first matching target
 * MonsterID, or null when nothing applies.
 */
import type { Evolution, MonsterID } from "@/types";

export interface EvolutionContext {
  level: number;
  friendship: number;
  timeOfDay: "day" | "night";
  /** Move the monster currently knows (for move-based evolutions). */
  knowsMove: (moveId: string) => boolean;
  /** Item applied at this moment (for stone evolutions). */
  itemApplied?: string;
  /** Whether a trade is currently occurring. */
  traded?: boolean;
  /** Location id (for location-based evolutions). */
  location?: string;
}

/**
 * Returns the evolving-to MonsterID for the first matching evolution method,
 * or null if none apply at this time.
 */
export function checkEvolution(
  evolutions: Evolution[] | undefined,
  ctx: EvolutionContext,
): MonsterID | null {
  if (!evolutions || evolutions.length === 0) return null;

  for (const evo of evolutions) {
    switch (evo.method) {
      case "level":
        if (ctx.level >= Number(evo.requirement)) return evo.to;
        continue;
      case "stone":
        if (evo.requirement === ctx.itemApplied) return evo.to;
        continue;
      case "trade":
        if (ctx.traded) return evo.to;
        continue;
      case "friendship":
        if (ctx.friendship >= Number(evo.requirement)) return evo.to;
        continue;
      case "time":
        if (
          ctx.level >= Number(evo.requirement) &&
          ctx.timeOfDay === evo.condition
        ) {
          return evo.to;
        }
        continue;
      case "move":
        if (ctx.knowsMove(String(evo.requirement))) return evo.to;
        continue;
      case "location":
        if (evo.requirement === ctx.location) return evo.to;
        continue;
      default:
        continue;
    }
  }

  return null;
}

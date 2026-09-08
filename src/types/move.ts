/**
 * Move + ability + item data types (PIXELMON_FACTORY.md §1.2 / §1.3).
 */
import type {
  AbilityID,
  MoveCategory,
  MoveID,
  MoveTarget,
  StatusCondition,
  Type,
} from "./core.js";
import type { Stats } from "./monster.js";

/** Secondary move effect (PIXELMON_FACTORY.md §1.2). */
export interface MoveEffect {
  /** Inflicts this status condition. */
  status?: StatusCondition;
  /** Chance (0–1) the status is inflicted. */
  chance?: number;
  /** Damage multiplier applied to the target (e.g. -0.25 for recoil reduction). */
  damage?: number;
  /** Stat change to apply, e.g. { stat: "defense", amount: -1 }. */
  statStages?: { stat: keyof Stats; amount: number }[];
  /** Arbitrary extra flags (weather, terrain, etc.). */
  [key: string]: unknown;
}

/** A move definition (PIXELMON_FACTORY.md §1.2). */
export interface Move {
  id: MoveID;
  name: string;
  type: Type;
  category: MoveCategory;
  power: number; // 0 for status moves
  accuracy: number; // 1–100, or 0 for always-hit
  pp: number; // 5–40
  priority: number; // -7 to +7
  target: MoveTarget;
  effect?: MoveEffect;
  description: string;
}

/** Ability definition (PIXELMON_FACTORY.md §1.3). */
export interface Ability {
  id: AbilityID;
  name: string;
  description: string;
  effect: string; // pseudocode trigger
}

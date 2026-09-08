/**
 * Turn-order resolution (GDD §3.1.2).
 *
 * Each action gets a priority score = priorityValue. Higher goes first.
 * Ties are broken by a random draw so order is not deterministic on parity.
 *
 *   switch / item -> +6
 *   run           -> -7
 *   normal move    -> move.priority (typically -7..+5)
 *
 *   priorityValue = (priority * 1000) + effectiveSpeed
 *   effectiveSpeed = speed * (1.5 if Quick Feet and status active else 1)
 */
import type { AbilityID, Move, StatusCondition } from "@/types";
import { rollBelow } from "@/utils/rng.js";

export const ACTION_PRIORITY = {
  switch: 6,
  item: 6,
  run: -7,
} as const;

export interface TurnAction {
  kind: "move" | "switch" | "item" | "run";
  user: {
    speed: number;
    ability?: AbilityID;
    status: StatusCondition | null;
  };
  move?: Move;
}

export interface ResolvedAction extends TurnAction {
  /** The numeric priority score used for ordering. */
  priorityValue: number;
}

/** Quick Feet boosts speed by 50% while statused (GDD §3.1.2). */
function effectiveSpeed(action: TurnAction): number {
  const hasQuickFeet = action.user.ability === "AB-QUICK_FEET";
  const statused = action.user.status !== null;
  return hasQuickFeet && statused ? action.user.speed * 1.5 : action.user.speed;
}

function basePriority(action: TurnAction): number {
  if (action.kind === "move") {
    // A status move still carries a priority (0 for most).
    return action.move ? action.move.priority : 0;
  }
  return ACTION_PRIORITY[action.kind] as number;
}

/**
 * Order a set of simultaneous actions by priority, breaking ties with a
 * random 50/50 draw. Ties that remain tied keep their input order (stable).
 */
export function resolveTurnOrder(
  actions: readonly TurnAction[],
  rng: () => number = Math.random,
): ResolvedAction[] {
  const resolved = actions.map((action) => ({
    ...action,
    priorityValue: basePriority(action) * 1000 + effectiveSpeed(action),
  }));

  // Descending by priorityValue; ties broken by random draw.
  return resolved.sort((a, b) => {
    if (a.priorityValue !== b.priorityValue) {
      return b.priorityValue - a.priorityValue;
    }
    // 50/50 coin flip for same-priority actions.
    return rollBelow(rng, 0.5) ? -1 : 1;
  });
}

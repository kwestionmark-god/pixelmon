/**
 * Capture rate + shake-check math (GDD §3.2.1).
 *
 * The classic Gen 3–5 capture formula:
 *
 *   a = ((3*maxHP - 2*currentHP) * catchRate * ballMod * statusMod) / (3*maxHP)
 *   if a >= 255            -> immediate catch
 *   b = 1048560 / sqrt(sqrt(16711680 / a))
 *   4 shake checks; each succeeds if rand(0,65535) < b. All 4 -> caught.
 */
import type { StatusCondition } from "@/types";
import { rollInt } from "@/utils/rng.js";

/** Ball catch-rate multipliers (GDD §3.2.1). */
export const BALL_MODIFIERS = {
  Poke: 1,
  Great: 1.5,
  Ultra: 2,
  Master: 255,
} as const;

export type BallType = keyof typeof BALL_MODIFIERS;

/** Status bonuses applied to the catch rate (GDD §3.2.1). */
export function getStatusMod(status: StatusCondition | null): number {
  if (status === "Sleep" || status === "Freeze") return 2;
  if (
    status === "Paralysis" ||
    status === "Poison" ||
    status === "Burn" ||
    status === "BadlyPoisoned"
  ) {
    return 1.5;
  }
  return 1;
}

export interface CaptureParams {
  maxHP: number;
  currentHP: number;
  catchRate: number; // 3–255
  ball: BallType;
  status: StatusCondition | null;
  rng: () => number;
}

export interface CaptureResult {
  caught: boolean;
  shakes: number; // 0–4 successful shake checks
  /** The 'a' parameter (>= 255 means guaranteed catch). */
  a: number;
  /** The 'b' threshold for shake checks (undefined when a >= 255). */
  b: number | null;
}

/**
 * Resolve a capture attempt. Returns whether the monster was caught plus how
 * many of the 4 shake checks succeeded (mirroring the anime/game shakes).
 */
export function calculateCapture(
  params: CaptureParams,
): CaptureResult {
  const { maxHP, currentHP, catchRate, ball, status, rng } = params;
  const statusMod = getStatusMod(status);
  const ballMod = BALL_MODIFIERS[ball];

  const a =
    ((3 * maxHP - 2 * currentHP) * catchRate * ballMod * statusMod) /
    (3 * maxHP);

  // Guaranteed catch when the formula saturates.
  if (a >= 255) {
    return { caught: true, shakes: 4, a, b: null };
  }

  const b = 1048560 / Math.sqrt(Math.sqrt(16711680 / a));
  let shakes = 0;
  for (let i = 0; i < 4; i += 1) {
    if (rollInt(rng, 0, 65535) < b) {
      shakes += 1;
    }
  }

  return { caught: shakes === 4, shakes, a, b };
}

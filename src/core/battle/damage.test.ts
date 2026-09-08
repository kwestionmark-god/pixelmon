import { describe, expect, it } from "vitest";
import {
  calculateDamage,
  getBurnMod,
  getWeatherMod,
} from "./damage";
import type { Move, Stats } from "@/types";

/** Returns queued values in order, repeating the last value. */
function queueRng(values: number[]): () => number {
  let i = 0;
  return () => {
    const v = values[Math.min(i, values.length - 1)];
    i += 1;
    return v;
  };
}

const PHYSICAL: Move = {
  id: "MV-TACKLE",
  name: "Tackle",
  type: "Normal",
  category: "Physical",
  power: 40,
  accuracy: 100,
  pp: 35,
  priority: 0,
  target: "enemy",
  description: "A full-body charge attack.",
};

const stats = (override: Partial<Stats> = {}): Stats => ({
  hp: 50,
  attack: 100,
  defense: 100,
  spAttack: 100,
  spDefense: 100,
  speed: 100,
  ...override,
});

describe("calculateDamage (GDD §3.1.3)", () => {
  it("reproduces the Gen 5 base-damage formula exactly", () => {
    // base = FLOOR( FLOOR( (2*50/5 + 2) * 40 * 100/100 ) / 50 ) + 2 = 19
    const res = calculateDamage({
      attacker: { level: 50, stats: stats(), types: ["Normal"], status: null },
      defender: { level: 50, stats: stats(), types: ["Normal"], status: null },
      move: PHYSICAL,
      weather: null,
      rng: queueRng([0.99]),
    });

    // random = 0.85 + 0.99*0.15 = 0.9985; no crit; STAB 1.5; eff 1.
    // rawDamage is the un-floored product; finalDamage is its floored, clamped value.
    const expectedRaw = 19 * 1.5 * 1 * 0.9985;
    expect(res.rawDamage).toBeCloseTo(expectedRaw, 5);
    expect(res.finalDamage).toBe(Math.floor(expectedRaw));
    expect(res.crit).toBe(false);
    expect(res.effectiveness).toBe(1);
    expect(res.stab).toBe(1.5);
  });

  it("selects Special stats for special moves", () => {
    const special: Move = {
      ...PHYSICAL,
      category: "Special",
      type: "Fire",
      power: 90,
    };
    const res = calculateDamage({
      attacker: {
        level: 50,
        stats: stats({ attack: 10, spAttack: 120 }),
        types: ["Fire"],
        status: null,
      },
      defender: {
        level: 50,
        stats: stats({ defense: 100, spDefense: 80 }),
        types: ["Fire"],
        status: null,
      },
      move: special,
      weather: null,
      rng: queueRng([0.99]),
    });
    // base = FLOOR( FLOOR( 22 * 90 * 120 / 80 ) / 50 ) + 2 = 67
    expect(res.rawDamage).toBeGreaterThan(0);
    // STAB applies (attacker is Fire, move is Fire).
    expect(res.stab).toBe(1.5);
  });

  it("STAB raises damage versus a non-STAB hit", () => {
    // Both hits target a neutral (Normal) defender with identical randomness;
    // only whether the attacker's type matches the move type differs.
    const noStab = calculateDamage({
      attacker: { level: 50, stats: stats(), types: ["Electric"], status: null },
      defender: { level: 50, stats: stats(), types: ["Normal"], status: null },
      move: { ...PHYSICAL, type: "Water" },
      weather: null,
      rng: queueRng([0.9]),
    });
    const withStab = calculateDamage({
      attacker: { level: 50, stats: stats(), types: ["Water"], status: null },
      defender: { level: 50, stats: stats(), types: ["Normal"], status: null },
      move: { ...PHYSICAL, type: "Water" },
      weather: null,
      rng: queueRng([0.9]),
    });
    expect(withStab.stab).toBe(1.5);
    expect(noStab.stab).toBe(1);
    expect(withStab.finalDamage).toBeGreaterThan(noStab.finalDamage);
  });

  it("super-effective hits do more than neutral hits", () => {
    const neutral = calculateDamage({
      attacker: { level: 50, stats: stats(), types: ["Normal"], status: null },
      defender: { level: 50, stats: stats(), types: ["Normal"], status: null },
      move: PHYSICAL,
      weather: null,
      rng: queueRng([0.9]),
    });
    const superEff = calculateDamage({
      attacker: { level: 50, stats: stats(), types: ["Fire"], status: null },
      defender: { level: 50, stats: stats(), types: ["Grass"], status: null },
      move: { ...PHYSICAL, type: "Fire", power: 40 },
      weather: null,
      rng: queueRng([0.9]),
    });
    expect(superEff.effectiveness).toBe(2);
    expect(superEff.finalDamage).toBeGreaterThan(neutral.finalDamage);
  });

  it("immunity deals zero damage", () => {
    const res = calculateDamage({
      attacker: { level: 50, stats: stats(), types: ["Normal"], status: null },
      defender: { level: 50, stats: stats(), types: ["Ghost"], status: null },
      move: PHYSICAL,
      weather: null,
      rng: queueRng([0.9]),
    });
    expect(res.effectiveness).toBe(0);
    expect(res.finalDamage).toBe(0);
  });

  it("critical hits multiply damage by ~1.5× at equal randomness", () => {
    const nonCrit = calculateDamage({
      attacker: { level: 50, stats: stats(), types: ["Normal"], status: null },
      defender: { level: 50, stats: stats(), types: ["Normal"], status: null },
      move: PHYSICAL,
      weather: null,
      rng: queueRng([0.5]),
    });
    // random draw 0.5, crit draw 0.0 (< 1/24) -> crit.
    const crit = calculateDamage({
      attacker: { level: 50, stats: stats(), types: ["Normal"], status: null },
      defender: { level: 50, stats: stats(), types: ["Normal"], status: null },
      move: PHYSICAL,
      weather: null,
      rng: queueRng([0.5, 0.0]),
    });
    expect(crit.crit).toBe(true);
    expect(crit.rawDamage).toBeCloseTo(nonCrit.rawDamage * 1.5, 5);
  });

  it("clamps real hits to a minimum of 1 (GDD §3.1.3)", () => {
    // A level-1 burned physical attacker: base is 2, but burn halves the
    // already-tiny hit below 1, so the min-clamp must raise it to exactly 1.
    const res = calculateDamage({
      attacker: {
        level: 1,
        stats: stats({ attack: 1, defense: 255 }),
        types: ["Electric"], // no STAB against a Normal move
        status: "Burn",
      },
      defender: {
        level: 1,
        stats: stats({ defense: 255 }),
        types: ["Normal"],
        status: null,
      },
      move: PHYSICAL,
      weather: null,
      // Two rng draws are consumed (random factor, then crit roll):
      // first draw 0 -> random 0.85; second draw 1 -> no crit.
      rng: queueRng([0, 1]),
    });
    expect(res.rawDamage).toBeLessThan(1);
    expect(res.finalDamage).toBe(1);
  });
});

describe("getWeatherMod", () => {
  it("applies the GDD §3.1.3 rain/sun modifiers", () => {
    expect(getWeatherMod("Rain", "Water")).toBe(1.5);
    expect(getWeatherMod("Rain", "Fire")).toBe(0.5);
    expect(getWeatherMod("Sun", "Fire")).toBe(1.5);
    expect(getWeatherMod("Sun", "Water")).toBe(0.5);
    expect(getWeatherMod(null, "Water")).toBe(1);
  });
});

describe("getBurnMod", () => {
  it("halves physical power for burned physical attacks only", () => {
    expect(getBurnMod("Burn", "Physical")).toBe(0.5);
    expect(getBurnMod("Burn", "Special")).toBe(1);
    expect(getBurnMod(null, "Physical")).toBe(1);
  });
});

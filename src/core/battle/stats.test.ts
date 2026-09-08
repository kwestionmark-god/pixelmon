import { describe, expect, it } from "vitest";
import { calculateStats } from "./stats";
import type { Stats, EVs } from "@/types";

const BASE_STATS: Stats = {
  hp: 100,
  attack: 120,
  defense: 80,
  spAttack: 90,
  spDefense: 70,
  speed: 60,
};

function ivs(override: Partial<EVs> = {}): EVs {
  return { hp: 31, attack: 31, defense: 31, spAttack: 31, spDefense: 31, speed: 31, ...override };
}

function evs(override: Partial<EVs> = {}): EVs {
  return { hp: 0, attack: 0, defense: 0, spAttack: 0, spDefense: 0, speed: 0, ...override };
}

describe("calculateStats (GDD §3.4)", () => {
  it("computes HP with level+10 offset (no nature modifier)", () => {
    // HP = floor((2*100 + 31 + floor(0/4)) * 50 / 100) + 50 + 10 = floor(231 * 0.5) + 60 = 115 + 60 = 175
    const res = calculateStats({
      baseStats: BASE_STATS,
      ivs: ivs(),
      evs: evs(),
      level: 50,
      nature: "Hardy",
    });
    expect(res.hp).toBe(175);
  });

  it("adds +5 before nature for non-HP stats", () => {
    // attack: floor((2*120 + 31 + 0) * 50 / 100) + 5 = floor(271 * 0.5) + 5 = 135 + 5 = 140
    const res = calculateStats({
      baseStats: BASE_STATS,
      ivs: ivs(),
      evs: evs(),
      level: 50,
      nature: "Hardy",
    });
    expect(res.attack).toBe(140);
  });

  it("applies +10% nature modifier (floored)", () => {
    // Lonely: attack +10%, defense -10%
    // attack base 140 -> floor(140 * 1.1) = 154
    // defense base: floor((2*80 + 31) * 50 / 100) + 5 = floor(191 * 0.5) + 5 = 95 + 5 = 100
    // defense with -10% -> floor(100 * 0.9) = 90
    const res = calculateStats({
      baseStats: BASE_STATS,
      ivs: ivs(),
      evs: evs(),
      level: 50,
      nature: "Lonely",
    });
    expect(res.attack).toBe(154);
    expect(res.defense).toBe(90);
  });

  it("applies +10% and -10% nature modifiers correctly", () => {
    const hardy = calculateStats({
      baseStats: BASE_STATS,
      ivs: ivs(),
      evs: evs(),
      level: 50,
      nature: "Hardy",
    });
    const adamant = calculateStats({
      baseStats: BASE_STATS,
      ivs: ivs(),
      evs: evs(),
      level: 50,
      nature: "Adamant", // +attack, -spAttack
    });
    // attack should be 10% higher (floored): floor(140 * 1.1) = 154
    expect(adamant.attack).toBeGreaterThan(hardy.attack);
    // spAttack should be 10% lower (floored): floor(110 * 0.9) = 99
    expect(adamant.spAttack).toBeLessThan(hardy.spAttack);
  });

  it("includes EVs in the calculation (scaled by level/100)", () => {
    // 252 EVs -> floor(252/4) = 63 stat points at level 100.
    // At level 50 the inner sum grows by 63, which adds floor(63 * 50/100) = 31
    // before the +5 cap, so the visible difference is 32.
    const withEvs = calculateStats({
      baseStats: BASE_STATS,
      ivs: ivs(),
      evs: evs({ attack: 252 }),
      level: 50,
      nature: "Hardy",
    });
    const withoutEvs = calculateStats({
      baseStats: BASE_STATS,
      ivs: ivs(),
      evs: evs({ attack: 0 }),
      level: 50,
      nature: "Hardy",
    });
    expect(withEvs.attack - withoutEvs.attack).toBe(32);
  });

  it("defaults missing IV/EV entries to 0", () => {
    const res = calculateStats({
      baseStats: BASE_STATS,
      ivs: {},
      evs: {},
      level: 50,
      nature: "Hardy",
    });
    // 2*100 + 0 + 0 = 200 -> floor(200 * 50/100) + 60 = 100 + 60 = 160 HP
    expect(res.hp).toBe(160);
  });

  it("Quirky and Hardy are neutral (no stat changes)", () => {
    const q = calculateStats({ baseStats: BASE_STATS, ivs: ivs(), evs: evs(), level: 50, nature: "Quirky" });
    const h = calculateStats({ baseStats: BASE_STATS, ivs: ivs(), evs: evs(), level: 50, nature: "Hardy" });
    expect(q).toEqual(h);
  });
});
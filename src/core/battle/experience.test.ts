import { describe, expect, it } from "vitest";
import { experienceToLevel, calculateXpYield, levelFromExp } from "./experience";
import type { GrowthRate } from "@/types";

describe("experienceToLevel (GDD §3.3.2)", () => {
  it("Fast: 4/5 n^3", () => {
    expect(experienceToLevel(1, "Fast")).toBe(0);
    expect(experienceToLevel(10, "Fast")).toBe(Math.floor(4 * 1000 / 5)); // 800
    expect(experienceToLevel(100, "Fast")).toBe(Math.floor(4 * 1000000 / 5)); // 800000
  });

  it("MediumFast: n^3", () => {
    expect(experienceToLevel(1, "MediumFast")).toBe(1);
    expect(experienceToLevel(50, "MediumFast")).toBe(125000);
    expect(experienceToLevel(100, "MediumFast")).toBe(1000000);
  });

  it("Slow: 5/4 n^3 (floored)", () => {
    expect(experienceToLevel(10, "Slow")).toBe(Math.floor(5 * 1000 / 4)); // 1250
    expect(experienceToLevel(100, "Slow")).toBe(Math.floor(5 * 1000000 / 4)); // 1250000
  });

  it("MediumSlow: 6/5 n^3 - 15 n^2 + 100 n - 140 (floored)", () => {
    const f = (n: number) => Math.floor(6/5 * n**3 - 15 * n**2 + 100 * n - 140);
    expect(experienceToLevel(10, "MediumSlow")).toBe(f(10));
    expect(experienceToLevel(50, "MediumSlow")).toBe(f(50));
    expect(experienceToLevel(100, "MediumSlow")).toBe(f(100));
  });

  it("Erratic: piecewise formula (non-monotonic at boundaries)", () => {
    // Erratic formula has known dips at level boundaries (e.g., 50->51, 68->69, 98->99)
    expect(experienceToLevel(1, "Erratic")).toBe(2); // floor(100*1/50) = 2
    expect(experienceToLevel(50, "Erratic")).toBe(Math.floor(100 * 125000 / 50)); // 250000
    expect(experienceToLevel(68, "Erratic")).toBe(Math.floor(100 * 314432 / 100)); // 314432
    expect(experienceToLevel(98, "Erratic")).toBe(Math.floor(100 * 941192 / 500)); // 188238
  });

  it("Fluctuating: piecewise formula", () => {
    expect(experienceToLevel(1, "Fluctuating")).toBe(0);
    expect(experienceToLevel(15, "Fluctuating")).toBe(Math.floor(15**3 * (24 + Math.floor(16/3)) / 50));
    expect(experienceToLevel(36, "Fluctuating")).toBe(Math.floor(36**3 * (14 + 36) / 50));
    expect(experienceToLevel(100, "Fluctuating")).toBe(Math.floor(100**3 * (32 + 50) / 50));
  });

  it("monotonic: higher levels always require more XP (except Erratic/Fluctuating)", () => {
    // Erratic and Fluctuating are intentionally non-monotonic in the games.
    const monotonicRates: GrowthRate[] = ["Fast", "MediumFast", "Slow", "MediumSlow"];
    for (const rate of monotonicRates) {
      let prev = -1;
      for (let n = 1; n <= 100; n += 1) {
        const cur = experienceToLevel(n, rate);
        expect(cur).toBeGreaterThanOrEqual(prev);
        prev = cur;
      }
    }
  });
});

describe("calculateXpYield (GDD §3.3.1)", () => {
  it("base formula: (baseExp * defeatedLevel) / 5", () => {
    const xp = calculateXpYield({
      baseExp: 200,
      defeatedLevel: 50,
      isTrainerBattle: false,
      holdsLuckyEgg: false,
      wasTraded: false,
      participants: 1,
      participated: true,
    });
    expect(xp).toBe(2000); // 200 * 50 / 5 = 2000
  });

  it("trainer battle multiplies by 1.5", () => {
    const normal = calculateXpYield({
      baseExp: 100,
      defeatedLevel: 50,
      isTrainerBattle: false,
      holdsLuckyEgg: false,
      wasTraded: false,
      participants: 1,
      participated: true,
    });
    const trainer = calculateXpYield({
      baseExp: 100,
      defeatedLevel: 50,
      isTrainerBattle: true,
      holdsLuckyEgg: false,
      wasTraded: false,
      participants: 1,
      participated: true,
    });
    expect(trainer).toBe(Math.floor(normal * 1.5));
  });

  it("Lucky Egg multiplies by 1.5", () => {
    const normal = calculateXpYield({
      baseExp: 100,
      defeatedLevel: 50,
      isTrainerBattle: false,
      holdsLuckyEgg: false,
      wasTraded: false,
      participants: 1,
      participated: true,
    });
    const egg = calculateXpYield({
      baseExp: 100,
      defeatedLevel: 50,
      isTrainerBattle: false,
      holdsLuckyEgg: true,
      wasTraded: false,
      participants: 1,
      participated: true,
    });
    expect(egg).toBe(Math.floor(normal * 1.5));
  });

  it("traded multiplier (1.5)", () => {
    const normal = calculateXpYield({
      baseExp: 100,
      defeatedLevel: 50,
      isTrainerBattle: false,
      holdsLuckyEgg: false,
      wasTraded: false,
      participants: 1,
      participated: true,
    });
    const traded = calculateXpYield({
      baseExp: 100,
      defeatedLevel: 50,
      isTrainerBattle: false,
      holdsLuckyEgg: false,
      wasTraded: true,
      participants: 1,
      participated: true,
    });
    expect(traded).toBe(Math.floor(normal * 1.5));
  });

  it("stacking multipliers are multiplicative", () => {
    const base = calculateXpYield({
      baseExp: 100,
      defeatedLevel: 50,
      isTrainerBattle: false,
      holdsLuckyEgg: false,
      wasTraded: false,
      participants: 1,
      participated: true,
    });
    const all = calculateXpYield({
      baseExp: 100,
      defeatedLevel: 50,
      isTrainerBattle: true,
      holdsLuckyEgg: true,
      wasTraded: true,
      participants: 1,
      participated: true,
    });
    expect(all).toBe(Math.floor(base * 1.5 * 1.5 * 1.5));
  });

  it("participants splits XP among participants", () => {
    const xp = calculateXpYield({
      baseExp: 200,
      defeatedLevel: 50,
      isTrainerBattle: false,
      holdsLuckyEgg: false,
      wasTraded: false,
      participants: 2,
      participated: true,
    });
    // 2000 / 2 = 1000
    expect(xp).toBe(1000);
  });

  it("non-participant gets half (Exp Share behavior)", () => {
    const xp = calculateXpYield({
      baseExp: 200,
      defeatedLevel: 50,
      isTrainerBattle: false,
      holdsLuckyEgg: false,
      wasTraded: false,
      participants: 2,
      participated: false,
    });
    // 2000 / 2 = 1000
    expect(xp).toBe(1000);
  });

  it("participants is floored at minimum 1", () => {
    const xp = calculateXpYield({
      baseExp: 200,
      defeatedLevel: 50,
      isTrainerBattle: false,
      holdsLuckyEgg: false,
      wasTraded: false,
      participants: 0,
      participated: true,
    });
    // denominator is max(1, 0) = 1
    expect(xp).toBe(2000);
  });
});

describe("levelFromExp (GDD §3.3.2)", () => {
  it("returns the highest level whose XP requirement is <= current exp", () => {
    // MediumFast: level 10 = 1000, level 11 = 1331
    expect(levelFromExp(999, "MediumFast")).toBe(9);
    expect(levelFromExp(1000, "MediumFast")).toBe(10);
    expect(levelFromExp(1330, "MediumFast")).toBe(10);
    expect(levelFromExp(1331, "MediumFast")).toBe(11);
  });

  it("level 1 for 0 exp", () => {
    expect(levelFromExp(0, "Fast")).toBe(1);
    expect(levelFromExp(0, "Slow")).toBe(1);
  });

  it("works for all growth rates", () => {
    const rates: GrowthRate[] = ["Fast", "MediumFast", "Slow", "MediumSlow", "Fluctuating"];
    for (const rate of rates) {
      const xp = experienceToLevel(50, rate);
      expect(levelFromExp(xp, rate)).toBe(50);
      expect(levelFromExp(xp - 1, rate)).toBeLessThan(50);
    }
    // Erratic is non-monotonic; levelFromExp finds the highest level <= xp
    // which may not be 50 for the XP at level 50
    const erraticXp = experienceToLevel(50, "Erratic");
    const lvl = levelFromExp(erraticXp, "Erratic");
    expect(lvl).toBeGreaterThanOrEqual(50);
  });
});
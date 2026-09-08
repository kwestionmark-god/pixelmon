import { describe, expect, it } from "vitest";
import {
  getEffectiveness,
  interpretEffectiveness,
  typeChart,
} from "./typeChart";
import type { Type } from "@/types";

describe("typeChart", () => {
  it("contains a 18×18 square matrix", () => {
    for (const attacker of Object.keys(typeChart) as Type[]) {
      expect(Object.keys(typeChart[attacker]).length).toBe(18);
      for (const defender of Object.keys(typeChart[attacker]) as Type[]) {
        const value = typeChart[attacker][defender];
        expect([0, 0.5, 1, 2]).toContain(value);
      }
    }
  });

  it("matches the canonical super-effective pairs (GDD §3.1.4)", () => {
    expect(typeChart.Fire.Grass).toBe(2);
    expect(typeChart.Water.Fire).toBe(2);
    expect(typeChart.Electric.Water).toBe(2);
    expect(typeChart.Grass.Water).toBe(2);
  });

  it("matches the canonical immune pairs (GDD §3.1.4)", () => {
    expect(typeChart.Electric.Ground).toBe(0);
    expect(typeChart.Ghost.Normal).toBe(0);
    expect(typeChart.Normal.Ghost).toBe(0);
    expect(typeChart.Ground.Flying).toBe(0);
  });

  it("matches a canonical resisted pair", () => {
    expect(typeChart.Fire.Fire).toBe(0.5);
  });
});

describe("getEffectiveness", () => {
  it("multiplies across dual types", () => {
    // Fire against a Fire/Water dual: 0.5 * 2 = 1.
    expect(getEffectiveness("Fire", ["Fire", "Water"])).toBe(1);
  });

  it("returns 0 when any type is immune", () => {
    expect(getEffectiveness("Electric", ["Ground", "Water"])).toBe(0);
  });

  it("defaults to 1 for a neutral single type", () => {
    expect(getEffectiveness("Normal", ["Normal"])).toBe(1);
  });
});

describe("interpretEffectiveness", () => {
  it("labels each multiplier tier", () => {
    expect(interpretEffectiveness(2)).toBe("super effective");
    expect(interpretEffectiveness(0.5)).toBe("not very effective");
    expect(interpretEffectiveness(0)).toBe("no effect");
    expect(interpretEffectiveness(1)).toBe("normal");
  });
});

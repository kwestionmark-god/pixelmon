import { describe, expect, it } from "vitest";
import { checkEvolution } from "./evolution";
import type { Evolution } from "@/types";

const knowsMove = (moveId: string) => moveId === "MV-EVOLVE";

const baseCtx = {
  level: 30,
  friendship: 100,
  timeOfDay: "day" as const,
  knowsMove,
  itemApplied: undefined,
  traded: false,
  location: undefined,
};

describe("checkEvolution (GDD §3.5)", () => {
  it("returns null when no evolutions defined", () => {
    expect(checkEvolution(undefined, baseCtx)).toBeNull();
    expect(checkEvolution([], baseCtx)).toBeNull();
  });

  it("level evolution triggers when level >= requirement", () => {
    const evos: Evolution[] = [
      { to: "PM-002", method: "level", requirement: 20 },
      { to: "PM-003", method: "level", requirement: 40 },
    ];
    expect(checkEvolution(evos, { ...baseCtx, level: 20 })).toBe("PM-002");
    expect(checkEvolution(evos, { ...baseCtx, level: 30 })).toBe("PM-002"); // first matching
    expect(checkEvolution(evos, { ...baseCtx, level: 19 })).toBeNull();
  });

  it("stone evolution triggers when itemApplied matches requirement", () => {
    const evos: Evolution[] = [
      { to: "PM-002", method: "stone", requirement: "ITEM-FIRE_STONE" },
    ];
    expect(checkEvolution(evos, { ...baseCtx, itemApplied: "ITEM-FIRE_STONE" })).toBe("PM-002");
    expect(checkEvolution(evos, { ...baseCtx, itemApplied: "ITEM-WATER_STONE" })).toBeNull();
    expect(checkEvolution(evos, baseCtx)).toBeNull();
  });

  it("trade evolution triggers when traded=true", () => {
    const evos: Evolution[] = [
      { to: "PM-002", method: "trade", requirement: 0 },
    ];
    expect(checkEvolution(evos, { ...baseCtx, traded: true })).toBe("PM-002");
    expect(checkEvolution(evos, { ...baseCtx, traded: false })).toBeNull();
  });

  it("friendship evolution triggers when friendship >= requirement", () => {
    const evos: Evolution[] = [
      { to: "PM-002", method: "friendship", requirement: 220 },
    ];
    expect(checkEvolution(evos, { ...baseCtx, friendship: 220 })).toBe("PM-002");
    expect(checkEvolution(evos, { ...baseCtx, friendship: 255 })).toBe("PM-002");
    expect(checkEvolution(evos, { ...baseCtx, friendship: 219 })).toBeNull();
  });

  it("time evolution triggers when level >= requirement AND timeOfDay matches", () => {
    const evos: Evolution[] = [
      { to: "PM-002", method: "time", requirement: 30, condition: "night" },
    ];
    expect(checkEvolution(evos, { ...baseCtx, level: 30, timeOfDay: "night" })).toBe("PM-002");
    expect(checkEvolution(evos, { ...baseCtx, level: 30, timeOfDay: "day" })).toBeNull();
    expect(checkEvolution(evos, { ...baseCtx, level: 29, timeOfDay: "night" })).toBeNull();
  });

  it("move evolution triggers when knowsMove returns true for requirement", () => {
    const evos: Evolution[] = [
      { to: "PM-002", method: "move", requirement: "MV-EVOLVE" },
    ];
    expect(checkEvolution(evos, { ...baseCtx })).toBe("PM-002");
    // knowsMove returns false for other moves
    expect(checkEvolution(evos, { ...baseCtx, knowsMove: () => false })).toBeNull();
  });

  it("location evolution triggers when location matches requirement", () => {
    const evos: Evolution[] = [
      { to: "PM-002", method: "location", requirement: "LOC-MT_CORONET" },
    ];
    expect(checkEvolution(evos, { ...baseCtx, location: "LOC-MT_CORONET" })).toBe("PM-002");
    expect(checkEvolution(evos, { ...baseCtx, location: "LOC-ROUTE_1" })).toBeNull();
    expect(checkEvolution(evos, baseCtx)).toBeNull();
  });

  it("returns first matching evolution in list order", () => {
    const evos: Evolution[] = [
      { to: "PM-FIRST", method: "level", requirement: 10 },
      { to: "PM-SECOND", method: "level", requirement: 5 },
    ];
    // Level 30 meets both, but first in list wins
    expect(checkEvolution(evos, { ...baseCtx, level: 30 })).toBe("PM-FIRST");
  });

  it("ignores unknown method strings", () => {
    const evos: Evolution[] = [
      { to: "PM-002", method: "unknown" as any, requirement: 10 },
      { to: "PM-003", method: "level", requirement: 20 },
    ];
    expect(checkEvolution(evos, { ...baseCtx, level: 30 })).toBe("PM-003");
  });
});
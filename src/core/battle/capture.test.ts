import { describe, expect, it } from "vitest";
import { calculateCapture, getStatusMod, BALL_MODIFIERS } from "./capture";

function queueRng(values: number[]): () => number {
  let i = 0;
  return () => values[Math.min(i++, values.length - 1)];
}

describe("getStatusMod", () => {
  it("returns 2 for Sleep and Freeze", () => {
    expect(getStatusMod("Sleep")).toBe(2);
    expect(getStatusMod("Freeze")).toBe(2);
  });

  it("returns 1.5 for Paralysis, Poison, Burn, BadlyPoisoned", () => {
    expect(getStatusMod("Paralysis")).toBe(1.5);
    expect(getStatusMod("Poison")).toBe(1.5);
    expect(getStatusMod("Burn")).toBe(1.5);
    expect(getStatusMod("BadlyPoisoned")).toBe(1.5);
  });

  it("returns 1 for no status or Confusion/Flinch", () => {
    expect(getStatusMod(null)).toBe(1);
    expect(getStatusMod("Confusion")).toBe(1);
    expect(getStatusMod("Flinch")).toBe(1);
  });
});

describe("BALL_MODIFIERS", () => {
  it("has expected values per GDD §3.2.1", () => {
    expect(BALL_MODIFIERS.Poke).toBe(1);
    expect(BALL_MODIFIERS.Great).toBe(1.5);
    expect(BALL_MODIFIERS.Ultra).toBe(2);
    expect(BALL_MODIFIERS.Master).toBe(255);
  });
});

describe("calculateCapture (GDD §3.2.1)", () => {
  it("returns caught=true and shakes=4 when a >= 255 (Master Ball / high catch rate)", () => {
    // Master Ball has 255x modifier -> a >= 255 always
    const res = calculateCapture({
      maxHP: 100,
      currentHP: 100,
      catchRate: 3,
      ball: "Master",
      status: null,
      rng: queueRng([0]),
    });
    expect(res.caught).toBe(true);
    expect(res.shakes).toBe(4);
    expect(res.a).toBeGreaterThanOrEqual(255);
    expect(res.b).toBeNull();
  });

  it("instant catch with high catch rate + full HP + Ultra Ball", () => {
    // a = ((3*100 - 2*100) * 255 * 2) / 300 = (100 * 510) / 300 = 170 -> not enough
    // With status:
    const res = calculateCapture({
      maxHP: 100,
      currentHP: 1,
      catchRate: 255,
      ball: "Ultra",
      status: "Sleep",
      rng: queueRng([0]),
    });
    // a = ((300-2) * 255 * 2 * 2) / 300 = 298 * 1020 / 300 = 1013.2 -> >= 255
    expect(res.caught).toBe(true);
    expect(res.shakes).toBe(4);
  });

  it("four shake checks when a < 255; caught only when all 4 succeed", () => {
    // Low catch rate, low HP, no status: b will be small
    const res = calculateCapture({
      maxHP: 100,
      currentHP: 50,
      catchRate: 45, // standard wild
      ball: "Poke",
      status: null,
      rng: queueRng([0, 0, 0, 0]), // all below any positive b
    });
    // With these params: a = ((300-100)*45*1) / 300 = 200*45/300 = 30
    // b = 1048560 / sqrt(sqrt(16711680/30)) = 1048560 / sqrt(sqrt(557056)) = 1048560 / sqrt(746.36) = 1048560 / 27.32 = 38378
    // max rand is 65535, so b/65535 ~ 58% success rate
    // rng draws all 0 -> all succeed
    expect(res.shakes).toBe(4);
    expect(res.caught).toBe(true);
  });

  it("shake failures accumulate; caught=false if any fails", () => {
    // Same setup, but rng draws exceed b on the third check
    // loop runs 4 times: [0, 0, 65535, 0] -> passes, passes, fails, passes = 3 successes
    const res = calculateCapture({
      maxHP: 100,
      currentHP: 50,
      catchRate: 45,
      ball: "Poke",
      status: null,
      rng: queueRng([0, 0, 65535, 0]),
    });
    expect(res.shakes).toBe(3); // three of four succeeded
    expect(res.caught).toBe(false); // but not all 4
  });

  it("lower current HP increases catch probability (higher a)", () => {
    const fullHP = calculateCapture({
      maxHP: 100,
      currentHP: 100,
      catchRate: 45,
      ball: "Poke",
      status: null,
      rng: queueRng([0, 0, 0, 0]),
    });
    const lowHP = calculateCapture({
      maxHP: 100,
      currentHP: 1,
      catchRate: 45,
      ball: "Poke",
      status: null,
      rng: queueRng([0, 0, 0, 0]),
    });
    expect(lowHP.a).toBeGreaterThan(fullHP.a);
  });

  it("status increases catch probability", () => {
    const normal = calculateCapture({
      maxHP: 100,
      currentHP: 50,
      catchRate: 45,
      ball: "Poke",
      status: null,
      rng: queueRng([0, 0, 0, 0]),
    });
    const asleep = calculateCapture({
      maxHP: 100,
      currentHP: 50,
      catchRate: 45,
      ball: "Poke",
      status: "Sleep",
      rng: queueRng([0, 0, 0, 0]),
    });
    expect(asleep.a).toBeGreaterThan(normal.a);
  });

  it("ball multiplier affects a", () => {
    const poke = calculateCapture({
      maxHP: 100,
      currentHP: 50,
      catchRate: 45,
      ball: "Poke",
      status: null,
      rng: queueRng([0, 0, 0, 0]),
    });
    const ultra = calculateCapture({
      maxHP: 100,
      currentHP: 50,
      catchRate: 45,
      ball: "Ultra",
      status: null,
      rng: queueRng([0, 0, 0, 0]),
    });
    expect(ultra.a).toBe(poke.a * 2);
  });

  it("returns correct a and b values for verification", () => {
    const res = calculateCapture({
      maxHP: 100,
      currentHP: 50,
      catchRate: 45,
      ball: "Poke",
      status: null,
      rng: queueRng([0]),
    });
    // a = 30 exactly
    expect(res.a).toBeCloseTo(30, 1);
    // b = 1048560 / sqrt(sqrt(16711680/30))
    const expectedB = 1048560 / Math.sqrt(Math.sqrt(16711680 / 30));
    expect(res.b).toBeCloseTo(expectedB, 1);
  });
});
import { describe, expect, it } from "vitest";
import { rollBelow, rollInt, seededRandom } from "./rng";

/** Returns queued values in order, repeating the last value. */
function queueRng(values: number[]): () => number {
  let i = 0;
  return () => {
    const v = values[Math.min(i, values.length - 1)];
    i += 1;
    return v;
  };
}

describe("seededRandom (mulberry32)", () => {
  it("produces a deterministic sequence for a given seed", () => {
    const a = seededRandom(42);
    const b = seededRandom(42);
    const seqA = Array.from({ length: 100 }, () => a());
    const seqB = Array.from({ length: 100 }, () => b());
    expect(seqA).toEqual(seqB);
  });

  it("always yields values in [0, 1)", () => {
    const rng = seededRandom(12345);
    for (let i = 0; i < 1000; i += 1) {
      const v = rng();
      expect(v).toBeGreaterThanOrEqual(0);
      expect(v).toBeLessThan(1);
    }
  });

  it("produces different sequences for different seeds", () => {
    const a = seededRandom(1);
    const b = seededRandom(2);
    const seqA = Array.from({ length: 50 }, () => a());
    const seqB = Array.from({ length: 50 }, () => b());
    expect(seqA).not.toEqual(seqB);
  });
});

describe("rollInt", () => {
  it("returns the inclusive lower bound when rng is 0", () => {
    expect(rollInt(queueRng([0]), 5, 20)).toBe(5);
  });

  it("returns the inclusive upper bound when rng approaches 1", () => {
    // floor(0.999999 * (20 - 5 + 1)) = floor(14.999...) = 15? no: max=20
    expect(rollInt(queueRng([0.999999]), 5, 20)).toBe(20);
  });

  it("never draws outside [min, max] across many rolls", () => {
    const rng = seededRandom(7);
    for (let i = 0; i < 5000; i += 1) {
      const v = rollInt(rng, 3, 9);
      expect(v).toBeGreaterThanOrEqual(3);
      expect(v).toBeLessThanOrEqual(9);
    }
  });

  it("throws when min > max", () => {
    expect(() => rollInt(queueRng([0]), 10, 1)).toThrow();
  });
});

describe("rollBelow", () => {
  it("is false for a zero threshold", () => {
    expect(rollBelow(queueRng([0.5]), 0)).toBe(false);
  });

  it("is true for a threshold of 1", () => {
    expect(rollBelow(queueRng([0.5]), 1)).toBe(true);
  });

  it("compares the draw against the threshold", () => {
    expect(rollBelow(queueRng([0]), 0.5)).toBe(true);
    expect(rollBelow(queueRng([0.9]), 0.5)).toBe(false);
  });
});

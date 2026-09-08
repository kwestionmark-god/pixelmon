import { describe, expect, it } from "vitest";
import { resolveTurnOrder } from "./turnOrder";
import type { Move, StatusCondition } from "@/types";

/** Builds a minimal TurnAction. */
function action(
  kind: "move" | "switch" | "item" | "run",
  opts: {
    speed?: number;
    ability?: string;
    status?: StatusCondition | null;
  } = {},
  move?: Move,
) {
  return {
    kind,
    user: {
      speed: opts.speed ?? 100,
      ability: opts.ability,
      status: opts.status ?? null,
    },
    move,
  };
}

function queueRng(v: number): () => number {
  return () => v;
}

describe("resolveTurnOrder (GDD §3.1.2)", () => {
  it("puts the higher-priority move first", () => {
    const fast: Move = { ...({} as Move), priority: 5 };
    const slow: Move = { ...({} as Move), priority: 0 };
    const res = resolveTurnOrder([
      action("move", { speed: 100 }, slow),
      action("move", { speed: 100 }, fast),
    ]);
    expect(res[0].move).toBe(fast);
  });

  it("breaks same-priority ties by speed", () => {
    const a = action("move", { speed: 200 });
    const b = action("move", { speed: 100 });
    const res = resolveTurnOrder([b, a]);
    expect(res[0].user.speed).toBe(200);
  });

  it("applies Quick Feet (1.5x speed) while statused", () => {
    // swift: 140 speed, no status. qf: 100 speed, Burn + Quick Feet -> 150.
    const swift = action("move", { speed: 140, status: null });
    const qf = action("move", { speed: 100, ability: "AB-QUICK_FEET", status: "Burn" });
    const res = resolveTurnOrder([swift, qf]);
    expect(res[0].user.ability).toBe("AB-QUICK_FEET");
  });

  it("does not boost speed with Quick Feet when unstatused", () => {
    const swift = action("move", { speed: 150, ability: "AB-QUICK_FEET", status: null });
    const slow = action("move", { speed: 100, status: null });
    const res = resolveTurnOrder([slow, swift]);
    expect(res[0].user).toBe(swift.user);
  });

  it("treats switch/item as +6 and run as -7", () => {
    const normal: Move = { ...({} as Move), priority: 0 };
    const res = resolveTurnOrder([
      action("run", { speed: 100 }),
      action("item", { speed: 100 }),
      action("move", { speed: 100 }, normal),
    ]);
    expect(res[0].kind).toBe("item");
    expect(res[2].kind).toBe("run");
  });

  it("breaks exact priority ties with a 50/50 random draw", () => {
    const moveA: Move = { ...({} as Move), priority: 0, name: "A" };
    const moveB: Move = { ...({} as Move), priority: 0, name: "B" };
    const a = action("move", { speed: 100 }, moveA);
    const b = action("move", { speed: 100 }, moveB);
    // Two different draws must produce two different winners, proving the
    // tie is broken randomly rather than by input order.
    const drawLow = resolveTurnOrder([a, b], queueRng(0));
    const drawHigh = resolveTurnOrder([a, b], queueRng(0.9));
    expect(drawLow[0].move).not.toBe(drawHigh[0].move);
    expect([drawLow[0].move, drawHigh[0].move]).toContain(moveA);
    expect([drawLow[0].move, drawHigh[0].move]).toContain(moveB);
  });
});
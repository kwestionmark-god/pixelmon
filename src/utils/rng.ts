/**
 * Deterministic pseudo-random number generator.
 *
 * The battle engine uses `Math.random()`-style calls throughout (crit rolls,
 * shake checks, encounter rolls). To keep tests reproducible we inject a seeded
 * PRNG (mulberry32) instead of relying on global randomness.
 *
 * @see https://gist.github.com/bryc/code/juriscache (mulberry32, public domain)
 */

/** Returns a function producing floats in [0, 1) from a 32-bit integer seed. */
export function seededRandom(seed: number): () => number {
  let a = seed >>> 0;
  return function next(): number {
    a |= 0;
    a = (a + 0x6d2b79f5) | 0;
    let t = Math.imul(a ^ (a >>> 15), 1 | a);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

/** Inclusive integer roll in [min, max] using the injected rng. */
export function rollInt(rng: () => number, min: number, max: number): number {
  const lo = Math.ceil(min);
  const hi = Math.floor(max);
  if (lo > hi) throw new Error(`rollInt: min(${min}) > max(${max})`);
  return lo + Math.floor(rng() * (hi - lo + 1));
}

/** Rolls `threshold` against a uniform [0, 1) draw. */
export function rollBelow(rng: () => number, threshold: number): boolean {
  return rng() < threshold;
}

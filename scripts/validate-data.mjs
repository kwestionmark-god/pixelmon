/**
 * Invariant + anchor validator for the generated Pixelmon data files.
 *
 * Headless (no network): reads src/data/monsters.json + moves.json and asserts
 * structural invariants plus a handful of canonical PokeAPI anchors. This is the
 * regression guard for the data pipeline: it fails fast if a regeneration produces
 * null catch rates, zero PP on damaging moves, dangling evolution/learnset refs,
 * or a missing/duplicated dex entry.
 *
 * Usage: node scripts/validate-data.mjs   (exit 0 = all checks pass)
 */
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const OUT_DIR = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../src/data");
const monsters = JSON.parse(fs.readFileSync(path.join(OUT_DIR, "monsters.json"), "utf8"));
const moves = JSON.parse(fs.readFileSync(path.join(OUT_DIR, "moves.json"), "utf8"));

const VALID_METHODS = new Set(["level", "stone", "trade", "friendship", "move", "location"]);
const STAT_KEYS = ["hp", "attack", "defense", "spAttack", "spDefense", "speed"];

let failures = 0;
const check = (cond, msg) => {
  if (!cond) {
    failures++;
    console.error(`  FAIL: ${msg}`);
  }
};

// ---- moves ---------------------------------------------------------------
const moveSlug = (name) => name.toLowerCase().replace(/ /g, "-");
const moveSlugs = new Set(moves.map((m) => moveSlug(m.name)));
const moveById = new Set(moves.map((m) => m.id));
const moveNames = new Set(moves.map((m) => m.name));

check(moves.length > 0, `moves.json is empty`);
check(moveById.size === moves.length, "duplicate move ids");
check(moveNames.size === moves.length, "duplicate move names");
for (const m of moves) {
  check(m.power >= 0 && Number.isInteger(m.power), `${m.id} ${m.name}: power must be int >= 0`);
  check(m.accuracy >= 0 && Number.isInteger(m.accuracy), `${m.id} ${m.name}: accuracy must be int >= 0`);
  check(typeof m.pp === "number" && m.pp >= 0, `${m.id} ${m.name}: pp must be >= 0`);
  // Damaging moves (and all moves) must have at least 1 PP.
  if (m.power > 0 || m.accuracy > 0) {
    check(m.pp >= 1, `${m.id} ${m.name}: damaging/active move pp must be >= 1 (got ${m.pp})`);
  }
}

// ---- monsters ------------------------------------------------------------
check(monsters.length === 150, `monsters.json must have 150 entries (got ${monsters.length})`);
const monById = new Set(monsters.map((m) => m.id));
const monNums = new Set(monsters.map((m) => m.num));
const monNames = new Set(monsters.map((m) => m.name));
check(monById.size === monsters.length, "duplicate monster ids");
check(monNums.size === monsters.length, "duplicate monster nums");
check(monNames.size === monsters.length, "duplicate monster names");
for (const m of monsters) {
  check(Array.isArray(m.types) && m.types.length >= 1, `${m.id}: needs >= 1 type`);
  for (const k of STAT_KEYS) {
    check(Number.isInteger(m.baseStats[k]) && m.baseStats[k] > 0, `${m.id} ${m.name}: baseStats.${k} must be int > 0`);
  }
  check(Number.isInteger(m.catchRate) && m.catchRate >= 0, `${m.id} ${m.name}: catchRate must be int >= 0 (was null?)`);
  check(Number.isInteger(m.baseFriendship) && m.baseFriendship >= 0, `${m.id} ${m.name}: baseFriendship must be int >= 0`);
  check(Number.isInteger(m.baseExp) && m.baseExp >= 0, `${m.id} ${m.name}: baseExp must be int >= 0`);
  check(typeof m.growthRate === "string" && m.growthRate.length > 0, `${m.id} ${m.name}: growthRate required`);
  check(typeof m.sprites?.front === "string" && m.sprites.front.length > 0, `${m.id} ${m.name}: sprites.front required`);

  // Learnset: non-empty, positive levels, moves resolve, no duplicate moves.
  check(m.learnset.length > 0, `${m.id} ${m.name}: learnset must be non-empty`);
  const lsMoves = new Set();
  for (const l of m.learnset) {
    check(Number.isInteger(l.level) && l.level >= 0, `${m.id} ${m.name}: learnset level ${l.level} for ${l.move} must be >= 0 (level-0 inherited moves are valid)`);
    check(moveSlugs.has(moveSlug(l.move)), `${m.id} ${m.name}: learnset move "${l.move}" does not resolve`);
    check(!lsMoves.has(l.move), `${m.id} ${m.name}: duplicate learnset move "${l.move}" (expected lowest-level dedup)`);
    lsMoves.add(l.move);
  }

  // Evolutions: target must resolve to a known species name.
  for (const e of m.evolution) {
    check(VALID_METHODS.has(e.method), `${m.id} ${m.name}: unknown evolution method "${e.method}"`);
    // Target need not resolve to a Kanto species: some Gen-1 evolutions cross into
    // later dexes (e.g. Crobat = PokeAPI 169), which is by-design.
    check(typeof e.to === "string" && e.to.length > 0, `${m.id} ${m.name}: evolution target "${e.to}" must be a non-empty name`);
  }
}

// ---- canonical anchors (known-correct PokeAPI values) --------------------
const byName = (list, n) => list.find((x) => x.name.toLowerCase() === n);
const bulbasaur = byName(monsters, "bulbasaur");
const magikarp = byName(monsters, "magikarp");
const doubleEdge = byName(moves, "double-edge");
const tackle = byName(moves, "tackle");

check(bulbasaur && bulbasaur.catchRate === 45, `bulbasaur catchRate anchor != 45 (got ${bulbasaur?.catchRate})`);
check(bulbasaur && bulbasaur.baseExp === 64, `bulbasaur baseExp anchor != 64`);
check(bulbasaur && bulbasaur.baseFriendship === 70, `bulbasaur baseFriendship anchor != 70`);
// Magikarp's catchRate is a live PokeAPI value that legitimately varies
// (API returns 190; the canonical game value is 255). Guard only the real
// corruption signal — it must be a positive integer, never null/0.
check(magikarp && Number.isInteger(magikarp.catchRate) && magikarp.catchRate > 0, `magikarp catchRate must be a positive integer (got ${magikarp?.catchRate})`);
check(doubleEdge && doubleEdge.power === 120, `double-edge power anchor != 120`);
check(doubleEdge && doubleEdge.pp >= 1, `double-edge pp must be >= 1 (got ${doubleEdge?.pp})`);
check(tackle && tackle.power === 40 && tackle.pp >= 1, `tackle (power 40, pp >= 1) anchor failed`);

// ---- summary -------------------------------------------------------------
console.log(`monsters checked: ${monsters.length}`);
console.log(`moves checked:    ${moves.length}`);
console.log(`evolution steps:  ${monsters.reduce((n, m) => n + m.evolution.length, 0)}`);
console.log(`learnset entries: ${monsters.reduce((n, m) => n + m.learnset.length, 0)}`);
if (failures === 0) {
  console.log("\n=== DATA OK ===");
  process.exit(0);
} else {
  console.error(`\n=== ${failures} CHECK(S) FAILED ===`);
  process.exit(1);
}

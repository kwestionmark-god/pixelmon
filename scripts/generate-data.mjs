/**
 * PokeAPI data pipeline for Pixelmon (Kanto region, Gen 1).
 *
 * Fetches the real 150 Kanto Pokemon (PokeAPI ids 1..150) and generates:
 *   - src/data/monsters.json   : species templates (accurate stats/types/etc.)
 *   - src/data/moves.json       : move definitions
 *
 * The first run fetches everything live from https://pokeapi.co (factual, not
 * hand-authored — this is what fixes the earlier "hallucinated monster" problem)
 * and caches every response under scripts/pokeapi-cache/. Later runs serve from
 * that cache with no network; pass --refresh to re-fetch and repopulate it.
 *
 * Usage:
 *   node scripts/generate-data.mjs        (use cache, populate it on first run)
 *   node scripts/generate-data.mjs --refresh   (re-fetch all endpoints live)
 */
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const API = "https://pokeapi.co/api/v2";
const OUT_DIR = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../src/data");
const KANTO_MAX = 150; // ids 1..150 = original Kanto dex
// Disk cache of raw PokeAPI responses: populate once, then serve every later
// run from disk with no network. Pass --refresh to force a live repopulate.
const CACHE_DIR = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "./pokeapi-cache");
const REFRESH = process.argv.includes("--refresh");

// Backoff between live-fetch retries only; no per-request politeness delay.
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

// Stable, readable filename for a cached response. The full URL is part of the
// key (non-alphanumerics collapsed), so different endpoints never collide.
function cachePath(url) {
  return path.join(CACHE_DIR, url.replace(/[^a-zA-Z0-9]+/g, "_") + ".json");
}

async function fetchJson(url, retries = 3) {
  // Serve from the on-disk cache when present. --refresh bypasses it to force a
  // live repopulate. A cache miss falls through to the live fetch below.
  if (!REFRESH) {
    try {
      return JSON.parse(fs.readFileSync(cachePath(url), "utf8"));
    } catch {
      /* cache miss */
    }
  }
  for (let attempt = 0; attempt <= retries; attempt++) {
    try {
      const res = await fetch(url);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const data = await res.json();
      fs.mkdirSync(CACHE_DIR, { recursive: true });
      fs.writeFileSync(cachePath(url), JSON.stringify(data));
      return data;
    } catch (err) {
      if (attempt === retries) throw err;
      console.error(`  retry ${attempt + 1} for ${url}: ${err.message}`);
      await sleep(1500 * (attempt + 1));
    }
  }
}

// ---- evolution parsing -----------------------------------------------------
// PokeAPI evolution chain is nested: each node has species + evolves_to[].
function collectEvolutions(node, out) {
  const children = node.evolves_to ?? node.chain?.evolves_to ?? [];
  for (const child of children) {
    const detail = (child.evolution_details ?? [])[0] ?? {};
    const trigger = detail.trigger ?? {};
    const tname = trigger.name ?? "";
    let method = "level";
    let requirement = String(detail.min_level ?? 0);
    let condition;

    switch (tname) {
      case "level-up":
        method = "level";
        requirement = String(detail.min_level ?? 0);
        break;
      case "use-item":
        method = "stone";
        requirement = detail.item?.name ?? "any"; // e.g. fire-stone
        break;
      case "trade":
        method = "trade";
        requirement = "any";
        break;
      case "known-move":
        method = "move";
        requirement = detail.known_move?.name ?? "any";
        break;
      case "throw":
        method = "location";
        requirement = "thrown";
        break;
      case "other":
        method = "location";
        requirement = detail.location?.name ?? "any";
        break;
      default:
        method = "level";
        requirement = String(detail.min_level ?? 0);
    }

    // Happiness-based evolutions (e.g. Greavard, some friendship evolutions).
    if (detail.min_happiness != null) {
      method = "friendship";
      requirement = String(detail.min_happiness);
    }

    // Day / night conditions (detail.hour ranges).
    if (detail.hour) {
      condition = detail.hour >= 0 && detail.hour < 18 ? "day" : "night";
    }

    out.push({
      to: child.species.name,
      method,
      requirement,
      ...(condition ? { condition } : {}),
    });
    collectEvolutions(child, out);
  }
}

// ---- species + evolution ---------------------------------------------------
async function loadSpecies(pokemon) {
  const sp = await fetchJson(pokemon.species.url);
  const genderRatio = sp.gender_rate === -1 ? -1 : sp.gender_rate * 12.5; // 0..100, -1 genderless
  const dexEntry = (sp.flavor_entries ?? [])
    .find((e) => e.language.name === "en")
    ?.flavor_text?.replace(/[\n\f\r]/g, " ")
    .trim();
  return {
    catchRate: sp.capture_rate,
    baseFriendship: sp.base_happiness ?? 70,
    genderRatio,
    eggGroups: (sp.egg_groups ?? []).map((e) => e.name),
    growthRate: sp.growth_rate?.name ?? "medium-slow",
    category: sp.category?.replace(/\b\w/g, (c) => c.toUpperCase()) ?? "",
    dexEntry,
    evolutionChainUrl: sp.evolution_chain?.url,
  };
}

// ---- moves -----------------------------------------------------------------
async function loadLearnset(pokemon) {
  // Dedup to the lowest level-up level per move (a move can list multiple levels).
  const byMove = new Map();
  for (const ml of pokemon.moves ?? []) {
    const vgs = ml.version_group_details ?? [];
    for (const vg of vgs) {
      if (vg.move_learn_method.name !== "level-up") continue;
      const lvl = vg.level_learned_at; // PokeAPI field name (not "level")
      if (typeof lvl !== "number") continue;
      const prev = byMove.get(ml.move.name);
      if (prev == null || lvl < prev) byMove.set(ml.move.name, lvl);
    }
  }
  const set = [...byMove.entries()].map(([move, level]) => ({ level, move }));
  return set.sort((a, b) => a.level - b.level || a.move.localeCompare(b.move));
}

// ---- sprites ---------------------------------------------------------------
function pickSprite(pokemon, key) {
  const other = pokemon.sprites.other ?? {};
  return (
    other["official-artwork"]?.[key] ??
    pokemon.sprites[key] ??
    pokemon.sprites.icons?.[key]
  );
}

// ---- main ------------------------------------------------------------------
async function main() {
  fs.mkdirSync(OUT_DIR, { recursive: true });
  const moveCache = new Map(); // move url -> move def
  const chainCache = new Map(); // chain url -> parsed evolution list

  console.log(
    REFRESH
      ? `Refresh mode: fetching ${KANTO_MAX} Kanto Pokemon (ids 1..${KANTO_MAX}) live...`
      : `Cache mode: ${KANTO_MAX} Kanto Pokemon (ids 1..${KANTO_MAX}) from ${CACHE_DIR}`
  );
  const monsters = [];

  for (let id = 1; id <= KANTO_MAX; id++) {
    const p = await fetchJson(`${API}/pokemon/${id}`);
    const sp = await loadSpecies(p);

    const chain = await fetchJson(sp.evolutionChainUrl);
    const evolutions = [];
    collectEvolutions(chain, evolutions);

    const learnset = await loadLearnset(p);

    // Queue move-unique fetches (dedup).
    for (const { move } of learnset) {
      if (!moveCache.has(move)) moveCache.set(move, { url: `${API}/move/${move}` });
    }

    const types = (p.types ?? []).map((t) => t.type.name);
    // PokeAPI stat names -> canonical Pixelmon keys (match Monster::Stats fields).
    const STAT_KEY = {
      hp: "hp", attack: "attack", defense: "defense",
      "special-attack": "spAttack", "special-defense": "spDefense", speed: "speed",
    };
    const baseStats = Object.fromEntries(
      (p.stats ?? []).map((s) => [STAT_KEY[s.stat.name] ?? s.stat.name, s.base_stat])
    );
    const evYield = Object.fromEntries(
      (p.stats ?? [])
        .filter((s) => s.effort > 0)
        .map((s) => [STAT_KEY[s.stat.name] ?? s.stat.name, s.effort])
    );
    const abilities = (p.abilities ?? [])
      .map((a) => a.ability.name)
      .filter((v, i, a) => a.indexOf(v) === i); // dedup, keep order

    monsters.push({
      id: `PM-${String(id).padStart(3, "0")}`,
      num: id,
      name: p.name,
      types,
      baseStats,
      evYield,
      abilities,
      height: p.height / 10, // dm -> m
      weight: p.weight / 10, // hg -> kg
      genderRatio: sp.genderRatio,
      eggGroups: sp.eggGroups,
      growthRate: sp.growthRate,
      catchRate: sp.catchRate,
      baseFriendship: sp.baseFriendship,
      baseExp: p.base_experience,
      learnset,
      evolution: evolutions,
      pokedex: { category: sp.category, entry: sp.dexEntry },
      sprites: {
        front: pickSprite(p, "front_default"),
        back: pickSprite(p, "back_default"),
        icon: pickSprite(p, "front_default"),
      },
    });

    if (id % 25 === 0) console.log(`  ${id}/${KANTO_MAX} done`);
  }

  console.log(`Fetching ${moveCache.size} unique move definitions...`);
  const moves = [];
  let moveIdx = 0;
  for (const { url } of moveCache.values()) {
    const m = await fetchJson(url);
    moves.push({
      id: `MV-${String(++moveIdx).padStart(4, "0")}`,
      name: m.name.replace(/\b\w/g, (c) => c.toUpperCase()),
      type: m.type?.name ?? "normal",
      category: m.damage_class?.name ?? "status",
      power: m.power ?? 0,
      accuracy: m.accuracy ?? 0,
      pp: m.pp ?? 0,
      damagePower: m.power ?? 0,
      flavor: (m.flavor_text_entries ?? [])
        .find((e) => e.language.name === "en")
        ?.flavor_text?.replace(/[\n\f\r]/g, " ")
        .trim(),
    });
  }

  // Index moves by slug for cross-referencing learnsets.
  const moveById = new Map(moves.map((m) => [m.name.toLowerCase().replace(/ /g, "-"), m]));

  fs.writeFileSync(path.join(OUT_DIR, "monsters.json"), JSON.stringify(monsters, null, 2));
  fs.writeFileSync(path.join(OUT_DIR, "moves.json"), JSON.stringify(moves, null, 2));

  console.log("\n=== DONE ===");
  console.log(`monsters: ${monsters.length}`);
  console.log(`moves:    ${moves.length}`);
  console.log(`sample:   ${monsters[0].name} (${monsters[0].id}) -> ${monsters[0].types.join("/")}, exp ${monsters[0].baseExp}`);
  const withEvo = monsters.filter((m) => m.evolution.length).length;
  console.log(`monsters w/ evolutions: ${withEvo}`);
}

main().catch((err) => {
  console.error("FATAL:", err);
  process.exit(1);
});

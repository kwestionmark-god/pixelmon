#!/usr/bin/env node
/**
 * generate-maps.mjs
 *
 * Parses vendored pokered map data and emits per-map JSON files under
 * src/data/maps/. Each JSON describes one map's tiles, warps, objects, and
 * connections. An index.json maps map-id strings to their JSON paths.
 *
 * Inputs (vendored under scripts/vendor/pokered/):
 *   constants/map_constants.asm        – map id → (blockWidth, blockHeight)
 *   data/maps/headers/<Map>.asm        – name, tileset, connections
 *   data/maps/objects/<Map>.asm        – warps, signs, NPCs
 *   maps/<Map>.blk                     – raw block bytes
 *   gfx/blocksets/<tileset>.bst        – block GFX (128 blocks × 16 bytes)
 *   data/tilesets/collision_tile_ids.asm – per-tileset collision 8px-tile IDs
 *
 * Outputs:
 *   src/data/maps/<mapId>.json
 *   src/data/maps/index.json
 *
 * Run scripts/build_tileset_atlases.py first (or after any tileset change) so
 * the atlas PNGs under src/data/maps/tilesets/ stay in sync with the
 * spriteIndex values written here.
 */

import { readFileSync, writeFileSync, mkdirSync, existsSync, readdirSync } from 'fs';
import { resolve, dirname } from 'path';
import { fileURLToPath } from 'url';
import { expandBlock } from './mapdata/blocktables.mjs';

const SCRIPTS_DIR = dirname(fileURLToPath(import.meta.url));
const POKERED = resolve(SCRIPTS_DIR, 'vendor/pokered');
const OUTPUT_DIR = resolve(SCRIPTS_DIR, '..', 'src', 'data', 'maps');

// ---------------------------------------------------------------------------
// ASM parsers
// ---------------------------------------------------------------------------

function readText(path) {
  return readFileSync(path, 'utf8');
}

/** Convert SCREAMING_SNAKE_CASE to TitleCase for filesystem lookups.
 *  PALLET_TOWN → PalletTown, BLUES_HOUSE → BluesHouse, ROUTE_12 → Route12 */
function toTitleCase(s) {
  // Split on underscores, capitalize first letter, lowercase rest.
  // Preserve uppercase letters that follow digits (e.g. 1F → 1F).
  // PALLET_TOWN → PalletTown, REDS_HOUSE_1F → RedsHouse1F
  return s.split('_').map(seg => {
    let lower = seg[0].toUpperCase() + seg.slice(1).toLowerCase();
    // Re-uppercase letters immediately after digits: 1f → 1F
    lower = lower.replace(/(\d)([a-z])/g, (_, d, c) => d + c.toUpperCase());
    return lower;
  }).join('');
}

/** Parse map_constants.asm for map id → { widthBlocks, heightBlocks, tilesetId }. */
function parseMapConstants() {
  const text = readText(resolve(POKERED, 'constants/map_constants.asm'));
  const out = {};
  // map_const NAME, W, H
  const re = /map_const\s+(\w+),\s*(\d+),\s*(\d+)/g;
  let m;
  while ((m = re.exec(text)) !== null) {
    out[m[1]] = { widthBlocks: +m[2], heightBlocks: +m[3] };
  }
  return out;
}

/** Tileset name (normalized) → { grassTile }. */
function parseTilesetHeaders() {
  const text = readText(resolve(POKERED, 'data/tilesets/tileset_headers.asm'));
  const out = {};
  // tileset Name, -1, -1, -1, $52, TILEANIM_...
  const re = /tileset\s+(\w+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*(\$\??[0-9a-fA-F]+|-\d*),\s*(\w+)/g;
  let m;
  while ((m = re.exec(text)) !== null) {
    const raw = m[5].replace('$', '');
    const grassTile = raw.startsWith('-') ? -1 : parseInt(raw, 16);
    out[normalizeTilesetName(m[1])] = { grassTile };
  }
  return out;
}

/** Case/underscore-insensitive tileset key: RedsHouse1 → redshouse1. */
function normalizeTilesetName(name) {
  return name.replace(/_/g, '').toLowerCase();
}

/** Parse collision_tile_ids.asm → { normalizedTilesetName: Set<8px-tile-id> }.
 *  Handles consecutive label aliases (e.g. RedsHouse1_Coll:: RedsHouse2_Coll::
 *  sharing one coll_tiles line). */
function parseCollisionLists() {
  const text = readText(resolve(POKERED, 'data/tilesets/collision_tile_ids.asm'));
  const out = {};
  let pending = [];
  for (const line of text.split('\n')) {
    const label = line.match(/^(\w+_Coll)::/);
    if (label) {
      pending.push(normalizeTilesetName(label[1].replace(/_Coll$/, '')));
    }
    const tiles = line.match(/coll_tiles\s+(.*)/);
    if (tiles) {
      const ids = tiles[1].match(/\$([0-9a-fA-F]+)/g) || [];
      const set = new Set(ids.map(h => parseInt(h.slice(1), 16)));
      for (const key of pending) out[key] = set;
      pending = [];
    }
  }
  return out;
}

/** Map id → actual header filename, matched case-insensitively with
 *  underscores stripped (SS_ANNE_1F → SSAnne1F.asm). */
const headerFiles = {};
for (const f of readdirSync(resolve(POKERED, 'data/maps/headers'))) {
  if (f.endsWith('.asm')) headerFiles[f.slice(0, -4).toLowerCase()] = f;
}
function headerFileFor(mapId) {
  return headerFiles[mapId.replace(/_/g, '').toLowerCase()] || null;
}

/** Parse a map header .asm → { name, tileset, connections }. Returns null if missing. */
function parseMapHeader(mapId) {
  const f = headerFileFor(mapId);
  if (!f) return null;
  const p = resolve(POKERED, `data/maps/headers/${f}`);
  const text = readText(p);
  const nameMatch = text.match(/map_header\s+(\w+)/);
  // Tileset is the third map_header field: map_header MapName, MAP_CONST, TILESET, ...
  const tilesetMatch = text.match(/map_header\s+\w+,\s*\w+,\s*(\w+)/);
  const connections = [];
  // connection north, Route1, ROUTE_1, 0  (offset may be negative)
  const connRe = /connection\s+(\w+),\s*(\w+),\s*(\w+),\s*(-?\d+)/g;
  let c;
  while ((c = connRe.exec(text)) !== null) {
    // c[3] is the map constant (ROUTE_1), matching index.json ids / warp targets.
    connections.push({ direction: c[1], targetMap: c[3].toLowerCase(), offset: +c[4] });
  }
  return {
    name: nameMatch ? nameMatch[1] : mapId,
    tileset: tilesetMatch ? tilesetMatch[1].toUpperCase() : 'OVERWORLD',
    connections,
  };
}

const objectFiles = {};
for (const f of readdirSync(resolve(POKERED, 'data/maps/objects'))) {
  if (f.endsWith('.asm')) objectFiles[f.slice(0, -4).toLowerCase()] = f;
}

/** Parse a map objects .asm → { warps, signs, npcs }. Returns null if missing. */
function parseMapObjects(mapId) {
  const f = objectFiles[mapId.replace(/_/g, '').toLowerCase()];
  if (!f) return null;
  const p = resolve(POKERED, `data/maps/objects/${f}`);
  const text = readText(p);
  const warps = [];
  const signs = [];
  const npcs = [];
  // warp_event x, y, DEST_MAP, warpId
  const warpRe = /warp_event\s+(\d+),\s*(\d+),\s*(\w+),\s*(\d+)/g;
  let m;
  while ((m = warpRe.exec(text)) !== null) {
    warps.push({ x: +m[1], y: +m[2], targetMap: m[3].toLowerCase(), warpId: +m[4] - 1 });
  }
  // bg_event x, y, textId
  const bgRe = /bg_event\s+(\d+),\s*(\d+),\s*(\w+)/g;
  let b;
  while ((b = bgRe.exec(text)) !== null) {
    signs.push({ x: +b[1], y: +b[2], textId: b[3] });
  }
  // object_event x, y, SPRITE, movement, range, textId
  const objRe = /object_event\s+(\d+),\s*(\d+),\s*(\w+),\s*(\w+),\s*(\w+),\s*(\w+)/g;
  let o;
  while ((o = objRe.exec(text)) !== null) {
    npcs.push({ x: +o[1], y: +o[2], sprite: o[3], movement: o[4], range: o[5], textId: o[6] });
  }
  return { warps, signs, npcs };
}

// ---------------------------------------------------------------------------
// Blockset / collision
// ---------------------------------------------------------------------------

// Tileset name -> blockset/gfx file stem; must match build_tileset_atlases.py.
// In Gen 1 the Mart, Dojo, and Museum tilesets share gfx/blocksets with
// Pokecenter, Gym, and Gate respectively (see gfx/tilesets.asm).
const TILESET_BST = {
  OVERWORLD: 'overworld',
  FOREST: 'forest',
  HOUSE: 'house',
  MART: 'pokecenter',
  POKECENTER: 'pokecenter',
  GYM: 'gym',
  DOJO: 'gym',
  MUSEUM: 'gate',
  FOREST_GATE: 'gate',
  UNDERGROUND: 'underground',
  GATE: 'gate',
  SHIP: 'ship',
  SHIPPORT: 'ship_port',
  CEMETERY: 'cemetery',
  INTERIOR: 'interior',
  CAVERN: 'cavern',
  LOBBY: 'lobby',
  MANSION: 'mansion',
  LAB: 'lab',
  CLUB: 'club',
  FACILITY: 'facility',
  PLATEAU: 'plateau',
  REDS_HOUSE_1: 'reds_house',
  REDS_HOUSE_2: 'reds_house',
};

/** Underscore-insensitive lookup: SHIP_PORT and SHIPPORT both match. */
function lookupBst(tilesetName) {
  const want = tilesetName.replace(/_/g, '');
  for (const k of Object.keys(TILESET_BST)) {
    if (k.replace(/_/g, '') === want) return TILESET_BST[k];
  }
  return null;
}

function loadBlockset(tilesetName) {
  const fname = lookupBst(tilesetName);
  if (!fname) return null;
  const p = resolve(POKERED, `gfx/blocksets/${fname}.bst`);
  if (!existsSync(p)) return null;
  return readFileSync(p);
}

function getCollisionSet(collisionLists, tilesetName) {
  return collisionLists[normalizeTilesetName(tilesetName)] || new Set();
}

// ---------------------------------------------------------------------------
// Map building
// ---------------------------------------------------------------------------

function buildMap(mapId, constants, headers, objects, tilesetHeaders, collisionLists) {
  const dim = constants[mapId.toUpperCase()];
  if (!dim) {
    console.error(`generate-maps: no dims for ${mapId}`);
    return null;
  }
  const header = headers[mapId];
  if (!header) {
    console.error(`generate-maps: no header for ${mapId}`);
    return null;
  }
  const obj = objects[mapId];
  if (!obj) {
    console.error(`generate-maps: no objects for ${mapId}`);
    return null;
  }

  const { widthBlocks, heightBlocks } = dim;
  // Each pokered block is 32×32px = 2×2 of our 16px game tiles.
  const tileW = widthBlocks * 2;
  const tileH = heightBlocks * 2;

  const bst = loadBlockset(header.tileset);
  const collision = getCollisionSet(collisionLists, header.tileset);
  const grassTile = (tilesetHeaders[normalizeTilesetName(header.tileset)] || {}).grassTile ?? -1;
  const tilesetFile = lookupBst(header.tileset) || header.tileset.toLowerCase();
  const blkPath = resolve(POKERED, `maps/${header.name}.blk`);
  if (!existsSync(blkPath)) {
    console.error(`generate-maps: missing blk for ${header.name}`);
    return null;
  }
  const blkData = readFileSync(blkPath);
  if (!bst) {
    console.error(`generate-maps: missing blockset for ${header.name} (${header.tileset})`);
    return null;
  }

  const empty = () => ({ tileType: 0, spriteIndex: 0, solid: false });
  const ground = [];
  const objectsLayer = [];
  const overlay = [];
  for (let y = 0; y < tileH; y++) {
    ground[y] = [];
    objectsLayer[y] = [];
    overlay[y] = [];
    for (let x = 0; x < tileW; x++) {
      ground[y][x] = empty();
      objectsLayer[y][x] = empty();
      overlay[y][x] = empty();
    }
  }

  // Expand each block into its 2×2 quadrant tiles: [TL, TR, BL, BR].
  for (let by = 0; by < heightBlocks; by++) {
    for (let bx = 0; bx < widthBlocks; bx++) {
      const blockId = by * widthBlocks + bx;
      if (blockId >= blkData.length) continue;
      const [tl, tr, bl, br] = expandBlock(bst, blkData[blockId], { collision, grassTile });
      const gx = bx * 2;
      const gy = by * 2;
      ground[gy][gx] = tl;
      ground[gy][gx + 1] = tr;
      ground[gy + 1][gx] = bl;
      ground[gy + 1][gx + 1] = br;
    }
  }

  // Warps: pokered warp coords are in 16px-tile units already.
  // Store as { x, y, targetMap, warpId } — target position resolved by MapStore.
  const warps = obj.warps
    .filter(w => !w.targetMap.startsWith('unused_map'))
    .map(w => ({ x: w.x, y: w.y, targetMap: w.targetMap, warpId: w.warpId }));

  // Warp tiles must be walkable (door mats, stairs, exits): the game lands
  // the player on them, and Gen 1 lets you stand on passable subtiles even
  // when our 16px quadrant mixes in solid door-frame tiles.
  for (const w of warps) {
    if (ground[w.y] && ground[w.y][w.x]) ground[w.y][w.x].solid = false;
  }

  // NPCs: store position + sprite + movement type
  const npcs = obj.npcs.map(n => ({
    x: n.x, y: n.y,
    sprite: n.sprite,
    movement: n.movement,
    textId: n.textId,
  }));

  // Signs
  const signs = obj.signs.map(s => ({ x: s.x, y: s.y, textId: s.textId }));

  return {
    id: mapId.toLowerCase(),
    name: header.name,
    tileset: header.tileset,
    tilesetFile,
    width: tileW,
    height: tileH,
    ground,
    objects: objectsLayer,
    overlay,
    warps,
    npcs,
    signs,
    connections: header.connections,
  };
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

function main() {
  const constants = parseMapConstants();
  const tilesetHeaders = parseTilesetHeaders();
  const collisionLists = parseCollisionLists();

  // Build for all maps listed in map_constants (skip zero-dim placeholders).
  const mapIds = Object.keys(constants).filter(k => constants[k].widthBlocks > 0);

  // Parse all headers and objects.
  const headers = {};
  const objects = {};
  for (const id of mapIds) {
    const h = parseMapHeader(id);
    if (!h) { console.warn(`skip ${id}: no header`); continue; }
    headers[id] = h;
    const o = parseMapObjects(id);
    if (!o) { console.warn(`skip ${id}: no objects`); continue; }
    objects[id] = o;
  }

  mkdirSync(OUTPUT_DIR, { recursive: true });

  const maps = {};
  const index = [];
  for (const id of mapIds) {
    const map = buildMap(id, constants, headers, objects, tilesetHeaders, collisionLists);
    if (!map) continue;
    maps[id.toLowerCase()] = map;

    // Silph Co elevator: pokered drives this with a scripted elevator menu we
    // don't have yet; synthesize exits to 1F so the elevator never softlocks.
    const elev = maps.silph_co_elevator;
    const lobby = maps.silph_co_1f;
    if (elev && lobby && elev.warps.length === 0) {
      const doorIdx = lobby.warps.findIndex(w => w.targetMap === 'silph_co_elevator');
      if (doorIdx !== -1) {
        elev.warps.push({ x: 1, y: 3, targetMap: 'silph_co_1f', warpId: doorIdx });
        elev.warps.push({ x: 2, y: 3, targetMap: 'silph_co_1f', warpId: doorIdx });
        for (const w of elev.warps) elev.ground[w.y][w.x].solid = false;
      }
    }
  }
  for (const [idLower, map] of Object.entries(maps)) {
    const path = `${idLower}.json`;
    writeFileSync(resolve(OUTPUT_DIR, path), JSON.stringify(map, null, 2));
    index.push({ id: idLower, path });
    map.id = idLower;
  }
  writeFileSync(resolve(OUTPUT_DIR, 'index.json'), JSON.stringify(index, null, 2));

  console.log(`generated ${index.length} maps → ${OUTPUT_DIR}`);
}

main();

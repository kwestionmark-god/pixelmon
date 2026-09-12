#!/usr/bin/env node
/**
 * generate-encounters.mjs
 *
 * Parses vendored pokered wild encounter data and merges encounter entries
 * into each map JSON under src/data/maps/.
 *
 * Inputs (vendored under scripts/vendor/pokered/):
 *   data/wild/grass_water.asm          – map-index → wild mons label table
 *   data/wild/maps/<Name>.asm          – per-map grass/water encounter tables
 *
 * Species name → PokeAPI monster-number mapping is loaded from monsters.json.
 * Unmapped species are skipped with a warning.
 *
 * Output:
 *   src/data/maps/<mapId>.json  (augmented with an "encounters" field)
 */

import { readFileSync, writeFileSync, readdirSync, existsSync } from 'fs';
import { resolve, dirname } from 'path';
import { fileURLToPath } from 'url';

const SCRIPTS_DIR = dirname(fileURLToPath(import.meta.url));
const POKERED = resolve(SCRIPTS_DIR, 'vendor/pokered');
const DATA_DIR = resolve(SCRIPTS_DIR, '..', 'src', 'data', 'maps');

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function readText(path) {
  return readFileSync(path, 'utf8');
}

/** Strip underscores, lowercase → normalized key for matching. */
function normalize(s) {
  return s.replace(/_/g, '').toLowerCase();
}

// ---------------------------------------------------------------------------
// Species lookup: pokered name → PokeAPI monster number
// ---------------------------------------------------------------------------

function buildSpeciesLookup() {
  const path = resolve(SCRIPTS_DIR, '..', 'src', 'data', 'monsters.json');
  const monsters = JSON.parse(readText(path));
  const lookup = {};
  for (const m of monsters) {
    // Normalize: nidoran-f → NIDORAN_F, nidoran-m → NIDORAN_M
    const key = m.name.toUpperCase().replace(/-/g, '_');
    lookup[key] = m.num;
  }
  return lookup;
}

// ---------------------------------------------------------------------------
// Parse map_constants.asm → { index: mapConstant }
// ---------------------------------------------------------------------------

function parseMapConstantIndices() {
  const text = readText(resolve(POKERED, 'constants/map_constants.asm'));
  const constants = [];
  // Match map_const NAME, W, H ; $XX
  const re = /map_const\s+(\w+)/g;
  let m;
  while ((m = re.exec(text)) !== null) {
    constants.push(m[1]);
  }
  return constants;
}

// ---------------------------------------------------------------------------
// Parse grass_water.asm → { index → label }
// ---------------------------------------------------------------------------

function parseWildDataPointers() {
  const text = readText(resolve(POKERED, 'data/wild/grass_water.asm'));
  const labels = [];
  // Match lines like: dw Route1WildMons         ; ROUTE_1  or just dw NothingWildMons
  const re = /dw\s+(\w+)/g;
  let m;
  while ((m = re.exec(text)) !== null) {
    labels.push(m[1]);
  }
  // Build index → constant map by aligning with map_constants
  const mapConstants = parseMapConstantIndices();
  const indexToConstant = {};
  for (let i = 0; i < labels.length && i < mapConstants.length; i++) {
    indexToConstant[i] = mapConstants[i];
  }
  // Also build label → constant for direct lookup
  const labelToConstant = {};
  for (const [idx, constant] of Object.entries(indexToConstant)) {
    labelToConstant[labels[idx]] = constant;
  }
  return { indexToConstant, labelToConstant };
}

// ---------------------------------------------------------------------------
// Parse a single wild mons asm file → { grass: [{level, species},...], water: [...] }
// ---------------------------------------------------------------------------

function parseWildMonsFile(filename) {
  const path = resolve(POKERED, `data/wild/maps/${filename}`);
  if (!existsSync(path)) return null;
  const text = readText(path);

  const grass = [];
  const water = [];

  let section = null; // 'grass' or 'water'
  const lines = text.split('\n');

  for (const line of lines) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith(';')) continue;

    if (trimmed.startsWith('def_grass_wildmons')) {
      section = 'grass';
      continue;
    }
    if (trimmed.startsWith('def_water_wildmons')) {
      section = 'water';
      continue;
    }
    if (trimmed.startsWith('end_grass_wildmons') || trimmed.startsWith('end_water_wildmons')) {
      section = null;
      continue;
    }
    // IF DEF(_RED) / IF DEF(_BLUE) / ENDC – include both branches
    if (trimmed.startsWith('IF DEF(_') || trimmed.startsWith('ENDC')) {
      continue;
    }

    // db <level>, <SPECIES>
    const dbRe = /^db\s+(\d+),\s*(\w+)/;
    const dm = dbRe.exec(trimmed);
    if (dm && section) {
      const level = parseInt(dm[1], 10);
      const species = dm[2].toUpperCase();
      if (section === 'grass') {
        grass.push({ level, species });
      } else {
        water.push({ level, species });
      }
    }
  }

  return { grass, water };
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

function main() {
  const speciesLookup = buildSpeciesLookup();
  const { indexToConstant } = parseWildDataPointers();

  // Build map-constant → encounters mapping
  const mapEncounters = {};

  // List all asm files in wild/maps/
  const wildFiles = readdirSync(resolve(POKERED, 'data/wild/maps'));
  for (const fname of wildFiles) {
    if (!fname.endsWith('.asm')) continue;
    // Filenames are like Route1.asm; labels in grass_water.asm are Route1WildMons
    const baseName = fname.replace('.asm', '');
    const label = baseName + 'WildMons';
    // Find index for this label
    const text = readText(resolve(POKERED, 'data/wild/grass_water.asm'));
    const labelRe = new RegExp('dw\\s+' + label.replace(/_/g, '_'));
    const labelMatch = text.match(labelRe);
    if (!labelMatch) {
      console.warn(`generate-encounters: no label ${label} in grass_water.asm`);
      continue;
    }
    const labelIdx = (labelMatch.index);
    // Count dw entries before this one to get the index
    const dwBefore = text.slice(0, labelMatch.index).match(/dw\s+/g);
    const idx = dwBefore ? dwBefore.length : 0;
    const mapConstant = indexToConstant[idx];
    if (!mapConstant) {
      console.warn(`generate-encounters: no map constant at index ${idx} for ${label}`);
      continue;
    }
    const mapId = mapConstant.toLowerCase();
    const data = parseWildMonsFile(fname);
    if (!data) continue;

    // Normalize species names to PokeAPI monster numbers
    const normalizeEncounters = (entries) => {
      const out = [];
      for (const e of entries) {
        const monNum = speciesLookup[e.species];
        if (monNum === undefined) {
          console.warn(`generate-encounters: unmapped species ${e.species} in ${mapId}`);
          continue;
        }
        out.push({ level: e.level, species: monNum });
      }
      return out;
    };

    const grass = normalizeEncounters(data.grass);
    const water = normalizeEncounters(data.water);

    // Skip if both empty
    if (grass.length === 0 && water.length === 0) continue;

    mapEncounters[mapId] = { grass, water };
  }

  // Merge encounters into each map JSON
  let merged = 0;
  const mapFiles = readdirSync(DATA_DIR).filter(f => f.endsWith('.json') && f !== 'index.json');
  for (const fname of mapFiles) {
    const mapId = fname.replace('.json', '');
    const path = resolve(DATA_DIR, fname);
    const map = JSON.parse(readText(path));

    if (mapEncounters[mapId]) {
      map.encounters = mapEncounters[mapId];
      writeFileSync(path, JSON.stringify(map, null, 2) + '\n');
      merged++;
    }
  }

  console.log(`generate-encounters: merged encounters into ${merged} maps`);

  // Summary
  const totalWithEncounters = Object.keys(mapEncounters).length;
  console.log(`generate-encounters: ${totalWithEncounters} maps have encounter data`);
}

main();

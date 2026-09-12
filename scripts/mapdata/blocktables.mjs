/**
 * Block expansion from pokered blocksets.
 *
 * A pokered block is 32×32 px = 2×2 of our 16px game tiles. Each block in a
 * .bst file is 16 bytes: a 4×4 grid of 8px tile ids from the tileset's PNG.
 *
 * Graphics: each quadrant becomes one atlas tile in the per-tileset atlas
 * built by scripts/build_tileset_atlases.py. The sprite index is
 * blockId * 4 + quadrant — no curated block table needed; maps render with
 * pokered's actual graphics.
 *
 * Solidity + tile type are derived per quadrant from tileset data:
 *   - pokered collision lists are PASSABLE tiles (see CheckTilePassable in
 *     home/overworld.asm: reaching the $ff terminator = not passable). The
 *     original engine only ever tests the BOTTOM-LEFT 8px subtile of each
 *     16px quadrant (see _GetTileAndCoordsInFrontOfPlayer in
 *     engine/overworld/player_state.asm), so a quadrant is solid iff its
 *     bottom-left subtile is not in the passable list
 *   - TallGrass if that same subtile equals the tileset's grass tile id
 *   - Water if that same subtile equals the animated water tile id (0x14)
 */

const T = {
  Grass: 0,
  TallGrass: 1,
  Dirt: 2,
  Sand: 3,
  Water: 4,
  Tree: 5,
  Rock: 6,
  Building: 7,
  Wall: 8,
  Door: 9,
  Floor: 10,
  Stairs: 11,
  Sign: 12,
  NPC: 13,
};

const WATER_TILE = 0x14;

// Quadrant order TL, TR, BL, BR; indices into the block's row-major 4×4 grid.
const QUAD_IDX = [
  [0, 1, 4, 5],
  [2, 3, 6, 7],
  [8, 9, 12, 13],
  [10, 11, 14, 15],
];

/**
 * Expand one block into its 2×2 quadrant tiles.
 * @param bstData   raw .bst bytes
 * @param blockId   block id (index into .blk data)
 * @param opts      { collision: Set<number>, grassTile: number }
 * @returns [TL, TR, BL, BR] as { tileType, spriteIndex, solid }
 */
export function expandBlock(bstData, blockId, opts = {}) {
  const { collision = new Set(), grassTile = -1 } = opts;
  const offset = blockId * 16;
  const b = (i) => (offset + i < bstData.length ? bstData[offset + i] : 0);

  return QUAD_IDX.map((quad, q) => {
    const ids = quad.map((i) => b(i));
    // pokered tests only the bottom-left 8px subtile (quad[2]) of each 16px
    // quadrant; collision list = PASSABLE tiles.
    const foot = ids[2];
    const solid = !collision.has(foot);
    // $14 is only water where the tileset doesn't mark it passable
    // (interiors reuse that id for floor tiles).
    const isWater = foot === WATER_TILE && !collision.has(WATER_TILE);

    let tileType;
    if (isWater) {
      tileType = T.Water;
    } else if (foot === grassTile) {
      tileType = T.TallGrass;
    } else if (solid) {
      tileType = T.Wall;
    } else {
      tileType = T.Grass;
    }

    return { tileType, spriteIndex: blockId * 4 + q, solid };
  });
}

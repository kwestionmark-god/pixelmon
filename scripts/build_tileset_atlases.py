#!/usr/bin/env python3
"""Build per-tileset 16px-tile atlas PNGs from pokered blocksets + tileset gfx.

For each tileset, every block in gfx/blocksets/<name>.bst is 16 bytes: a 4x4
grid of 8px tile ids referencing gfx/tilesets/<name>.png (16 tiles per row,
row-major). Each block expands to 2x2 quadrants of 16px (= 2x2 of the 8px
tiles). The atlas stores one 16px tile per (block, quadrant), laid out 16 per
row, so spriteIndex = blockId * 4 + quadrant, with quadrant order
TL=0, TR=1, BL=2, BR=3 — the same convention generate-maps.mjs emits.

Output: src/data/maps/tilesets/<tilesetFile>.png
"""

from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
POKERED = ROOT / "scripts" / "vendor" / "pokered"
OUT_DIR = ROOT / "src" / "data" / "maps" / "tilesets"

ATLAS_COLS = 16  # 16px tiles per atlas row — must match world_.tilesetColumns

# Quadrant q -> indices of its 2x2 8px tiles inside the block's 16 bytes.
QUAD_IDX = ((0, 1, 4, 5), (2, 3, 6, 7), (8, 9, 12, 13), (10, 11, 14, 15))

# Tileset name (as written in map headers) -> blockset/gfx file stem.
# Must match TILESET_BST in generate-maps.mjs.
TILESET_BST = {
    "OVERWORLD": "overworld",
    "FOREST": "forest",
    "HOUSE": "house",
    "POKECENTER": "pokecenter",
    "GYM": "gym",
    "UNDERGROUND": "underground",
    "GATE": "gate",
    "SHIP": "ship",
    "SHIPPORT": "ship_port",
    "CEMETERY": "cemetery",
    "INTERIOR": "interior",
    "CAVERN": "cavern",
    "LOBBY": "lobby",
    "MANSION": "mansion",
    "LAB": "lab",
    "CLUB": "club",
    "FACILITY": "facility",
    "PLATEAU": "plateau",
    "REDS_HOUSE_1": "reds_house",
    "REDS_HOUSE_2": "reds_house",
}

# Game Boy green palette, lightest -> darkest.
PALETTE = (
    (155, 188, 15),
    (139, 172, 15),
    (48, 98, 48),
    (15, 56, 15),
)


def colorize(img: Image.Image) -> Image.Image:
    """Map the 2bpp grayscale image to the Game Boy green palette."""
    gray = img.convert("L")
    out = Image.new("RGB", gray.size)
    src = gray.load()
    dst = out.load()
    w, h = gray.size
    for y in range(h):
        for x in range(w):
            v = src[x, y]
            if v >= 192:
                dst[x, y] = PALETTE[0]
            elif v >= 128:
                dst[x, y] = PALETTE[1]
            elif v >= 64:
                dst[x, y] = PALETTE[2]
            else:
                dst[x, y] = PALETTE[3]
    return out


def load_8px_tiles(stem: str):
    """Return a list of colorized 8x8 images, index = 8px tile id."""
    png = POKERED / "gfx" / "tilesets" / f"{stem}.png"
    if not png.exists():
        return None
    img = colorize(Image.open(png))
    w, h = img.size
    tiles = []
    for ty in range(h // 8):
        for tx in range(w // 8):
            tiles.append(img.crop((tx * 8, ty * 8, tx * 8 + 8, ty * 8 + 8)))
    return tiles


def build_atlas(name: str, stem: str) -> int:
    bst_path = POKERED / "gfx" / "blocksets" / f"{stem}.bst"
    if not bst_path.exists():
        return 0
    bst = bst_path.read_bytes()
    tiles = load_8px_tiles(stem)
    if tiles is None:
        return 0

    num_blocks = len(bst) // 16
    total = num_blocks * 4
    rows = (total + ATLAS_COLS - 1) // ATLAS_COLS
    atlas = Image.new("RGB", (ATLAS_COLS * 16, rows * 16), PALETTE[0])

    blank = Image.new("RGB", (8, 8), PALETTE[0])
    for block in range(num_blocks):
        base = block * 16
        for q, quad in enumerate(QUAD_IDX):
            tile = Image.new("RGB", (16, 16))
            for i, idx in enumerate(quad):
                tile_id = bst[base + idx]
                sub = tiles[tile_id] if tile_id < len(tiles) else blank
                tile.paste(sub, ((i % 2) * 8, (i // 2) * 8))
            idx = block * 4 + q
            atlas.paste(tile, ((idx % ATLAS_COLS) * 16, (idx // ATLAS_COLS) * 16))

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    atlas.save(OUT_DIR / f"{stem}.png")
    return total


def main() -> None:
    for name, stem in sorted(TILESET_BST.items()):
        n = build_atlas(name, stem)
        if n:
            print(f"atlas {stem}: {n} tiles")
        else:
            print(f"atlas {stem}: skipped (missing bst/png)")


if __name__ == "__main__":
    main()

# Pixelmon

A full-featured, single-player, offline-first monster-catching RPG inspired by Pokémon
Gold/Silver (Gen 2). Explore the **Aurelia Region**, catch and train **150 unique
pixel-art creatures**, battle 8 Gym Leaders, defeat the Elite 4, and become Champion.

> **Stack:** TypeScript + React + Vite + Zustand · **Target:** Web (Mobile/Desktop)

## Status

In development. See the design documents for full scope:

- [`PIXELMON_GDD.md`](./PIXELMON_GDD.md) — Game Design Document (v1.0)
- [`PIXELMON_FACTORY.md`](./PIXELMON_FACTORY.md) — Creature generation & data spec (v1.0)

## Design overview

| Pillar | Description |
|--------|-------------|
| Collection | 150 creatures across 10 biomes with day/night variations |
| Strategy | Deep type-matchup system, 165 TMs, abilities, held items |
| Exploration | Non-linear world with HM-gated progression, hidden areas |
| Progression | 8 Gyms → Elite 4 → Champion → Post-Game (Battle Tower) |

## Repository layout

```
pixelmon/
├── public/            # Sprites, audio, JSON data databases
├── src/
│   ├── core/          # Game engine (battle, overworld, AI)
│   ├── components/    # React UI (battle, overworld, menus)
│   ├── data/          # TypeScript data definitions (monsters, moves, items, maps)
│   ├── systems/       # Party, storage, breeding, trading
│   ├── utils/         # Math, RNG, helpers
│   └── types/         # Shared TypeScript interfaces
├── tests/             # Unit tests for battle math
└── docs/              # PIXELMON_FACTORY.md, PIXELMON_GDD.md
```

## Development roadmap

See [GDD §11](./PIXELMON_GDD.md#11-development-roadmap) for the full 6-phase plan.

- **Phase 1 – Foundation:** project setup, core data structures, battle engine prototype
- **Phase 2 – Battle System:** full battle state machine, 18 types, status conditions, AI
- **Phase 3 – Overworld:** tilemap renderer, player movement, map transitions, encounters
- **Phase 4 – Progression:** party management, XP/leveling, evolution, gym battles
- **Phase 5 – Content:** all 150 monsters, moves/TMs, story, Elite 4 + Champion, post-game
- **Phase 6 – Polish:** audio, save/load, UI polish, balance, performance, mobile controls

## Git workflow (agentic)

This repo follows the agentic coding protocol defined in [GDD §12](./PIXELMON_GDD.md#12-git-workflow-for-agentic-coding):
feature branches (`feat/<scope>/<slug>`), Conventional Commits, mandatory `npm run lint` /
`typecheck` / `test` before each commit. See [CODEOWNERS](./.github/CODEOWNERS).

## License

Released under the [MIT License](./LICENSE).

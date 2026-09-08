/**
 * Public type surface for Pixelmon.
 *
 * Re-exports the shared data contracts so the rest of the codebase can
 * import a single entry point: `import type { Monster } from "@/types"`.
 */
export * from "./core.js";
export * from "./monster.js";
export * from "./move.js";
export * from "./item.js";
export * from "./game.js";

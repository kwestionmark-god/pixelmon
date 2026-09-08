/**
 * Item data types (GDD §6.1).
 */
import type { ItemID } from "./core.js";

/** Item categories (GDD §6.1). */
export const ITEM_CATEGORIES = [
  "PokeBall",
  "Healing",
  "Battle",
  "TM",
  "KeyItem",
  "HeldItem",
  "Evolution",
  "Berry",
] as const;

export type ItemCategory = (typeof ITEM_CATEGORIES)[number];

/** Resolved effect of an item (pseudocode contract from GDD §6.1). */
export interface ItemEffect {
  /** Heal this much HP (or a fraction of max HP). */
  heal?: number;
  /** Cure this status condition. */
  cureStatus?: boolean;
  /** Restore this many PP to a move id. */
  restorePp?: { moveId: string; amount: number };
  /** Held-item passive (keyed by ability-like trigger). */
  held?: string;
  /** Evolution stone target monster id. */
  evolveTo?: string;
  /** Arbitrary extra fields. */
  [key: string]: unknown;
}

/** An item definition (GDD §6.1). */
export interface Item {
  id: ItemID;
  name: string;
  category: ItemCategory;
  price: number; // 0 if unsellable
  description: string;
  effect: ItemEffect;
  sprite: string; // 16x16 icon
}

/** A TM entry (GDD §6.2 / PIXELMON_FACTORY.md §5.2). */
export interface TM {
  number: number; // 0–100
  moveId: string;
  name: string;
  sprite: string;
}

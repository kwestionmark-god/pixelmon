/**
 * Game state, player, world and save types (GDD §2.2 / §9.1).
 */
import type { BadgeType, BattleType, Direction, Weather } from "./core.js";
import type { MonsterInstance } from "./monster.js";
import type { ItemID, TMNumber } from "./core.js";

/** In-game clock (GDD §2.2 / §7.2). */
export interface GameTime {
  hours: number; // 0–23
  minutes: number; // 0–59
  day: number;
}

/** A player-owned badge (GDD §2.2). */
export interface Badge {
  id: string;
  name: string;
  gym: string;
  type: BadgeType;
  obtained: boolean;
}

/** Player profile (GDD §2.2). */
export interface Player {
  name: string;
  id: string; // UUID for save files
  position: { x: number; y: number; mapId: string };
  direction: Direction;
  badges: Badge[];
  money: number;
  playTime: number; // seconds
}

/** Story progression flags (GDD §2.2). */
export interface Story {
  flags: Record<string, boolean>; // "defeated_brock", "got_bike", etc.
  gymBadges: number; // 0–8
  eliteFourDefeated: boolean;
  championDefeated: boolean;
}

/** World state (GDD §2.2). */
export interface World {
  time: GameTime;
  weather: Weather;
  mapId: string;
  encounters: number; // steps since last encounter
}

/** Bag inventory (GDD §2.2). */
export interface Bag {
  items: Record<ItemID, number>; // stack counts
  tms: TMNumber[];
  keyItems: string[];
}

/** Global game state (GDD §2.2). */
export interface GameState {
  player: Player;
  story: Story;
  party: MonsterInstance[]; // max 6
  storage: MonsterInstance[][]; // PC boxes (30 per box, 20 boxes)
  daycare: MonsterInstance[]; // max 2
  bag: Bag;
  world: World;
}

/** Battle state snapshot (GDD §3.1.1). */
export interface Battle {
  type: BattleType;
  weather: Weather | null;
  turn: number;
  activePlayer: string | null; // MonsterInstance.uid
  activeEnemy: string | null;
}

/** Save file (GDD §9.1). */
export interface SaveFile {
  version: string; // "1.0.0"
  timestamp: number; // unix ms
  checksum: string; // md5 for validation
  player: Player;
  party: MonsterInstance[];
  storage: MonsterInstance[][];
  bag: Bag;
  story: Story;
  world: World;
  battle: Battle;
  records: {
    hallOfFame: string[];
    linkBattles: { wins: number; losses: number };
  };
}

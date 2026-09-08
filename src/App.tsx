import { HEADER } from "./ui/constants";

/**
 * Root application shell.
 *
 * Phase 1 placeholder: renders the game splash. The full overworld/battle
 * UI is layered on top of this shell in later phases (see GDD §8.1).
 */
export default function App() {
  return (
    <div className="app" role="application" aria-label={HEADER.title}>
      <h1 className="app__title">{HEADER.title}</h1>
      <p className="app__subtitle">{HEADER.tagline}</p>
      <p className="app__version">{`v${HEADER.version}`}</p>
    </div>
  );
}

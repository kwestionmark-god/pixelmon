import "@testing-library/jest-dom/vitest";
import { afterEach } from "vitest";
import { cleanup } from "@testing-library/react";

// Ensure the DOM is reset after each test (React cleanup of attached
// containers and event listeners).
afterEach(() => {
  cleanup();
});

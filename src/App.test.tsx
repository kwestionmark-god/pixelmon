import { describe, it, expect } from "vitest";
import { render, screen } from "@testing-library/react";
import App from "./App";
import { HEADER } from "./ui/constants";

describe("App shell", () => {
  it("renders the game title", () => {
    render(<App />);
    expect(screen.getByRole("heading", { name: HEADER.title })).toBeInTheDocument();
  });

  it("renders the tagline and version", () => {
    render(<App />);
    expect(screen.getByText(HEADER.tagline)).toBeInTheDocument();
    expect(screen.getByText(`v${HEADER.version}`)).toBeInTheDocument();
  });
});

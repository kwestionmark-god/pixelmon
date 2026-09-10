#include <iostream>
#include <string>

#include "core/Game.hpp"

static void usage(const char* prog) {
    std::cout <<
        "Pixelmon — Kanto monster-catching RPG (C++20 + SFML)\n"
        "Usage: " << prog << " [options]\n"
        "  --smoke         Headless check: load data + sprite + texture, no window\n"
        "  --render-out <p> Render the demo monster to a PNG (headless, no display)\n"
        "  --data <dir>    Data dir (default: auto / build dir)\n"
        "  --cache <dir>   Sprite cache dir (default: .sprite-cache)\n"
        "  --help          Show this help\n"
        "\n"
        "Without any flag, opens a window rendering Bulbasaur's PokeAPI sprite.\n";
}

int main(int argc, char** argv) {
    std::string dataDir = ".";              // build/ (JSON copied next to binary)
    std::string cacheDir = ".sprite-cache";
    std::string renderOut;
    bool headless = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--smoke") {
            headless = true;
        } else if (arg == "--render-out" && i + 1 < argc) {
            renderOut = argv[++i];
        } else if (arg == "--data" && i + 1 < argc) {
            dataDir = argv[++i];
        } else if (arg == "--cache" && i + 1 < argc) {
            cacheDir = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            usage(argv[0]);
            return 0;
        } else {
            std::cerr << "unknown argument: " << arg << "\n";
            usage(argv[0]);
            return 2;
        }
    }

    pm::Game game(dataDir, cacheDir);

    // Load data first, then pick the demo subject (Bulbasaur, Kanto #001).
    if (!game.loadData()) return 1;
    const pm::Monster* mon = game.data().monsterByNum(1);
    if (!mon) {
        std::cerr << "main: no demo monster (Kanto #1) found in data\n";
        return 1;
    }

    if (!renderOut.empty()) return game.renderToFile(*mon, renderOut) ? 0 : 1;
    if (headless) return game.headlessSmoke(*mon) ? 0 : 1;
    return game.run(*mon) ? 0 : 1;
}

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include "core/Game.hpp"
#include "core/World.hpp"
#include "core/BattleSystem.hpp"
#include "data/DataRepository.hpp"

static void usage(const char* prog) {
    std::cout <<
        "Pixelmon — Kanto monster-catching RPG (C++20 + SFML)\n"
        "Usage: " << prog << " [options]\n"
        "  --smoke         Headless check: load data + sprite + texture, no window\n"
        "  --render-out <p> Render the demo monster to a PNG (headless, no display)\n"
        "  --battle [p] [e] Run CLI battle demo: player species #p vs enemy #e (default 1 vs 4)\n"
        "  --data <dir>    Data dir (default: auto / build dir)\n"
        "  --cache <dir>   Sprite cache dir (default: .sprite-cache)\n"
        "  --map <id>      Start on the given map id (default: pallet_town)\n"
        "  --walk-test     Headless Pallet Town ↔ Route 1 connection check\n"
        "  --help          Show this help\n"
        "\n"
        "Without any flag, opens the overworld at Pallet Town.\n";
}

static bool runBattleDemo(const std::string& dataDir, int playerNum, int enemyNum) {
    pm::DataRepository data;
    if (!data.load(dataDir)) {
        if (!data.load("src/data")) {
            std::cerr << "battle: could not find data in '" << dataDir << "' or 'src/data'\n";
            return false;
        }
    }

    std::cout << "=== Battle Demo ===\n";
    std::cout << "Data loaded: " << data.count() << " monsters, " << data.moves().size() << " moves\n\n";

    pm::BattleContext ctx = pm::createDemoBattle(data, playerNum, enemyNum, 50);

    // Run battle until completion
    int maxTurns = 100; // safety limit
    while (ctx.result == pm::BattleResult::Ongoing && maxTurns-- > 0) {
        if (!pm::battleStep(ctx, data)) break;
        // Print new log entries
        static size_t lastLogSize = 0;
        for (size_t i = lastLogSize; i < ctx.log.size(); ++i) {
            std::cout << ctx.log[i].text << "\n";
        }
        lastLogSize = ctx.log.size();
    }

    if (maxTurns <= 0) {
        std::cout << "\n(Battle hit turn limit)\n";
    }

    std::cout << "\n=== Battle End ===\n";
    if (ctx.result == pm::BattleResult::PlayerWon) {
        std::cout << "Result: VICTORY!\n";
    } else if (ctx.result == pm::BattleResult::PlayerLost) {
        std::cout << "Result: DEFEAT...\n";
    } else {
        std::cout << "Result: ONGOING (turn limit)\n";
    }

    // Show final HP
    const auto& pMon = ctx.playerTeam[ctx.playerActiveIdx];
    const auto& eMon = ctx.enemyTeam[ctx.enemyActiveIdx];
    std::cout << pMon.nickname << " HP: " << pMon.currentHp << "/" << pMon.maxHp << "\n";
    std::cout << eMon.nickname << " HP: " << eMon.currentHp << "/" << eMon.maxHp << "\n";

    return true;
}

static std::string lowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool runWalkTest(const std::string& dataDir) {
    pm::World world;
    world.initMapStore(dataDir);
    if (!world.loadMap("pallet_town")) {
        world.initMapStore("src/data");
        if (!world.loadMap("pallet_town")) {
            std::cerr << "walk-test: failed to load pallet_town\n";
            return false;
        }
    }

    // North path on Pallet Town's top edge (x=10, y=0), then step off the map.
    world.player.setPosition(10, 0);
    if (!world.tryMove(pm::Direction::Up)) {
        std::cerr << "walk-test: blocked stepping north off Pallet Town\n";
        return false;
    }
    world.update(1.f);
    std::cout << "walk-test: Pallet → " << world.currentMap.id
              << " at " << world.player.x << "," << world.player.y << "\n";
    if (lowerCopy(world.currentMap.id) != "route_1") {
        std::cerr << "walk-test: expected route_1\n";
        return false;
    }

    world.player.setPosition(world.player.x, world.currentMap.height - 1);
    if (!world.tryMove(pm::Direction::Down)) {
        std::cerr << "walk-test: blocked stepping south off Route 1\n";
        return false;
    }
    world.update(1.f);
    std::cout << "walk-test: Route 1 → " << world.currentMap.id
              << " at " << world.player.x << "," << world.player.y << "\n";
    if (lowerCopy(world.currentMap.id) != "pallet_town") {
        std::cerr << "walk-test: expected pallet_town\n";
        return false;
    }
    std::cout << "walk-test: ok\n";
    return true;
}

int main(int argc, char** argv) {
    std::string dataDir = ".";              // build/ (JSON copied next to binary)
    std::string cacheDir = ".sprite-cache";
    std::string renderOut;
    bool headless = false;
    bool runBattle = false;
    bool walkTest = false;
    int battlePlayerNum = 1;   // Bulbasaur
    int battleEnemyNum = 4;    // Charmander
    std::string startMap = "pallet_town";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--smoke") {
            headless = true;
        } else if (arg == "--render-out" && i + 1 < argc) {
            renderOut = argv[++i];
        } else if (arg == "--battle") {
            runBattle = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                battlePlayerNum = std::stoi(argv[++i]);
            }
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                battleEnemyNum = std::stoi(argv[++i]);
            }
        } else if (arg == "--data" && i + 1 < argc) {
            dataDir = argv[++i];
        } else if (arg == "--cache" && i + 1 < argc) {
            cacheDir = argv[++i];
        } else if (arg == "--map" && i + 1 < argc) {
            startMap = argv[++i];
        } else if (arg == "--walk-test") {
            walkTest = true;
        } else if (arg == "--help" || arg == "-h") {
            usage(argv[0]);
            return 0;
        } else {
            std::cerr << "unknown argument: " << arg << "\n";
            usage(argv[0]);
            return 2;
        }
    }

    if (runBattle) {
        return runBattleDemo(dataDir, battlePlayerNum, battleEnemyNum) ? 0 : 1;
    }
    if (walkTest) {
        return runWalkTest(dataDir) ? 0 : 1;
    }

    pm::Game game(dataDir, cacheDir, startMap);

    if (!renderOut.empty()) {
        // Need a monster to render.
        if (!game.loadData()) return 1;
        const pm::Monster* mon = game.data().monsterByNum(1);
        if (!mon) {
            std::cerr << "main: no demo monster (Kanto #1) found in data\n";
            return 1;
        }
        return game.renderToFile(*mon, renderOut) ? 0 : 1;
    }
    if (headless) {
        if (!game.loadData()) return 1;
        const pm::Monster* mon = game.data().monsterByNum(1);
        if (!mon) return 1;
        return game.headlessSmoke(*mon) ? 0 : 1;
    }
    return game.run() ? 0 : 1;
}

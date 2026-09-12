#include "core/Game.hpp"

#include <filesystem>
#include <iostream>
#include <random>
#include <functional>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace pm {

Game::Game(const std::string& dataDir, const std::string& spriteCacheDir,
           const std::string& startMap)
    : dataDir_(dataDir), spriteCacheDir_(spriteCacheDir), startMap_(startMap) {}

bool Game::loadData() {
    if (!data_.load(dataDir_)) {
        if (!data_.load("src/data")) {
            std::cerr << "game: could not find data in '" << dataDir_ << "' or 'src/data'\n";
            return false;
        }
    }
    return true;
}

bool Game::createTileset() {
    // Create a procedural 16x16 tileset with 16 columns, 8 rows (128 tiles).
    // Row 0 (y=0): Ground tiles
    //   0: Grass       1: Dirt        2: Tree         3: Building (red roof)
    //   4: Door        5: Lab/Blue    6: Tall Grass   7: Flower Grass
    //   8: Mountain    9: Rock        10: Ledge Top   11: Ledge Side
    //   12: Fence      13: Sign       14: Cave Entry  15: Water Edge
    // Row 1 (y=1): Water animation frames (4 frames)
    //   16-19: Water frames 0-3
    //   20: Sand       21: Deep Sand  22: Shallow W.  23: Bridge
    // Row 2 (y=2): Building variants
    //   32: PokeCenter 33: PokeMart   34: Gym         35: House Variant
    //   36: Roof Red   37: Roof Blue  38: Roof Green  39: Wall Beige
    //   40: Wall White 41: Wall Red   42: Window      43: PC Sign
    //   44: Mart Sign  45: Gym Sign   46: Fence Post  47: Gate
    // Row 3 (y=3): Trees/Decor
    //   48: Tree Thin  49: Tree Thick 50: Bush        51: Flower Red
    //   52: Flower Yel 53: Flower Blu 54: Grass Tuft  55: Rock Small
    //   56: Rock Med   57: Rock Large 58: Stump       59: Cut Tree
    //   60: Berry Tree 61: Palm Tree  62: Pine Tree   63: Log
    // Row 4-7: Reserved for future use / interiors
    const int tileSize = TILE_SIZE;
    const int cols = 16;
    const int rows = 8;
    const int texW = cols * tileSize;
    const int texH = rows * tileSize;

    sf::Image img;
    img.create(texW, texH, sf::Color::Transparent);

    auto drawTile = [&](int tx, int ty, auto&& fn) {
        for (int y = 0; y < tileSize; ++y) {
            for (int x = 0; x < tileSize; ++x) {
                img.setPixel(tx * tileSize + x, ty * tileSize + y, fn(x, y));
            }
        }
    };

    // ---- Row 0: Ground tiles ----

    // 0: Grass (green with noise).
    drawTile(0, 0, [&](int x, int y) {
        int base = 55 + (x * 7 + y * 11) % 35;
        return sf::Color(25, base, 25);
    });

    // 1: Dirt path (brown).
    drawTile(1, 0, [&](int x, int y) {
        int base = 110 + (x * 13 + y * 17) % 25;
        return sf::Color(base, base * 3 / 4, base / 2);
    });

    // 2: Tree (dark green canopy + brown trunk).
    drawTile(2, 0, [&](int x, int y) {
        if (y > 11) return sf::Color(90, 55, 25); // trunk
        int g = 35 + (x * 5 + y * 7) % 25;
        return sf::Color(5, g, 5);
    });

    // 3: Building - Red roof, beige walls (generic house).
    drawTile(3, 0, [&](int x, int y) {
        if (y < 5) return sf::Color(160, 30, 30); // red roof
        if (y < 7) return sf::Color(140, 25, 25); // roof shadow
        return sf::Color(210, 190, 150); // beige walls
    });

    // 4: Door (brown with gold knob).
    drawTile(4, 0, [&](int x, int y) {
        if (x == 7 && y == 11) return sf::Color(255, 215, 0); // knob
        if (x == 7 && y == 10) return sf::Color(180, 140, 80); // knob base
        return sf::Color(110, 65, 25);
    });

    // 5: Lab / Blue roof building (Oak's Lab).
    drawTile(5, 0, [&](int x, int y) {
        if (y < 5) return sf::Color(30, 70, 200); // blue roof
        if (y < 7) return sf::Color(25, 55, 170); // roof shadow
        return sf::Color(230, 230, 240); // white walls
    });

    // 6: Tall grass (darker, taller blades).
    drawTile(6, 0, [&](int x, int y) {
        if (y < 3) return sf::Color(15, 90 + (x * 4) % 20, 15);
        if (y < 6) return sf::Color(20, 100 + (x * 3) % 15, 20);
        return sf::Color(30, 110, 30);
    });

    // 7: Flower grass (grass with flowers).
    drawTile(7, 0, [&](int x, int y) {
        int base = 55 + (x * 7 + y * 11) % 35;
        sf::Color c(25, base, 25);
        // Add flowers at specific positions.
        if ((x == 3 && y == 4) || (x == 11 && y == 8) || (x == 6 && y == 12)) {
            return sf::Color(220, 50, 100); // pink flower
        }
        if ((x == 9 && y == 3) || (x == 13 && y == 10)) {
            return sf::Color(255, 220, 50); // yellow flower
        }
        return c;
    });

    // 8: Mountain / cliff face (gray).
    drawTile(8, 0, [&](int x, int y) {
        int base = 100 + (x * 11 + y * 13) % 40;
        return sf::Color(base, base, base + 10);
    });

    // 9: Rock / boulder.
    drawTile(9, 0, [&](int x, int y) {
        int cx = 7, cy = 10;
        int dx = x - cx, dy = y - cy;
        if (dx * dx + dy * dy < 64) {
            int base = 120 + (x * 7 + y * 11) % 30;
            return sf::Color(base, base - 10, base - 20);
        }
        return sf::Color::Transparent;
    });

    // 10: Ledge top (gray-brown, walkable from top).
    drawTile(10, 0, [&](int x, int y) {
        if (y < 4) {
            int base = 130 + (x * 5 + y * 7) % 20;
            return sf::Color(base, base - 10, base - 20); // ledge surface
        }
        // Ledge face.
        int base = 90 + (x * 11 + y * 3) % 30;
        return sf::Color(base, base - 5, base - 15);
    });

    // 11: Ledge side (vertical face).
    drawTile(11, 0, [&](int x, int y) {
        int base = 90 + (x * 11 + y * 3) % 30;
        return sf::Color(base, base - 5, base - 15);
    });

    // 12: Fence (wooden).
    drawTile(12, 0, [&](int x, int y) {
        if (x == 3 || x == 11) return sf::Color(100, 60, 20); // posts
        if (y == 4 || y == 10) return sf::Color(120, 70, 25); // rails
        return sf::Color::Transparent;
    });

    // 13: Sign post.
    drawTile(13, 0, [&](int x, int y) {
        if (x == 7 && y >= 10) return sf::Color(80, 50, 20); // post
        if (y < 8 && x >= 4 && x <= 11) return sf::Color(200, 200, 150); // sign board
        if (y == 2 && x >= 5 && x <= 10) return sf::Color(50, 50, 50); // text line
        return sf::Color::Transparent;
    });

    // 14: Cave entrance (dark hole in mountain).
    drawTile(14, 0, [&](int x, int y) {
        int cx = 7, cy = 10;
        int dx = x - cx, dy = y - cy;
        if (dx * dx + dy * dy < 36) return sf::Color(10, 5, 0); // dark entrance
        // Mountain around.
        int base = 100 + (x * 11 + y * 13) % 40;
        return sf::Color(base, base, base + 10);
    });

    // 15: Water edge (transition grass->water).
    drawTile(15, 0, [&](int x, int y) {
        if (y < 8) {
            int base = 55 + (x * 7 + y * 11) % 35;
            return sf::Color(25, base, 25); // grass
        }
        int b = 100 + (x * 11 + (y - 8) * 13) % 50;
        return sf::Color(30, 60, b); // water
    });

    // ---- Row 1: Water animation (4 frames) + Sand + Bridge ----

    // 16: Water frame 0.
    drawTile(0, 1, [&](int x, int y) {
        int b = 110 + (x * 11 + y * 13) % 45;
        return sf::Color(25, 55, b);
    });
    // 17: Water frame 1 (slightly shifted).
    drawTile(1, 1, [&](int x, int y) {
        int b = 115 + ((x + 1) * 11 + (y + 1) * 13) % 45;
        return sf::Color(25, 55, b);
    });
    // 18: Water frame 2.
    drawTile(2, 1, [&](int x, int y) {
        int b = 105 + ((x + 2) * 11 + (y + 2) * 13) % 45;
        return sf::Color(25, 55, b);
    });
    // 19: Water frame 3.
    drawTile(3, 1, [&](int x, int y) {
        int b = 112 + ((x + 3) * 11 + (y + 1) * 13) % 45;
        return sf::Color(25, 55, b);
    });

    // 20: Sand / beach.
    drawTile(4, 1, [&](int x, int y) {
        int base = 210 + (x * 7 + y * 11) % 25;
        return sf::Color(base, base - 10, base - 30);
    });

    // 21: Deep sand / wet sand.
    drawTile(5, 1, [&](int x, int y) {
        int base = 190 + (x * 7 + y * 11) % 20;
        return sf::Color(base, base - 5, base - 20);
    });

    // 22: Shallow water (transparent blue over sand).
    drawTile(6, 1, [&](int x, int y) {
        int base = 210 + (x * 7 + y * 11) % 25;
        int b = 120 + (x * 11 + y * 13) % 40;
        return sf::Color(base / 2, base / 2, b);
    });

    // 23: Bridge (wooden).
    drawTile(7, 1, [&](int x, int y) {
        if (y >= 6 && y <= 9) return sf::Color(110, 70, 30); // planks
        if (x == 2 || x == 13) return sf::Color(90, 55, 20); // side rails
        return sf::Color::Transparent;
    });

    // Fill remaining row 1 with water variants.
    for (int tx = 8; tx < cols; ++tx) {
        drawTile(tx, 1, [&](int x, int y) {
            int b = 100 + (x * 11 + y * 13) % 50;
            return sf::Color(30, 60, b);
        });
    }

    // ---- Row 2: Building variants ----

    // 32: Pokemon Center (red roof, white walls, red cross).
    drawTile(0, 2, [&](int x, int y) {
        if (y < 5) return sf::Color(180, 30, 30); // red roof
        if (y < 7) return sf::Color(150, 25, 25);
        if (x >= 6 && x <= 9 && y >= 8 && y <= 11) return sf::Color(220, 30, 30); // red cross
        return sf::Color(240, 240, 245); // white walls
    });

    // 33: Poke Mart (blue roof, orange walls).
    drawTile(1, 2, [&](int x, int y) {
        if (y < 5) return sf::Color(30, 80, 180); // blue roof
        if (y < 7) return sf::Color(25, 65, 160);
        if (x >= 5 && x <= 10 && y == 8) return sf::Color(255, 180, 0); // "MART" sign
        return sf::Color(255, 220, 180); // orange/tan walls
    });

    // 34: Gym (distinctive roof, badge symbols).
    drawTile(2, 2, [&](int x, int y) {
        if (y < 5) return sf::Color(80, 50, 160); // purple roof
        if (y < 7) return sf::Color(60, 40, 140);
        if (x >= 6 && x <= 9 && y >= 9 && y <= 12) return sf::Color(255, 215, 0); // badge
        return sf::Color(200, 190, 210); // lavender walls
    });

    // 35: House variant (green roof).
    drawTile(3, 2, [&](int x, int y) {
        if (y < 5) return sf::Color(30, 120, 40); // green roof
        if (y < 7) return sf::Color(25, 100, 35);
        return sf::Color(220, 200, 170); // cream walls
    });

    // 36: Red roof tile (for building construction).
    drawTile(4, 2, [&](int x, int y) {
        int base = 160 + (x * 3 + y * 5) % 20;
        return sf::Color(base, 30, 30);
    });

    // 37: Blue roof tile.
    drawTile(5, 2, [&](int x, int y) {
        int base = 180 + (x * 3 + y * 5) % 20;
        return sf::Color(30, 70, base);
    });

    // 38: Green roof tile.
    drawTile(6, 2, [&](int x, int y) {
        int base = 120 + (x * 3 + y * 5) % 20;
        return sf::Color(30, base, 40);
    });

    // 39: Beige wall tile.
    drawTile(7, 2, [&](int x, int y) {
        int base = 210 + (x * 5 + y * 3) % 15;
        return sf::Color(base, base - 10, base - 20);
    });

    // 40: White wall tile.
    drawTile(8, 2, [&](int x, int y) {
        int base = 235 + (x * 3 + y * 2) % 10;
        return sf::Color(base, base, base + 5);
    });

    // 41: Red brick wall.
    drawTile(9, 2, [&](int x, int y) {
        int base = 170 + (x * 7 + y * 3) % 20;
        if ((x + y) % 8 < 4) return sf::Color(base, base - 30, base - 30);
        return sf::Color(base - 10, base - 40, base - 40);
    });

    // 42: Window (lit).
    drawTile(10, 2, [&](int x, int y) {
        if (x >= 3 && x <= 12 && y >= 3 && y <= 11) return sf::Color(255, 255, 180); // lit window
        return sf::Color(50, 50, 80); // dark frame
    });

    // 43: Pokemon Center sign (red cross on white).
    drawTile(11, 2, [&](int x, int y) {
        if (x >= 5 && x <= 10 && y >= 4 && y <= 11) return sf::Color(255, 255, 255);
        if ((x == 7 && y >= 4 && y <= 11) || (y == 7 && x >= 5 && x <= 10))
            return sf::Color(220, 30, 30);
        return sf::Color::Transparent;
    });

    // 44: Poke Mart sign.
    drawTile(12, 2, [&](int x, int y) {
        if (x >= 4 && x <= 11 && y >= 4 && y <= 11) return sf::Color(255, 220, 180);
        if (y == 7 && x >= 5 && x <= 10) return sf::Color(255, 180, 0);
        return sf::Color::Transparent;
    });

    // 45: Gym sign (badge).
    drawTile(13, 2, [&](int x, int y) {
        int cx = 7, cy = 7;
        int dx = x - cx, dy = y - cy;
        if (dx * dx + dy * dy < 25) return sf::Color(255, 215, 0);
        return sf::Color::Transparent;
    });

    // 46: Fence post.
    drawTile(14, 2, [&](int x, int) {
        if (x >= 6 && x <= 9) return sf::Color(100, 60, 20);
        return sf::Color::Transparent;
    });

    // 47: Gate (closed).
    drawTile(15, 2, [&](int x, int y) {
        if (x >= 2 && x <= 13 && y >= 4 && y <= 12) return sf::Color(100, 60, 20);
        return sf::Color::Transparent;
    });

    // ---- Row 3: Trees, flowers, rocks, decor ----

    // 48: Thin tree.
    drawTile(0, 3, [&](int x, int y) {
        if (y > 10) return sf::Color(80, 50, 20);
        if (x == 7) return sf::Color(10, 80, 10);
        if (x >= 6 && x <= 8 && y < 10) return sf::Color(10, 70, 10);
        return sf::Color::Transparent;
    });

    // 49: Thick tree (3x3 canopy).
    drawTile(1, 3, [&](int x, int y) {
        if (y > 9) return sf::Color(90, 55, 25);
        if (x >= 4 && x <= 10 && y < 9) {
            int g = 40 + (x * 3 + y * 5) % 25;
            return sf::Color(5, g, 5);
        }
        return sf::Color::Transparent;
    });

    // 50: Bush (round, low).
    drawTile(2, 3, [&](int x, int y) {
        int cx = 7, cy = 11;
        int dx = x - cx, dy = y - cy;
        if (dx * dx + dy * dy < 36) {
            int g = 60 + (x * 5 + y * 3) % 30;
            return sf::Color(10, g, 10);
        }
        return sf::Color::Transparent;
    });

    // 51: Red flower.
    drawTile(3, 3, [&](int x, int y) {
        if (y >= 10) return sf::Color(30, 100, 30); // stem/leaf
        if ((x >= 5 && x <= 9) && (y >= 6 && y <= 9)) return sf::Color(220, 50, 80);
        return sf::Color::Transparent;
    });

    // 52: Yellow flower.
    drawTile(4, 3, [&](int x, int y) {
        if (y >= 10) return sf::Color(30, 100, 30);
        if ((x >= 5 && x <= 9) && (y >= 6 && y <= 9)) return sf::Color(255, 230, 50);
        return sf::Color::Transparent;
    });

    // 53: Blue flower.
    drawTile(5, 3, [&](int x, int y) {
        if (y >= 10) return sf::Color(30, 100, 30);
        if ((x >= 5 && x <= 9) && (y >= 6 && y <= 9)) return sf::Color(80, 120, 255);
        return sf::Color::Transparent;
    });

    // 54: Grass tuft (tall grass clump).
    drawTile(6, 3, [&](int x, int y) {
        if (y < 12) {
            int g = 80 + (x * 7 + y * 3) % 30;
            if (x % 3 == 1) return sf::Color(20, g, 20);
        }
        return sf::Color(30, 110, 30);
    });

    // 55: Small rock.
    drawTile(7, 3, [&](int x, int y) {
        int cx = 7, cy = 11;
        int dx = x - cx, dy = y - cy;
        if (dx * dx + dy * dy < 16) {
            int base = 130 + (x * 5) % 20;
            return sf::Color(base, base, base + 10);
        }
        return sf::Color::Transparent;
    });

    // 56: Medium rock.
    drawTile(8, 3, [&](int x, int y) {
        int cx = 7, cy = 11;
        int dx = x - cx, dy = y - cy;
        if (dx * dx + dy * dy < 36) {
            int base = 120 + (x * 5) % 25;
            return sf::Color(base, base - 5, base - 15);
        }
        return sf::Color::Transparent;
    });

    // 57: Large rock / boulder.
    drawTile(9, 3, [&](int x, int y) {
        int cx = 7, cy = 10;
        int dx = x - cx, dy = y - cy;
        if (dx * dx + dy * dy < 50) {
            int base = 110 + (x * 7 + y * 3) % 30;
            return sf::Color(base, base - 10, base - 20);
        }
        return sf::Color::Transparent;
    });

    // 58: Tree stump.
    drawTile(10, 3, [&](int x, int y) {
        if (y > 8) {
            int base = 100 + (x * 5) % 30;
            return sf::Color(base, base - 20, base - 40);
        }
        // Rings on top.
        if (y == 8) return sf::Color(80, 50, 25);
        return sf::Color::Transparent;
    });

    // 59: Cut tree (thin, can be Cut).
    drawTile(11, 3, [&](int x, int y) {
        if (y > 11) return sf::Color(80, 50, 20);
        if (x >= 5 && x <= 9 && y < 11) {
            int g = 70 + (x * 3) % 20;
            return sf::Color(20, g, 20);
        }
        return sf::Color::Transparent;
    });

    // 60: Berry tree.
    drawTile(12, 3, [&](int x, int y) {
        if (y > 9) return sf::Color(90, 55, 25);
        if (x >= 5 && x <= 9 && y < 9) {
            int g = 50 + (x * 3) % 20;
            sf::Color c(10, g, 10);
            // Berries.
            if ((x == 6 && y == 4) || (x == 8 && y == 6)) return sf::Color(220, 30, 30);
            if ((x == 7 && y == 5) || (x == 9 && y == 3)) return sf::Color(255, 180, 0);
            return c;
        }
        return sf::Color::Transparent;
    });

    // 61: Palm tree (for tropical areas).
    drawTile(13, 3, [&](int x, int y) {
        if (y > 6 && x == 7) return sf::Color(100, 65, 25); // trunk
        if (y <= 6) {
            // Fronds.
            if ((y == 2 && x == 7) || (y == 3 && x >= 5 && x <= 9) ||
                (y == 4 && x >= 4 && x <= 10) || (y == 5 && x >= 3 && x <= 11))
                return sf::Color(20, 120, 30);
        }
        return sf::Color::Transparent;
    });

    // 62: Pine tree.
    drawTile(14, 3, [&](int x, int y) {
        if (y > 10) return sf::Color(80, 50, 20);
        int cx = 7;
        int width = (14 - y) / 2;
        if (x >= cx - width && x <= cx + width) {
            int g = 30 + (y * 3) % 20;
            return sf::Color(5, g, 5);
        }
        return sf::Color::Transparent;
    });

    // 63: Fallen log.
    drawTile(15, 3, [&](int x, int y) {
        if (y >= 8 && y <= 10) {
            int base = 90 + (x * 7) % 25;
            return sf::Color(base, base - 20, base - 40);
        }
        return sf::Color::Transparent;
    });

    // Rows 4-7: Leave empty for interior tiles / future use.
    for (int ty = 4; ty < rows; ++ty) {
        for (int tx = 0; tx < cols; ++tx) {
            drawTile(tx, ty, [&](int x, int y) {
                (void)x; (void)y;
                return sf::Color::Transparent;
            });
        }
    }

    if (!tilesetTexture_.loadFromImage(img)) {
        std::cerr << "game: failed to create procedural tileset\n";
        return false;
    }
    tilesetTexture_.setSmooth(false); // pixel-perfect
    world_.tileset = &tilesetTexture_;
    world_.tilesetColumns = cols;
    return true;
}

bool Game::headlessSmoke(const Monster& mon) {
    std::cout << "smoke: data dir  = " << dataDir_ << "\n";
    std::cout << "smoke: monsters  = " << data_.count() << "\n";
    std::cout << "smoke: moves     = " << static_cast<int>(data_.moves().size()) << "\n";
    cache_ = new SpriteCache(spriteCacheDir_);
    if (!cache_->get(mon.frontSprite)) return false;
    std::cout << "smoke: ok        = rendered " << mon.name
              << " sprite (" << cache_->loadedCount() << " cached)\n";
    return true;
}

bool Game::run() {
    if (!loadData()) return false;
    if (!createTileset()) return false;

    // Initialize the MapStore so loadMap can read from JSON.
    world_.initMapStore(dataDir_);

    // Load the starting map.
    if (!world_.loadMap(startMap_)) {
        std::cerr << "game: failed to load starting map\n";
        return false;
    }

    // Font for UI.
    fontOk_ = font_.loadFromFile("/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf");

    // Create window.
    window_ = std::make_unique<sf::RenderWindow>(sf::VideoMode(640, 480), "Pixelmon", sf::Style::Default);
    window_->setFramerateLimit(60);
    window_->setKeyRepeatEnabled(false);

    // Initialize sprite cache for monster sprites in battle.
    cache_ = new SpriteCache(spriteCacheDir_);

    sf::Clock clock;
    while (window_->isOpen()) {
        float dt = clock.restart().asSeconds();

        // Event handling.
        sf::Event event;
        while (window_->pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window_->close();
            } else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    if (state_ == GameState::Battle) {
                        endBattle();
                    } else {
                        window_->close();
                    }
                } else if (event.key.code == sf::Keyboard::F1) {
                    // Debug: force wild encounter.
                    startBattle(true, 16); // Pidgey
                }
                if (state_ == GameState::Overworld) {
                    handleOverworldInput(event);
                } else if (state_ == GameState::Battle) {
                    handleBattleInput(event);
                }
            }
        }

        // Update.
        if (state_ == GameState::Overworld) {
            updateOverworld(dt);
        } else if (state_ == GameState::Battle) {
            updateBattle(dt);
        }

        // Render.
        window_->clear(sf::Color(28, 28, 36));
        if (state_ == GameState::Overworld) {
            renderOverworld();
        } else if (state_ == GameState::Battle) {
            renderBattle();
        }
        window_->display();
    }
    return true;
}

void Game::handleOverworldInput(const sf::Event& event) {
    if (world_.player.moving) return;

    if (event.type == sf::Event::KeyPressed) {
        Direction dir = Direction::Down;

        switch (event.key.code) {
            case sf::Keyboard::Up:
            case sf::Keyboard::W:
                dir = Direction::Up;
                world_.tryMove(dir);
                break;
            case sf::Keyboard::Down:
            case sf::Keyboard::S:
                dir = Direction::Down;
                world_.tryMove(dir);
                break;
            case sf::Keyboard::Left:
            case sf::Keyboard::A:
                dir = Direction::Left;
                world_.tryMove(dir);
                break;
            case sf::Keyboard::Right:
            case sf::Keyboard::D:
                dir = Direction::Right;
                world_.tryMove(dir);
                break;
            case sf::Keyboard::Space:
            case sf::Keyboard::Enter:
            case sf::Keyboard::Z:
                // Interact (talk to trainer, open door, etc.)
                if (auto trainerIdx = world_.checkTrainerInteraction()) {
                    startBattle(false, 0, *trainerIdx);
                }
                break;
            default:
                break;
        }
    }
}

void Game::handleBattleInput(const sf::Event& event) {
    if (event.type != sf::Event::KeyPressed) return;

    // Simple battle controls: 1-4 for moves, 5 for struggle.
    if (battleCtx_.phase == pm::BattlePhase::InputWait) {
        int moveSlot = -1;
        switch (event.key.code) {
            case sf::Keyboard::Num1: moveSlot = 0; break;
            case sf::Keyboard::Num2: moveSlot = 1; break;
            case sf::Keyboard::Num3: moveSlot = 2; break;
            case sf::Keyboard::Num4: moveSlot = 3; break;
            default: break;
        }
        if (moveSlot >= 0 && moveSlot < (int)battleCtx_.playerTeam[battleCtx_.playerActiveIdx].moves.size()) {
            pm::executeMove(battleCtx_, true, moveSlot, data_);
            battleCtx_.phase = pm::BattlePhase::ExecuteMove;
        }
    }
}

void Game::updateOverworld(float dt) {
    // Realtime hold-to-move polling (key repeat is disabled, so events only
    // fire once per press).
    if (!world_.player.moving && window_ && window_->hasFocus()) {
        struct KeyDir { sf::Keyboard::Key key; sf::Keyboard::Key alt; Direction dir; };
        static const KeyDir keys[] = {
            {sf::Keyboard::Up, sf::Keyboard::W, Direction::Up},
            {sf::Keyboard::Down, sf::Keyboard::S, Direction::Down},
            {sf::Keyboard::Left, sf::Keyboard::A, Direction::Left},
            {sf::Keyboard::Right, sf::Keyboard::D, Direction::Right},
        };
        for (const auto& k : keys) {
            if (sf::Keyboard::isKeyPressed(k.key) || sf::Keyboard::isKeyPressed(k.alt)) {
                world_.tryMove(k.dir);
                break;
            }
        }
    }

    world_.update(dt);

    // Check if battle was triggered.
    if (world_.inBattle) {
        if (world_.pendingBattleType == "wild") {
            startBattle(true, world_.pendingEncounterSpecies);
        } else if (world_.pendingBattleType == "trainer") {
            startBattle(false, 0, world_.pendingTrainerIndex);
        }
        world_.inBattle = false;
    }
}

void Game::updateBattle(float dt) {
    (void)dt;
    if (!battleInitialized_) return;

    // Run battle steps automatically for enemy turns, wait for player input.
    if (battleCtx_.result != pm::BattleResult::Ongoing) {
        // Battle ended, wait for key to return.
        return;
    }

    if (battleCtx_.phase == pm::BattlePhase::InputWait) {
        // Waiting for player input (handled in handleBattleInput).
        return;
    }

    // Auto-advance battle phases.
    if (!pm::battleStep(battleCtx_, data_)) {
        // Battle ended.
        return;
    }

    // If it's enemy's turn, auto-pick and execute move.
    if (battleCtx_.phase == pm::BattlePhase::InputWait) {
        int enemyMove = pm::pickEnemyMove(battleCtx_);
        if (enemyMove >= 0) {
            pm::executeMove(battleCtx_, false, enemyMove, data_);
            battleCtx_.phase = pm::BattlePhase::ExecuteMove;
        }
    }
}

void Game::startBattle(bool isWild, int speciesNum, int trainerIndex) {
    state_ = GameState::Battle;
    battleInitialized_ = false;

    if (isWild) {
        // Wild battle: player's first party member vs wild Pokemon.
        int playerSpecies = world_.player.party.empty() ? 1 : world_.player.party[0];
        battleCtx_ = pm::createDemoBattle(data_, playerSpecies, speciesNum, 5);
    } else {
        // Trainer battle.
        const auto& trainer = world_.currentMap.trainers[trainerIndex];
        int playerSpecies = world_.player.party.empty() ? 1 : world_.player.party[0];
        // For simplicity, use trainer's first Pokemon.
        int enemySpecies = trainer.party.empty() ? 4 : trainer.party[0];
        battleCtx_ = pm::createDemoBattle(data_, playerSpecies, enemySpecies, 5);
    }
    battleInitialized_ = true;
}

void Game::endBattle() {
    state_ = GameState::Overworld;
    battleInitialized_ = false;

    // If trainer was defeated, mark as such.
    if (world_.pendingBattleType == "trainer" && world_.pendingTrainerIndex >= 0) {
        auto& trainers = world_.currentMap.trainers;
        if (world_.pendingTrainerIndex < (int)trainers.size()) {
            trainers[world_.pendingTrainerIndex].defeated = true;
        }
    }
    world_.pendingBattleType.clear();
    world_.pendingEncounterSpecies = 0;
    world_.pendingTrainerIndex = -1;
}

const sf::Texture& Game::currentTilesetTexture() {
    std::string stem = world_.currentMap.tilesetFile.empty()
        ? "overworld" : world_.currentMap.tilesetFile;
    auto [it, inserted] = atlasTextures_.try_emplace(stem);
    if (inserted) {
        // Resolve the atlas base dir once without hitting missing paths
        // (SFML spams an error log for every failed probe).
        if (tilesetBase_.empty()) {
            const std::string primary = dataDir_ + "/maps/tilesets/";
            tilesetBase_ = std::filesystem::exists(primary + "overworld.png")
                ? primary : "src/data/maps/tilesets/";
        }
        if (it->second.loadFromFile(tilesetBase_ + stem + ".png")) {
            it->second.setSmooth(false);
        } else {
            std::cerr << "game: no tileset atlas for '" << stem
                      << "' (" << tilesetBase_ << stem << ".png)\n";
        }
    }
    // Fall back to the procedural tileset when the atlas is missing.
    if (it->second.getSize().x == 0) return tilesetTexture_;
    return it->second;
}

void Game::renderOverworld() {
    const int tileSize = TILE_SIZE;
    const int scale = world_.renderScale; // 3x render scale for 16->48px tiles
    const int scaledTile = tileSize * scale;

    // Camera is in scaled pixels. Convert to tile coords for culling.
    int camTileX = (int)(world_.camera.x / scaledTile);
    int camTileY = (int)(world_.camera.y / scaledTile);

    // Calculate visible tile range.
    int startX = std::max(0, camTileX - 1);
    int startY = std::max(0, camTileY - 1);
    int endX = std::min(world_.currentMap.width, startX + 640 / scaledTile + 2);
    int endY = std::min(world_.currentMap.height, startY + 480 / scaledTile + 2);

    sf::Sprite tileSprite(currentTilesetTexture());

    // Render ground layer.
    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            const Tile& tile = world_.currentMap.ground[y][x];
            int sx = (tile.spriteIndex % world_.tilesetColumns) * tileSize;
            int sy = (tile.spriteIndex / world_.tilesetColumns) * tileSize;
            tileSprite.setTextureRect(sf::IntRect(sx, sy, tileSize, tileSize));
            tileSprite.setPosition(
                x * scaledTile - world_.camera.x,
                y * scaledTile - world_.camera.y
            );
            tileSprite.setScale(scale, scale);
            window_->draw(tileSprite);
        }
    }

    // Render objects layer.
    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            const Tile& tile = world_.currentMap.objects[y][x];
            // Skip empty tiles (default-constructed Grass/Dirt with spriteIndex 0).
            // Only skip if it's truly empty (spriteIndex 0 AND type Grass/Dirt).
            if (tile.spriteIndex == 0 && (tile.type == TileType::Grass || tile.type == TileType::Dirt)) continue;
            int sx = (tile.spriteIndex % world_.tilesetColumns) * tileSize;
            int sy = (tile.spriteIndex / world_.tilesetColumns) * tileSize;
            tileSprite.setTextureRect(sf::IntRect(sx, sy, tileSize, tileSize));
            tileSprite.setPosition(
                x * scaledTile - world_.camera.x,
                y * scaledTile - world_.camera.y
            );
            tileSprite.setScale(scale, scale);
            window_->draw(tileSprite);
        }
    }

    // Render overlay layer (above objects, below player for tall grass coverage).
    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            const Tile& tile = world_.currentMap.overlay[y][x];
            if (tile.spriteIndex == 0 && (tile.type == TileType::Grass || tile.type == TileType::Dirt)) continue;
            int sx = (tile.spriteIndex % world_.tilesetColumns) * tileSize;
            int sy = (tile.spriteIndex / world_.tilesetColumns) * tileSize;
            tileSprite.setTextureRect(sf::IntRect(sx, sy, tileSize, tileSize));
            tileSprite.setPosition(
                x * scaledTile - world_.camera.x,
                y * scaledTile - world_.camera.y
            );
            tileSprite.setScale(scale, scale);
            window_->draw(tileSprite);
        }
    }

    // Render player sprite (using front sprite from data).
    const Monster* playerMon = data_.monsterByNum(world_.player.party.empty() ? 1 : world_.player.party[0]);
    if (playerMon && cache_) {
        sf::Texture* tex = cache_->get(playerMon->frontSprite);
        if (tex) {
            sf::Sprite playerSprite(*tex);
            // Overworld player sprite: scale to fit in one tile (48x48).
            // Battle sprites are 96x96, so use 0.5 (48/96) to fit in tile.
            const float playerScale = 0.5f;
            playerSprite.setScale(playerScale, playerScale);
            // Center the sprite on the tile (offset by half the difference).
            const float spriteOffset = (scaledTile - 96 * playerScale) / 2.f;
            // Player position with movement interpolation.
            float px = world_.player.x;
            float py = world_.player.y;
            if (world_.player.moving) {
                float progress = world_.player.moveProgress;
                switch (world_.player.facing) {
                    case Direction::Up:    py -= progress; break;
                    case Direction::Down:  py += progress; break;
                    case Direction::Left:  px -= progress; break;
                    case Direction::Right: px += progress; break;
                }
            }
            playerSprite.setPosition(
                px * scaledTile - world_.camera.x + spriteOffset,
                py * scaledTile - world_.camera.y + spriteOffset
            );
            window_->draw(playerSprite);
        } else {
            // Fallback: draw a green square if sprite fails to load.
            sf::RectangleShape fallback(sf::Vector2f(scaledTile, scaledTile));
            fallback.setFillColor(sf::Color(50, 200, 50));
            float px = world_.player.x;
            float py = world_.player.y;
            fallback.setPosition(
                px * scaledTile - world_.camera.x,
                py * scaledTile - world_.camera.y
            );
            window_->draw(fallback);
        }
    }

    // Render trainer NPCs.
    for (const auto& trainer : world_.currentMap.trainers) {
        if (trainer.defeated) continue;
        // Simple NPC sprite (red square for now).
        sf::RectangleShape npcShape(sf::Vector2f(scaledTile, scaledTile));
        npcShape.setFillColor(sf::Color(200, 50, 50));
        npcShape.setPosition(
            trainer.x * scaledTile - world_.camera.x,
            trainer.y * scaledTile - world_.camera.y
        );
        window_->draw(npcShape);
    }

    // UI: location name.
    if (fontOk_) {
        sf::Text locText(world_.currentMap.name, font_, 16);
        locText.setPosition(10.f, 10.f);
        locText.setFillColor(sf::Color::White);
        window_->draw(locText);

        // Coordinates.
        sf::Text coordText("X: " + std::to_string(world_.player.x) + " Y: " + std::to_string(world_.player.y), font_, 12);
        coordText.setPosition(10.f, 30.f);
        coordText.setFillColor(sf::Color(200, 200, 200));
        window_->draw(coordText);

        // Controls hint.
        sf::Text hintText("WASD/Arrows: Move | Space: Interact | ESC: Quit", font_, 10);
        hintText.setPosition(10.f, 450.f);
        hintText.setFillColor(sf::Color(150, 150, 150));
        window_->draw(hintText);
    }
}

void Game::renderBattle() {
    if (!fontOk_) return;

    // Battle background.
    sf::RectangleShape bg(sf::Vector2f(640, 480));
    bg.setFillColor(sf::Color(40, 40, 60));
    window_->draw(bg);

    // Battle log (last 6 lines).
    int logY = 300;
    size_t startIdx = battleCtx_.log.size() > 6 ? battleCtx_.log.size() - 6 : 0;
    for (size_t i = startIdx; i < battleCtx_.log.size(); ++i) {
        sf::Text logText(battleCtx_.log[i].text, font_, 14);
        logText.setPosition(20.f, float(logY));
        logText.setFillColor(battleCtx_.log[i].isPlayer ? sf::Color(100, 200, 100) : sf::Color(200, 100, 100));
        window_->draw(logText);
        logY += 20;
    }

    // Player mon info (bottom left).
    const auto& pMon = battleCtx_.playerTeam[battleCtx_.playerActiveIdx];
    sf::Text pName(pMon.nickname + "  Lv." + std::to_string(pMon.level), font_, 16);
    pName.setPosition(20.f, 360.f);
    pName.setFillColor(sf::Color::White);
    window_->draw(pName);

    // HP bar.
    float hpRatio = pMon.maxHp > 0 ? float(pMon.currentHp) / pMon.maxHp : 0.f;
    sf::RectangleShape hpBg(sf::Vector2f(200, 16));
    hpBg.setPosition(20.f, 385.f);
    hpBg.setFillColor(sf::Color(60, 60, 60));
    window_->draw(hpBg);
    sf::RectangleShape hpBar(sf::Vector2f(200 * hpRatio, 16));
    hpBar.setPosition(20.f, 385.f);
    hpBar.setFillColor(hpRatio > 0.5f ? sf::Color(60, 200, 60) : (hpRatio > 0.2f ? sf::Color(200, 200, 60) : sf::Color(200, 60, 60)));
    window_->draw(hpBar);

    sf::Text hpText(std::to_string(pMon.currentHp) + "/" + std::to_string(pMon.maxHp), font_, 12);
    hpText.setPosition(230.f, 385.f);
    hpText.setFillColor(sf::Color::White);
    window_->draw(hpText);

    // Enemy mon info (top right).
    const auto& eMon = battleCtx_.enemyTeam[battleCtx_.enemyActiveIdx];
    sf::Text eName(eMon.nickname + "  Lv." + std::to_string(eMon.level), font_, 16);
    eName.setPosition(420.f, 80.f);
    eName.setFillColor(sf::Color::White);
    window_->draw(eName);

    float eHpRatio = eMon.maxHp > 0 ? float(eMon.currentHp) / eMon.maxHp : 0.f;
    sf::RectangleShape eHpBg(sf::Vector2f(200, 16));
    eHpBg.setPosition(420.f, 105.f);
    eHpBg.setFillColor(sf::Color(60, 60, 60));
    window_->draw(eHpBg);
    sf::RectangleShape eHpBar(sf::Vector2f(200 * eHpRatio, 16));
    eHpBar.setPosition(420.f, 105.f);
    eHpBar.setFillColor(eHpRatio > 0.5f ? sf::Color(60, 200, 60) : (eHpRatio > 0.2f ? sf::Color(200, 200, 60) : sf::Color(200, 60, 60)));
    window_->draw(eHpBar);

    // Move list (bottom).
    sf::Text movesHeader("Moves (1-4):", font_, 14);
    movesHeader.setPosition(20.f, 410.f);
    movesHeader.setFillColor(sf::Color(200, 200, 100));
    window_->draw(movesHeader);

    const auto& pMoves = pMon.moves;
    for (size_t i = 0; i < pMoves.size() && i < 4; ++i) {
        const auto& slot = pMoves[i];
        if (slot.move) {
            std::string moveText = std::to_string(i + 1) + ". " + slot.move->name +
                " (PP " + std::to_string(slot.pp) + "/" + std::to_string(slot.maxPp) + ")";
            sf::Text moveTextObj(moveText, font_, 12);
            moveTextObj.setPosition(20.f, 430.f + i * 16);
            moveTextObj.setFillColor(slot.pp > 0 ? sf::Color::White : sf::Color(150, 50, 50));
            window_->draw(moveTextObj);
        }
    }

    // Battle result message.
    if (battleCtx_.result == pm::BattleResult::PlayerWon) {
        sf::Text winText("VICTORY! Press ESC to continue.", font_, 20);
        winText.setPosition(200.f, 200.f);
        winText.setFillColor(sf::Color(100, 255, 100));
        window_->draw(winText);
    } else if (battleCtx_.result == pm::BattleResult::PlayerLost) {
        sf::Text loseText("DEFEAT... Press ESC to continue.", font_, 20);
        loseText.setPosition(200.f, 200.f);
        loseText.setFillColor(sf::Color(255, 100, 100));
        window_->draw(loseText);
    }
}

bool Game::renderToFile(const Monster& mon, const std::string& outputPath) {
    // Data should already be loaded by caller; don't reload (would invalidate refs).
    cache_ = new SpriteCache(spriteCacheDir_);
    sf::Texture* tex = cache_->get(mon.frontSprite);
    if (!tex) return false;

    const sf::Vector2u size = tex->getSize();
    const float scale = 3.0f;
    const unsigned int w = size.x * scale;
    const unsigned int h = size.y * scale;

    const sf::Image* src = cache_->image(mon.frontSprite);
    if (!src) return false;

    sf::Image out;
    out.create(w, h, sf::Color(28, 28, 36));
    for (unsigned int y = 0; y < size.y; ++y) {
        for (unsigned int x = 0; x < size.x; ++x) {
            const sf::Color c = src->getPixel(x, y);
            for (unsigned int sy = 0; sy < scale; ++sy) {
                for (unsigned int sx = 0; sx < scale; ++sx) {
                    out.setPixel(x * scale + sx, y * scale + sy, c);
                }
            }
        }
    }

    if (!out.saveToFile(outputPath)) {
        std::cerr << "game: failed to save " << outputPath << "\n";
        return false;
    }
    std::cout << "render: saved " << outputPath << " (" << w << "x" << h << ")\n";
    return true;
}

}  // namespace pm

#include "core/Game.hpp"

#include <iostream>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace pm {

Game::Game(const std::string& dataDir, const std::string& spriteCacheDir)
    : dataDir_(dataDir), spriteCacheDir_(spriteCacheDir) {}

bool Game::loadData() {
    // Prefer the caller's dir (build/ has the copied JSON); fall back to the
    // source tree so the app also works when run from the repo root.
    if (!data_.load(dataDir_)) {
        if (!data_.load("src/data")) {
            std::cerr << "game: could not find data in '" << dataDir_ << "' or 'src/data'\n";
            return false;
        }
    }
    return true;
}

bool Game::loadSprite(const Monster& mon) {
    cache_ = new SpriteCache(spriteCacheDir_);
    sf::Texture* tex = cache_->get(mon.frontSprite);
    if (!tex) {
        std::cerr << "game: failed to load sprite for " << mon.name << " (" << mon.frontSprite << ")\n";
        return false;
    }

    texture_ = *tex;          // textures share the GPU handle; safe copy
    texture_.setSmooth(true);
    sprite_.setTexture(texture_);
    return true;
}

bool Game::load(const Monster& mon) {
    return loadData() && loadSprite(mon);
}

bool Game::headlessSmoke(const Monster& mon) {
    std::cout << "smoke: data dir  = " << dataDir_ << "\n";
    std::cout << "smoke: monsters  = " << data_.count() << "\n";
    std::cout << "smoke: moves     = " << static_cast<int>(data_.moves().size()) << "\n";
    if (!loadSprite(mon)) return false;
    std::cout << "smoke: ok        = rendered " << mon.name
              << " sprite (" << cache_->loadedCount() << " cached)\n";
    return true;
}

bool Game::run(const Monster& mon) {
    if (!loadSprite(mon)) return false;

    const sf::Vector2u size = texture_.getSize();
    // Scale the square sprite to fit on typical displays (1080p and up) with
    // room for the name label, rather than a window taller than the screen.
    const float scale = 2.0f;
    const unsigned int spriteW = size.x * scale;
    const unsigned int spriteH = size.y * scale;
    const unsigned int w = spriteW + 320;
    const unsigned int h = spriteH + 120;

    sf::RenderWindow window(sf::VideoMode(w, h), "Pixelmon", sf::Style::Default);
    window.setFramerateLimit(30);

    // Font is best-effort: the label is a nicety, not required for the demo.
    sf::Font font;
    bool fontOk = font.loadFromFile("/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf");

    while (window.isOpen()) {
        sf::Event event;
        while (window.waitEvent(event)) {
            if (event.type == sf::Event::Closed ||
                (event.type == sf::Event::KeyPressed &&
                 event.key.code == sf::Keyboard::Escape)) {
                window.close();
            }
        }

        window.clear(sf::Color(28, 28, 36));
        sprite_.setOrigin(size.x / 2.f, size.y / 2.f);
        sprite_.setPosition(w / 2.f + 60.f, h / 2.f);
        window.draw(sprite_);

        if (fontOk) {
            sf::Text label(mon.name, font, 20);
            label.setPosition(20.f, 20.f);
            window.draw(label);
        }
        window.display();
    }
    return true;
}

bool Game::renderToFile(const Monster& mon, const std::string& outputPath) {
    if (!loadSprite(mon)) return false;

    const sf::Vector2u size = texture_.getSize();
    const float scale = 3.0f;
    const unsigned int w = size.x * scale;
    const unsigned int h = size.y * scale;

    // SFML 2.6 can't read pixels back from a texture, so build the output
    // image directly: nearest-neighbor upscale the real sprite onto the
    // background, matching the 3x scaling used by the windowed render.
    const sf::Image* src = cache_->image(mon.frontSprite);
    if (!src) {
        std::cerr << "game: failed to decode sprite for " << mon.name << "\n";
        return false;
    }

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

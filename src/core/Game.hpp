#pragma once
#include <string>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include "data/DataRepository.hpp"
#include "ui/SpriteCache.hpp"

namespace pm {

// Minimal SFML application. Loads the generated data repository + a PokeAPI
// sprite, then either renders it in a window (full run) or just proves the
// whole pipeline works headlessly (smoke test, no window / display needed).
class Game {
public:
    Game(const std::string& dataDir, const std::string& spriteCacheDir);

    // Loads data (with a src/data fallback) and the given monster's front sprite.
    bool load(const Monster& mon);

    // Data repo only (call before querying the repository).
    bool loadData();
    // Front sprite + texture only (assumes data is already loaded).
    bool loadSprite(const Monster& mon);

    // Full windowed run: renders the monster until the window is closed.
    bool run(const Monster& mon);

    // Headless check: load data + sprite + texture, print counts, no window.
    bool headlessSmoke(const Monster& mon);

    // Render the monster to an offscreen texture and save a PNG. Works
    // without a display, so it doubles as visual proof of the render path.
    bool renderToFile(const Monster& mon, const std::string& outputPath);

    const DataRepository& data() const { return data_; }

private:
    std::string dataDir_;
    std::string spriteCacheDir_;
    DataRepository data_;
    SpriteCache* cache_ = nullptr;
    sf::Texture texture_;
    sf::Sprite sprite_;
};

}  // namespace pm
